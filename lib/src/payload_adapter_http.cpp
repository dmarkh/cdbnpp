
#include "cdbnpp/payload_adapter_http.h"

#include "cdbnpp/base64.h"
#include "cdbnpp/json_schema.h"
#include "cdbnpp/log.h"
#include "cdbnpp/rng.h"
#include "cdbnpp/uuid.h"

namespace CDBNPP {

	PayloadAdapterHttp::PayloadAdapterHttp() : IPayloadAdapter("http"), mHttpClient(new HttpClient) {}

	PayloadResults_t PayloadAdapterHttp::getPayloads( const std::set<std::string>& paths, const std::vector<std::string>& flavors,
			const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime, int64_t eventTime, int64_t run, int64_t seq ) {

		// TODO: possible latency optimization: do bulk HTTP request here instead of many single requests

		PayloadResults_t res;

		if ( !ensureMetadata() ) {
			return res;
		}

		std::set<std::string> unfolded_paths{};
		for ( const auto& path : paths ) {
			if ( mPaths.count(path) == 0 ) {
				for ( const auto& [ key, value ] : mPaths ) {
					if ( string_starts_with( key, path ) && value->mode() > 0 ) {
						unfolded_paths.insert( key );
					}
				}
			} else {
				unfolded_paths.insert( path );
			}
		}

		for ( const auto& path : unfolded_paths ) {
			Result<SPayloadPtr_t> rc = getPayload( path, flavors, maxEntryTimeOverrides, maxEntryTime, eventTime, run, seq );
			if ( rc.valid() ) {
				SPayloadPtr_t p = rc.get();
				res.insert({ p->directory() + "/" + p->structName(), p });
			}
		}
		return res;
	}

	Result<SPayloadPtr_t> PayloadAdapterHttp::getPayload( const std::string& path, const std::vector<std::string>& service_flavors,
			const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime, int64_t eventTime, int64_t run, int64_t seq ) {

		Result<SPayloadPtr_t> res;

		if ( !ensureMetadata() ) {
			res.setMsg( "http adapter cannot download metadata" );
			return res;
		}

		auto [ flavors, directory, structName, is_path_valid ] = Payload::decodePath( path );

		if ( !is_path_valid ) {
			res.setMsg( "request path has not been decoded, path: " + path );
			return res;
		}

		if ( !service_flavors.size() && !flavors.size() ) {
			res.setMsg( "request does not specify flavor, path: " + path );
			return res;
		}

		if ( !directory.size() ) {
			res.setMsg( "request does not specify path: " + path );
			return res;
		}

		if ( !structName.size() ) {
			res.setMsg( "request does not specify structName: " + path );
			return res;
		}

		std::string dirpath = directory + "/" + structName;
    // check for path-specific maxEntryTime overrides
    if ( maxEntryTimeOverrides.size() ) {
      for ( const auto& [ opath, otime ] : maxEntryTimeOverrides ) {
        if ( string_starts_with( dirpath, opath ) ) {
          maxEntryTime = otime;
          break;
        }
      }
    }

		// get tag
		auto tagit = mPaths.find( dirpath );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "cannot find tag for " + dirpath );
			return res;
		}

		// get tbname from tag
		std::string tbname = tagit->second->tbname();
		int64_t mode = tagit->second->mode();

		if ( !tbname.size() ) {
			res.setMsg( "requested path points to the directory, need struct: " + dirpath );
			return res;
		}

		for ( const auto& flavor : ( flavors.size() ? flavors : service_flavors ) ) {
			std::string params = "?";
			params += "tb=" + tbname + "&f=" + flavor + "&mt=" + std::to_string(maxEntryTime);
			if ( mode == 1 ) {
				params += "&et=" + std::to_string(eventTime);
			} else if ( mode == 2 ) {
				params += "&run=" + std::to_string(run) + "&seq=" + std::to_string(seq);
			}
			params += "&tm=" + std::to_string(std::time(nullptr)); // NOTE: limits caching

			HttpResponse r = makeGetRequest( "admin", "/payload_get/" + params );
			if ( r.error ) {
				res.setMsg( "payload get via http(s) failed. Url: " + r.url + ", error: " + std::to_string(r.error) );
				return res;
			}

			nlohmann::json reply = nlohmann::json::parse( r.text.begin(), r.text.end(), nullptr, false, true );

			if ( reply.empty() || reply.is_discarded() || !reply.contains("payload") ) {
				res.setMsg( "server replied with malformed data (not json)" );
				return res;
			}

			// got data from server, assemble Payload
			auto p = std::make_shared<Payload>(
					reply["payload"]["id"].get<std::string>(), reply["payload"]["pid"].get<std::string>(),
					reply["payload"]["flavor"].get<std::string>(),
					structName, directory,
					reply["payload"]["ct"],  reply["payload"]["bt"],
					reply["payload"]["et"],  reply["payload"]["dt"],
					reply["payload"]["run"], reply["payload"]["seq"]
					);
			p->setData( std::string(""), reply["payload"]["fmt"] );
			p->setURI( reply["payload"]["uri"] );

			if ( string_starts_with( p->URI(), "db://" ) ) {
				// rewrite URI endpoint to HTTP if data is receved via HTTP adapter
				std::string uri = mConfig["adapters"]["http"]["get"][0]["url"].get<std::string>() + "/download/?tbname=" + tbname + "&id=" + p->id();
				p->setURI( uri );
			}

			res = p;
			return res;

		} // loop over flavors

		return res;
	}

	Result<std::string> PayloadAdapterHttp::setPayload( const SPayloadPtr_t& payload ) {

		Result <std::string> res;

		if ( !payload->ready() ) {
			res.setMsg( "payload is not ready to be stored to HTTP" );
			return res;
		}

		if ( !ensureMetadata() ) {
			res.setMsg( "http adapter cannot download metadata" );
			return res;
		}

		if ( !hasAccess("set") ) {
			res.setMsg( "http adapter is not configured" );
			return res;
		}

		// get tag, fetch tbname
		auto tagit = mPaths.find( payload->directory() + "/" + payload->structName() );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "cannot find payload tag in the database: " + payload->directory() + "/" + payload->structName() );
			return res;
		}

		if ( payload->format() != "dat" ) {
			// check if schema exists, validate payload against it
			Result<std::string> schema = getTagSchema( payload->directory() + "/" + payload->structName() );
			if ( schema.valid() ) {
				std::string json_schema = schema.get();
				nlohmann::json pjs = payload->dataAsJson();
				bool rc = validate_json_using_schema( pjs, json_schema );
				if ( !rc ) {
					res.setMsg( "payload failed to validate against json schema" );
					return res;
				}
			}
		}

		std::string tbname = tagit->second->tbname();
		sanitize_alnumuscore(tbname);

		HttpPostParams_t params{};
		params.push_back({ "id", payload->id() });
		params.push_back({ "pid", payload->pid() });
		params.push_back({ "flavor", payload->flavor() });
		params.push_back({ "ct", std::to_string(std::time(nullptr)) });
		params.push_back({ "dt", std::to_string(payload->deactiveTime()) });
		params.push_back({ "bt", std::to_string(payload->beginTime()) });
		params.push_back({ "et", std::to_string(payload->endTime()) });
		params.push_back({ "run", std::to_string(payload->run()) });
		params.push_back({ "seq", std::to_string(payload->seq()) });
		params.push_back({ "fmt", payload->format() });
		params.push_back({ "uri", payload->URI() });
		params.push_back({ "tbname", tbname });
		params.push_back({ "data", payload->data() });
		params.push_back({ "data_size", std::to_string( payload->data().size() ) });

		HttpResponse r = makePostRequest( "admin", "/payload_set/", params );
		if ( r.error ) {
			res.setMsg( "payload set via http(s) failed. Url: " + r.url + ", text: " + r.text + ", error: " + std::to_string(r.error) );
			return res;
		}

		res = payload->id();
		return res;
	}

	Result<std::string> PayloadAdapterHttp::deactivatePayload( const SPayloadPtr_t& payload, int64_t deactiveTime ) {
		Result <std::string> res;

		if ( !payload->ready() ) {
			res.setMsg( "payload is not ready to be stored to HTTP" );
			return res;
		}

		if ( !ensureMetadata() ) {
			res.setMsg( "http adapter cannot download metadata" );
			return res;
		}

		if ( !hasAccess("admin") ) {
			res.setMsg( "http adapter is not configured" );
			return res;
		}

		std::string id = payload->id();
		sanitize_alnumdash(id);

		if ( !id.size() || id != payload->id() ) {
			res.setMsg( "payload id is bad" );
			return res;
		}

		// get tag, fetch tbname
		auto tagit = mPaths.find( payload->directory() + "/" + payload->structName() );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "cannot find payload tag in the database: " + payload->directory() + "/" + payload->structName() );
			return res;
		}

		std::string tbname = tagit->second->tbname();

		HttpPostParams_t params{};
		params.push_back({ "id", id });
		params.push_back({ "tbname", tbname });
		params.push_back({ "dt", std::to_string(deactiveTime) });

		HttpResponse r = makePostRequest( "admin", "/payload_deactivate/", params );
		if ( r.error ) {
			res.setMsg( "payload set via http(s) failed. Url: " + r.url + ", text: " + r.text + ", error: " + std::to_string(r.error) );
			return res;
		}

		res = id;
		return res;
	}

	Result<SPayloadPtr_t> PayloadAdapterHttp::prepareUpload( const std::string& path ) {

		Result<SPayloadPtr_t> res;

		// check if Http Adapter is configured for writes]
		if ( !ensureMetadata() ) {
			res.setMsg( "http adapter cannot download metadata" );
			return res;
		}

		auto [ flavors, directory, structName, is_path_valid ] = Payload::decodePath( path );

		if ( !flavors.size() ) {
			res.setMsg( "no flavor provided, path: " + path );
			return res;
		}

		if ( !is_path_valid ) {
			res.setMsg( "bad path provided: " + path );
			return res;
		}

		// see if path exists in the known tags map
		auto tagit = mPaths.find( directory + "/" + structName );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "path does not exist in the db. path: " + directory + "/" + structName );
			return res;
		}

		// check if path is really a struct
		if ( !tagit->second->tbname().size() ) {
			res.setMsg( "cannot upload to the folder, need struct. path: " + path );
			return res;
		}

		SPayloadPtr_t p = std::make_shared<Payload>();

		p->setId( generate_uuid() );
		p->setPid( tagit->second->id() );
		p->setFlavor( flavors[0] );
		p->setDirectory( directory );
		p->setStructName( structName );
		p->setMode( tagit->second->mode() );

		res = p;

		return res;
	}

	Result<std::string> PayloadAdapterHttp::createTag( const STagPtr_t& tag ) {
		Result<std::string> res;

		if ( !ensureMetadata() ) {
			res.setMsg( "http adapter cannot download metadata" );
			return res;
		}

		if ( !hasAccess("admin") ) {
			res.setMsg( "http adapter is not configured" );
			return res;
		}

		HttpPostParams_t params{};
		params.push_back({ "op", "create" });
		params.push_back({ "id", tag->id() });
		params.push_back({ "pid", tag->pid() });
		params.push_back({ "name", tag->name() });
		params.push_back({ "tbname", tag->tbname() });
		params.push_back({ "ct", std::to_string(tag->ct()) });
		params.push_back({ "dt", std::to_string(tag->dt()) });
		params.push_back({ "mode", std::to_string(tag->mode()) });

		HttpResponse r = makePostRequest( "admin", "/tag/", params);

		if ( r.error ) {
			res.setMsg( "tag create via http(s) failed. Url: " + r.url + ", error: " + std::to_string(r.error) + ", text: " + r.text );
			return res;
		}

		res = tag->id();
		return res;
	}

	Result<std::string> PayloadAdapterHttp::createTag( const std::string& path, int64_t tag_mode ) {

		Result<std::string> res;

		if ( !ensureMetadata() ) {
			res.setMsg( "http adapter cannot download metadata" );
			return res;
		}

		if ( !hasAccess("admin") ) {
			res.setMsg( "http adapter is not configured" );
			return res;
		}

		std::string sanitized_path = path;
		sanitize_alnumslash( sanitized_path );

		if ( sanitized_path != path ) {
			res.setMsg( "path contains unwanted characters" );
			return res;
		}

		// check for existing tag
		auto tagit = mPaths.find( sanitized_path );
		if ( tagit != mPaths.end() ) {
			res.setMsg( "attempt to create an existing tag " + sanitized_path );
			return res;
		}

		std::string tag_id = uuid_from_str( sanitized_path ), tag_name = "", tag_tbname = "", tag_pid = "";
		if ( !tag_id.size() ) {
			res.setMsg( "new tag uuid generation failed" );
			return res;
		}

		long long tag_ct = time(0), tag_dt = 0;
		Tag* parent_tag = nullptr;

		std::vector<std::string> parts = explode( sanitized_path, '/' );
		tag_name = parts.back();
		parts.pop_back();
		sanitized_path = parts.size() ? implode( parts, "/" ) : "";

		if ( sanitized_path.size() ) {
			auto ptagit = mPaths.find( sanitized_path );
			if ( ptagit != mPaths.end() ) {
				parent_tag = (ptagit->second).get();
				tag_pid = parent_tag->id();
			} else {
				res.setMsg( "parent tag does not exist: " + sanitized_path );
				return res;
			}
		}

		if ( tag_mode > 0 ) {
			tag_tbname = ( parent_tag ? parent_tag->path() + "/" + tag_name : tag_name );
			std::replace( tag_tbname.begin(), tag_tbname.end(), '/', '_');
			string_to_lower_case( tag_tbname );
		}

		// do http request with:
		// tag_id, tag_name, tag_pid, tag_tbname, tag_ct, tag_dt, tag_mode

		HttpPostParams_t params{};
		params.push_back({ "op", "create" });
		params.push_back({ "id", tag_id });
		params.push_back({ "pid", tag_pid });
		params.push_back({ "name", tag_name });
		params.push_back({ "tbname", tag_tbname });
		params.push_back({ "ct", std::to_string(tag_ct) });
		params.push_back({ "dt", std::to_string(tag_dt) });
		params.push_back({ "mode", std::to_string(tag_mode) });

		HttpResponse r = makePostRequest( "admin", "/tag/", params);

		if ( r.error ) {
			res.setMsg( "tag create via http(s) failed. Url: " + r.url + ", error: " + std::to_string(r.error) + ", text: " + r.text );
			return res;
		}

		res = tag_id;

		return res;
	}

	Result<std::string> PayloadAdapterHttp::deactivateTag( const std::string& path, int64_t deactiveTime ) {
		Result<std::string> res;

		if ( !ensureMetadata() ) {
			res.setMsg( "http adapter cannot download metadata" );
			return res;
		}

		if ( !hasAccess("admin") ) {
			res.setMsg( "http adapter is not configured" );
			return res;
		}

		std::string sanitized_path = path;
		sanitize_alnumslash( sanitized_path );

		if ( sanitized_path != path ) {
			res.setMsg( "path contains unwanted characters" );
			return res;
		}

		// check for existing tag
		auto tagit = mPaths.find( sanitized_path );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "attempt to create an existing tag " + sanitized_path );
			return res;
		}

		std::string tag_id = tagit->second->id();
		// id, dt

		HttpPostParams_t params{};
		params.push_back({ "op", "deactivate" });
		params.push_back({ "id", tag_id });
		params.push_back({ "dt", std::to_string( deactiveTime ) });

		HttpResponse r = makePostRequest( "admin", "/tag/", params );
		if ( r.error ) {
			res.setMsg( "tag create via http(s) failed. Url: " + r.url + ", error: " + std::to_string(r.error) + ", text: " + r.text );
			return res;
		}

		res = tag_id;
		return res;
	}

	bool PayloadAdapterHttp::downloadMetadata() {

		if ( !hasAccess("get") ) {
			return false;
		}

		mTags.clear();
		mPaths.clear();

		HttpResponse r = makeGetRequest( "get", "/tags/" );
		if ( r.error ) {
			// download of metadata via http(s) failed :(
			return false;
		}

		nlohmann::json meta = nlohmann::json::parse( r.text.begin(), r.text.end(), nullptr, false, true );
		if ( meta.empty() || meta.is_discarded() || !meta.contains("tags") ) {
			return false;
		}

		for ( auto it : meta["tags"] ) {
			mTags.insert({ it["id"], std::make_shared<Tag>( it["id"], it["name"], it["pid"], it["tbname"], it["ct"], it["dt"], it["mode"], it["schema_id"].is_null() ? "" : it["schema_id"] ) });
		}

		if ( mTags.size() ) {
			// erase tags that have parent id but no parent exists => side-effect of tag deactivation and/or maxEntryTime
			// unfortunately, std::erase_if is C++20
			container_erase_if( mTags, [this]( const auto item ) {
					return ( item.second->pid().size() && mTags.find( item.second->pid() ) == mTags.end() );
					});
		}

		// parent-child: assign children and parents
		if ( mTags.size() ) {
			for ( auto& tag : mTags ) {
				if ( !(tag.second)->pid().size() ) { continue; }
				auto rc = mTags.find( ( tag.second )->pid() );
				if ( rc == mTags.end() ) { continue; }
				( tag.second )->setParent( rc->second );
				( rc->second )->addChild( tag.second );
			}
			// populate lookup map: path => Tag obj
			for ( const auto& tag : mTags ) {
				mPaths.insert({ ( tag.second )->path(), tag.second });
			}
		}

		// TODO: filter tags and paths based on maxEntryTime

		return true;
	}

	std::vector<std::string> PayloadAdapterHttp::getTags( bool skipStructs ) {

		if ( !ensureMetadata() ) {
			return std::vector<std::string>();
		}

		std::vector<std::string> tags{};
		tags.reserve( mPaths.size() );
		for ( const auto& [key, value] : mPaths ) {
			if ( value->mode() == 0 ) {
				const auto& children = value->children();
				if ( children.size() == 0 ) {
					tags.push_back( "[-] " + key );
				} else {
					bool has_folders = std::find_if( children.begin(), children.end(),
							[]( auto& child ) { return child.second->tbname().size() == 0; } ) != children.end();
					if ( has_folders ) {
						tags.push_back( "[+] " + key );
					} else {
						tags.push_back( "[-] " + key );
					}
				}
			} else if ( !skipStructs ) {
				std::string schema = "-";
				if (value->schema().size() ) { schema = "s"; }
				tags.push_back( "[s/" + schema + "/" + std::to_string( value->mode() ) + "] " + key );
			}
		}
		return tags;
	}

	Result<bool> PayloadAdapterHttp::setTagSchema( const std::string& tag_path, const std::string& schema_json ) {

		Result<bool> res;

		if ( !schema_json.size() ) {
			res.setMsg("empty tag schema provided");
			return res;
		}

		if ( !ensureMetadata() ) {
			res.setMsg("cannot download metadata");
			return res;
		}

		if ( !hasAccess("admin") ) {
			res.setMsg("http adapter is not configured");
			return res;
		}

		// make sure tag_path exists and is a struct
		std::string sanitized_path = tag_path;
		sanitize_alnumslash( sanitized_path );
		if ( sanitized_path != tag_path ) {
			res.setMsg( "sanitized tag path != input tag path... path: " + tag_path );
			return res;
		}

		std::string tag_pid = "";
		auto tagit = mPaths.find( sanitized_path );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "cannot find tag path in the database... path: " + sanitized_path );
			return res;
		}

		tag_pid = (tagit->second)->id();
		if ( tagit->second->mode() == 0 ) {
			res.setMsg( "tag is not a struct... path: " + sanitized_path );
			return res;
		}

		// make http request
		// id, pid, data, ct, dt
		HttpPostParams_t params{};
		params.push_back({ "op", "create" });
		params.push_back({ "id", generate_uuid() });
		params.push_back({ "pid", tag_pid });
		params.push_back({ "schema", base64::encode(schema_json) });
		params.push_back({ "ct", std::to_string(time(0)) });
		params.push_back({ "dt", std::to_string(0) });

		HttpResponse r = makePostRequest( "admin", "/schema/", params);
		if ( r.error ) {
			res.setMsg( "schema set via http(s) failed. Url: " + r.url + ", text: " + r.text + ", error: " + std::to_string(r.error) );
			return res;
		}

		nlohmann::json reply = nlohmann::json::parse( r.text.begin(), r.text.end(), nullptr, false, true );

		if ( reply.empty() || reply.is_discarded() || !reply.contains("id") ) {
			res.setMsg( "server replied with malformed data (not json): " + r.text );
			return res;
		}

		res = true;
		return res;
	}

	Result<std::string> PayloadAdapterHttp::getTagSchema( const std::string& tag_path ) {

		Result<std::string> res;

		if ( !ensureMetadata() ) {
			res.setMsg( "cannot download metadata" );
			return res;
		}

		if ( !hasAccess("get") ) {
			res.setMsg( "http adapter is not configured" );
			return res;
		}

		// make sure tag_path exists and is a struct
		std::string sanitized_path = tag_path;
		sanitize_alnumslash( sanitized_path );
		if ( sanitized_path != tag_path ) {
			res.setMsg( "sanitized tag path != input tag path... path: " + sanitized_path );
			return res;
		}
		std::string tag_pid = "";
		auto tagit = mPaths.find( sanitized_path );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "cannot find tag path in the database... path: " + sanitized_path );
			return res;
		}
		tag_pid = (tagit->second)->id();

		if ( tagit->second->mode() == 0 ) {
			res.setMsg( "tag is not a struct... path: " + sanitized_path );
			return res;
		}

		HttpResponse r = makeGetRequest( "get", "/schema/?id=" + tag_pid );
		if ( r.error ) {
			res.setMsg( "schema get via http(s) failed. Url: " + r.url + ", text: " + r.text + ", error: " + std::to_string(r.error) );
			return res;
		}

		nlohmann::json reply = nlohmann::json::parse( r.text.begin(), r.text.end(), nullptr, false, false );

		if ( reply.empty() || reply.is_discarded() || !reply.contains("schema") ) {
			res.setMsg( "server replied with malformed data (not json): " + r.text );
			return res;
		}
		res = reply["schema"];

		return res;
	}

	Result<bool> PayloadAdapterHttp::dropTagSchema( const std::string& tag_path ) {
		Result<bool> res;

		if ( !ensureMetadata() ) {
			res.setMsg("cannot download metadata");
			return res;
		}

		if ( !hasAccess("admin") ) {
			res.setMsg("http adapter is not configured");
			return res;
		}

		// make sure tag_path exists and is a struct
		std::string sanitized_path = tag_path;
		sanitize_alnumslash( sanitized_path );
		if ( sanitized_path != tag_path ) {
			res.setMsg( "sanitized tag path != input tag path... path: " + tag_path );
			return res;
		}

		std::string tag_pid = "";
		auto tagit = mPaths.find( sanitized_path );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "cannot find tag path in the database... path: " + sanitized_path );
			return res;
		}

		tag_pid = (tagit->second)->id();
		if ( tagit->second->mode() == 0 ) {
			res.setMsg( "tag is not a struct... path: " + sanitized_path );
			return res;
		}

		// make http request
		// id, pid, data, ct, dt
		HttpPostParams_t params{};
		params.push_back({ "op", "drop" });
		params.push_back({ "pid", tag_pid });

		HttpResponse r = makePostRequest( "admin", "/schema/", params);
		if ( r.error ) {
			res.setMsg( "schema drop via http(s) failed. Url: " + r.url + ", text: " + r.text + ", error: " + std::to_string(r.error) );
			return res;
		}

		nlohmann::json reply = nlohmann::json::parse( r.text.begin(), r.text.end(), nullptr, false, true );

		if ( reply.empty() || reply.is_discarded() || !reply.contains("id") ) {
			res.setMsg( "server replied with malformed data (not json): " + r.text );
			return res;
		}

		res = true;
		return res;
	}

	Result<bool> PayloadAdapterHttp::createDatabaseTables() {
		Result<bool> res;

		HttpResponse r = makePostRequest( "admin", "/tables/", { {"op", "create"} } );
		if ( r.error ) {
			res.setMsg( "create database tables via http(s) failed. Url: " + r.url + ", text: " + r.text + ", error: " + std::to_string(r.error) );
			return res;
		}

		nlohmann::json reply = nlohmann::json::parse( r.text.begin(), r.text.end(), nullptr, false, false );

		if ( reply.empty() || reply.is_discarded() || !reply.contains("success") ) {
			res.setMsg( "server replied with malformed data (not json): " + r.text );
			return res;
		}

		res = reply["success"].get<bool>();
		return res;
	}

	Result<bool> PayloadAdapterHttp::dropDatabaseTables() {
		Result<bool> res;

		HttpResponse r = makePostRequest( "admin", "/tables/", { {"op","drop"} });
		if ( r.error ) {
			res.setMsg( "drop database tables via http(s) failed. Url: " + r.url + ", text: " + r.text + ", error: " + std::to_string(r.error) );
			return res;
		}

		nlohmann::json reply = nlohmann::json::parse( r.text.begin(), r.text.end(), nullptr, false, false );

		if ( reply.empty() || reply.is_discarded() || !reply.contains("success") ) {
			res.setMsg( "server replied with malformed data (not json): " + r.text );
			return res;
		}

		res = reply["success"].get<bool>();
		return res;
	}

	std::vector<std::string> PayloadAdapterHttp::listDatabaseTables() {
		std::vector<std::string> tables;

		HttpResponse r = makePostRequest( "admin", "/tables/", { {"op","list"} });
		if ( r.error ) {
			return tables;
		}

		nlohmann::json reply = nlohmann::json::parse( r.text.begin(), r.text.end(), nullptr, false, false );

		if ( reply.empty() || reply.is_discarded() || !reply.contains("tables") ) {
			return tables;
		}

		tables = reply["tables"].get<std::vector<std::string>>();
		return tables;
	}

	Result<std::string> PayloadAdapterHttp::downloadData( const std::string& uri ) {
		Result<std::string> res;

		if ( !uri.size() ) {
			res.setMsg( "empty uri" );
			return res;
		}

		auto parts = explode( uri, "://" );
		string_to_lower_case( parts[0] );
		sanitize_alnum( parts[0] );
		if ( parts.size() != 2 || !(parts[0] == "http" || parts[0] == "https" ) ) {
			res.setMsg("bad uri");
			return res;
		}

		std::string token = generateJWT( "get", 0 );
		mHttpClient->setToken( token.size() ? token : "" );
		HttpResponse r = mHttpClient->Get( uri );
		if ( r.error ) {
			res.setMsg( "download of data via http(s) failed. Url: " + r.url + ", error: " + std::to_string(r.error) );
			return res;
		}

		res = r.text;
		return res;
	}

	std::string PayloadAdapterHttp::generateJWT( const std::string& access, uint64_t idx ) {

		std::string user, pass;

		try {
			user = mConfig["adapters"]["http"][access][idx]["user"];
			pass = mConfig["adapters"]["http"][access][idx]["pass"];
		} catch( nlohmann::json::exception& e ) {
			return "";
		} catch(...) {
			return "";
		}

		if ( !user.size() || !pass.size() ) {
			return "";
		}

		jwt::jwt_object obj{ jwt::params::algorithm("HS256"), jwt::params::payload({}), jwt::params::secret(pass)};
		int64_t tm = time(0);

		uint64_t expirationSeconds = 10;
		if ( mConfig["adapters"]["http"]["config"].contains("jwt_expiration_seconds") ) {
			expirationSeconds = mConfig["adapters"]["http"]["config"]["jwt_expiration_seconds"].get<int64_t>();
		}

		obj.add_claim( "iat", tm )
			.add_claim("exp", tm + expirationSeconds )
			.add_claim("iss", user )
			.add_claim("jti", generate_uuid() )
			.add_claim("sub", access );

		return obj.signature();
	}

	void PayloadAdapterHttp::setHttpConfig() {

		if ( mConfig.empty() || !mConfig.contains("adapters") || !mConfig["adapters"].contains("http") 
				|| !mConfig["adapters"]["http"].contains("config") ) {
			return;
		}
		if ( mConfig["adapters"]["http"]["config"].contains("max_retries") ) {
			mHttpClient->setMaxRetries( mConfig["adapters"]["http"]["config"]["max_retries"] );
		}
		if ( mConfig["adapters"]["http"]["config"].contains("sleep_seconds") ) {
			mHttpClient->setSleepSeconds( mConfig["adapters"]["http"]["config"]["sleep_seconds"] );
		}
		if ( mConfig["adapters"]["http"]["config"].contains("verbose") ) {
			mHttpClient->setVerbose( mConfig["adapters"]["http"]["config"]["verbose"] );
		}
		if ( mConfig["adapters"]["http"]["config"].contains("timeout_ms") ) {
			mHttpClient->setTimeout( mConfig["adapters"]["http"]["config"]["timeout_ms"] );
		}
		if ( mConfig["adapters"]["http"]["config"].contains("connect_timeout_ms") ) {
			mHttpClient->setConnectTimeout( mConfig["adapters"]["http"]["config"]["connect_timeout_ms"] );
		}
		if ( mConfig["adapters"]["http"]["config"].contains("user_agent") ) {
			mHttpClient->setUserAgent( mConfig["adapters"]["http"]["config"]["user_agent"] );
		}
	}

	HttpResponse PayloadAdapterHttp::makeGetRequest( const std::string& access, const std::string& url ) {
		setHttpConfig();
		size_t idx = RngS::Instance().random_inclusive<size_t>( 0, mConfig["adapters"]["http"][ access ].size() - 1 );
		std::string token = generateJWT( access, idx );
		mHttpClient->setToken( token.size() ? token : "" );
		return mHttpClient->Get( mConfig["adapters"]["http"][access][idx]["url"].get<std::string>() + url );
	}

	HttpResponse PayloadAdapterHttp::makePostRequest( const std::string& access, const std::string& url, const HttpPostParams_t& params ) {
		setHttpConfig();
		size_t idx = RngS::Instance().random_inclusive<size_t>( 0, mConfig["adapters"]["http"][ access ].size() - 1 );
		std::string token = generateJWT( access, idx );
		mHttpClient->setToken( token.size() ? token : "" );
		return mHttpClient->Post( mConfig["adapters"]["http"][access][idx]["url"].get<std::string>() + url, params );
	}

	Result<std::string> PayloadAdapterHttp::exportTagsSchemas( bool tags, bool schemas ) {
		Result<std::string> res;

		if ( !ensureMetadata() ) {
			res.setMsg("cannot download metadata");
			return res;
		}
		if ( !mTags.size() || !mPaths.size() ) {
			res.setMsg("no tags, cannot export");
			return res;
		}

		if ( !tags && !schemas ) {
			res.setMsg("neither tags nor schemas were requested for export");
			return res;
		}

		nlohmann::json output;
		output["export_type"] = "cdbnpp_tags_schemas";
		output["export_timestamp"] = std::time(nullptr);
		if ( tags ) {
			output["tags"] = nlohmann::json::array();
		}
		if ( schemas ) {
			output["schemas"] = nlohmann::json::array();
		}

		for ( const auto& [ key, value ] : mPaths ) {
			if ( tags ) {
				output["tags"].push_back( value->toJson() );
			}
			if ( schemas && value->schema().size() ) {
				Result<std::string> schema_str = getTagSchema( key );
				if ( schema_str.valid() ) {
					output["schemas"].push_back({
							{ "id",value->schema() }, { "pid", value->id() }, { "path", value->path() }, { "data", schema_str.get() }
						});
				}
			}
		}

		res = output.dump(2);
		return res;
	}

	Result<bool> PayloadAdapterHttp::importTagsSchemas(const std::string& stringified_json ) {
		Result<bool> res;

		if ( !stringified_json.size() ) {
			res.setMsg("empty tag/schema data provided");
			return res;
		}

		nlohmann::json data = nlohmann::json::parse( stringified_json.begin(), stringified_json.end(), nullptr, false, true );

		if ( data.empty() || data.is_discarded() || data.is_null() ) {
			res.setMsg( "bad input data" );
			return res;
		}

		bool tag_import_errors = false, schema_import_errors = false;
		if ( data.contains("tags") ) {
			for ( const auto& [ key, tag ] : data["tags"].items() ) {
				STagPtr_t t = std::make_shared<Tag>(
						tag["id"].get<std::string>(),
						tag["name"].get<std::string>(),
						tag["pid"].get<std::string>(),
						tag["tbname"].get<std::string>(),
						tag["ct"].get<uint64_t>(),
						tag["dt"].get<uint64_t>(),
						tag["mode"].get<uint64_t>(),
						""
						);
				Result<std::string> rc = createTag( t );
				if ( rc.invalid() ) { tag_import_errors = true; }
			}
		}

		if ( data.contains("schemas") ) {
			for ( const auto& [ key, schema ] : data["schemas"].items() ) {
				Result<bool> rc = setTagSchema(
						schema["path"].get<std::string>(),
						schema["data"].get<std::string>()
						);
				if ( rc.invalid() ) { schema_import_errors = true; }
			}
		}

		res = !tag_import_errors && !schema_import_errors;
		return res;
	}

} // namespace CDBNPP
