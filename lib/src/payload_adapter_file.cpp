
#include "cdbnpp/payload_adapter_file.h"

#include <filesystem>
#include <fstream>

#include "cdbnpp/json_schema.h"
#include "cdbnpp/log.h"
#include "cdbnpp/util.h"
#include "cdbnpp/uuid.h"

namespace CDBNPP {

	PayloadAdapterFile::PayloadAdapterFile() : IPayloadAdapter("file") {}

	PayloadResults_t PayloadAdapterFile::getPayloads( const std::set<std::string>& paths, const std::vector<std::string>& flavors,
			const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime, int64_t eventTime, int64_t run, int64_t seq ) {
		PayloadResults_t res;

		std::string dir = std::filesystem::current_path().string()
			+ "/" + config().value("dirname",".CDBNPP") + "/";

		std::set<std::string> unfolded_paths{};
		for ( const auto& path : paths ) {
			std::vector<std::string> parts = explode( path, ":" );
			std::string flavor = parts.size() == 2 ? parts[0] : "";
			std::string unflavored_path = parts.size() == 2 ? parts[1] : parts[0];
			if ( !std::filesystem::is_directory( dir + unflavored_path ) ) {
				for ( auto const& dir_entry : std::filesystem::recursive_directory_iterator( dir ) ) {
					if ( !dir_entry.is_directory() ) { continue; }
					if ( string_starts_with( dir_entry.path().string(), dir + unflavored_path ) ) {
						unfolded_paths.insert( ( flavor.size() ? (flavor + ":") : "" ) + dir_entry.path().string().substr( dir.size() ) );
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

	Result<SPayloadPtr_t> PayloadAdapterFile::getPayload( const std::string& path, const std::vector<std::string>& service_flavors,
			const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime, int64_t eventTime, int64_t eventRun, int64_t eventSeq ) {
		Result<SPayloadPtr_t> res;

		auto [ flavors, directory, structName, is_path_valid ] = Payload::decodePath( path );

		if ( !is_path_valid ) {
			res.setMsg( "request path has not been decoded, path: " + path );
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

		std::string dir = std::filesystem::current_path().string()
			+ "/"	+ config().value("dirname",".CDBNPP") + "/" + directory;

		if ( !std::filesystem::exists( dir ) ) {
			res.setMsg( "directory does not exist: " + dir );
			return res;
		}

		if ( !std::filesystem::exists( dir + "/" + structName ) ) {
			res.setMsg( "struct directory does not exist: " + dir + "/" + structName );
			return res;
		}

		if ( !service_flavors.size() && !flavors.size() ) {
			res.setMsg( "request does not specify flavor, path: " + path );
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

		for ( const auto& flavor : ( flavors.size() ? flavors : service_flavors ) ) {
			// iterate over fs files, find and return one that fits
			for ( const auto& dir_entry : std::filesystem::directory_iterator{ dir + "/" + structName } ) {
				if ( !dir_entry.is_regular_file() ) { continue; }
				auto [
					file_flavor,
					file_ct, file_bt,	file_et, file_dt,
					file_run,	file_seq,
					file_is_binary,	file_is_valid
				] = decodeFilename( dir_entry.path().filename().string() );

				// skip if filename was not properly decoded
				if ( !file_is_valid ) { continue; }

				// check if decoded flavor does not match
				if ( flavor != file_flavor ) { continue; }

				// skip if createTime > maxEntryTime or deactiveTime is set and is < maxEntryTime
				if ( maxEntryTime > 0 && ( file_ct > maxEntryTime || ( file_dt != 0 && file_dt < maxEntryTime ) ) ) { continue; } 

				// check for matching run, seq
				if ( eventRun != 0 && file_run != 0 && ( file_run != eventRun || file_seq != eventSeq ) ) { continue; }

				// check for matching beginTime, endTime
				if ( eventTime != 0 && ( file_bt > eventTime || ( file_et != 0 && file_et <= eventTime ) ) ) { continue; }

				std::string fmt = dir_entry.path().extension();
				string_to_lower_case( fmt );

				// ok, this payload matches
				SPayloadPtr_t pld = std::make_shared<Payload>(
						generate_uuid(), uuid_from_str( directory ),
						file_flavor, structName, directory,
						file_ct, file_bt,	file_et, file_dt, file_run, file_seq
						);

				pld->setURI( std::string("file://") + dir_entry.path().string() );

				if ( eventRun != 0 && file_run != 0 && file_run == eventRun && file_seq == eventSeq ) {
					res = pld;
					return res; // found by exact match of run, seq => return immediately
				} else {
					if ( res.invalid() ) {
						res = pld; // found our first match by eventTime, let's save it until the end
					} else if ( ( eventTime - pld->beginTime() ) < ( eventTime - res.get()->beginTime() ) ) {
						res = pld; // payload was found by an approximate match, scan until the end
					} else {
						// finally, silently discard newly contstructed payload, it is worse than the one previously found
					}
				}
			} // directory loop

			if ( res.valid() ) {
				return res;
			}
		} // flavors loop

		return res;
	}

	Result<std::string> PayloadAdapterFile::setPayload( const SPayloadPtr_t& payload ) {
		Result<std::string> res;

		if ( !payload->ready() ) {
			res.setMsg( "payload is not ready to be stored" );
			return res;
		}

		if ( payload->beginTime() != 0 && payload->run() != 0 ) {
			res.setMsg( "payload contains both beginTime and run, NOTE: api will use beginTime for payload::get");
		}

		std::string path = std::filesystem::current_path().string()
			+ "/"	+ config().value("dirname",".CDBNPP") + "/";

		std::string directory = payload->directory();
		trim( directory, "/ \n\r\t\v" );
		sanitize_alnumslash( directory );
		path += directory;
		path += "/" + payload->structName();

		std::string schema_path = std::filesystem::current_path().string()
			+ "/" + config().value("dirname",".CDBNPP") + "/.schemas";

		std::string schema_name = payload->directory() + "/" + payload->structName() + ".json";
		std::replace( schema_name.begin(), schema_name.end(), '/', '_');
		string_to_lower_case( schema_name );

		std::string schema_data = file_get_contents( schema_path + "/" + schema_name );

		if ( schema_data.size() ) {
			// now, validate payload data vs schema
			bool rc = validate_json_using_schema( payload->data(), schema_data );
			if ( !rc ) {
				res.setMsg( "schema was found for " + payload->URI() + ", but schema validation failed" );
				return res;
			}
		}

		// create directories if not exist
		if ( !std::filesystem::exists( path ) ) {
			if ( !std::filesystem::create_directories( path ) ) {
				res.setMsg( "cannot create directory = " + path );
				return res; // cannot create directory
			}
		}

		// initialize create time if not set
		if ( payload->createTime() == 0 ) { payload->setCreateTime( time(NULL) ); }

		// construct full file name
		// format: <structName>.<flavor>.c<time>_b<time>_e<time>_d<time>.dat

		std::string filename = path
			+ "/"	+ sanitize_alnum( payload->flavor() ) + ".";

		std::vector<std::string> chunks;

		if ( payload->createTime() != 0 ) {
			chunks.push_back( "c"+unixtime_to_utc_date( payload->createTime(), "%Y%m%dT%H%M%S" ) );
		}

		if ( payload->beginTime() != 0 ) {
			chunks.push_back( "b"+unixtime_to_utc_date( payload->beginTime(), "%Y%m%dT%H%M%S" ) );
		}

		if ( payload->endTime() != 0 ) {
			chunks.push_back( "e"+unixtime_to_utc_date( payload->endTime(), "%Y%m%dT%H%M%S" ) );
		}

		if ( payload->deactiveTime() != 0 ) {
			chunks.push_back( "d"+unixtime_to_utc_date( payload->deactiveTime(), "%Y%m%dT%H%M%S" ) );
		}

		if ( payload->run() != 0 ) {
			chunks.push_back( "r"+std::to_string(payload->run()) );
		}

		if ( payload->seq() != 0 ) {
			chunks.push_back( "s"+std::to_string(payload->seq()) );
		}

		filename += implode( chunks, "_" );
		filename += "." + payload->format();

		// store payload contents to disk
		std::ofstream ofs( filename );
		if ( !ofs.is_open() ) {
			res.setMsg( "file cannot be opened = " + filename );
			return res;
		}
		ofs << payload->data();
		ofs.close();

		res = payload->id();
		return res;
	}

	Result<std::string> PayloadAdapterFile::deactivatePayload( __attribute__((unused)) const SPayloadPtr_t& payload, __attribute__((unused)) int64_t deactiveTime ) {
		Result<std::string> res;
		res.setMsg( "File adapter cannot deactivate payloads" );
		return res;
	}

	Result<SPayloadPtr_t> PayloadAdapterFile::prepareUpload( const std::string& path ) {
		Result<SPayloadPtr_t> res;

		auto [ flavors, directory, structName, is_path_valid ] = Payload::decodePath( path );

		if ( !flavors.size() ) {
			res.setMsg( "no flavor provided: " + path );
			return res;
		}

		if ( !is_path_valid ) {
			res.setMsg( "bad path provided: " + path );
			return res;
		}

		if ( !flavors.size() ) {
			res.setMsg( "prepare path does not contain flavor" );
			return res;
		}

		std::string root_path = std::filesystem::current_path().string()
			+ "/"	+ config().value("dirname",".CDBNPP");

		if ( !std::filesystem::exists( root_path + "/" + directory ) ) {
			res.setMsg( "directory does not exist, cannot prepare for upload: " + directory );
			return res;
		}

		std::string complete_path = root_path + "/" + directory + "/" + structName;
		if ( !std::filesystem::exists( complete_path ) ) {
			if ( !std::filesystem::create_directories( complete_path ) ) {
				res.setMsg( "cannot create directory = " + complete_path );
				return res;
			}
		}

		SPayloadPtr_t p = std::make_shared<Payload>();

		p->setId( generate_uuid() );
		p->setPid( uuid_from_str( directory + "/" + structName ) );
		p->setFlavor( flavors[0] );
		p->setDirectory( directory );
		p->setStructName( structName );
		p->setMode( 2 );
		res = p;

		return res;
	}

	Result<std::string> PayloadAdapterFile::createTag( __attribute__ ((unused)) const STagPtr_t& tag ) {
		Result<std::string> res;
		res.setMsg("cannot create tag by ref");
		return res;
	}

	Result<std::string> PayloadAdapterFile::createTag( const std::string& path, int64_t tag_mode ) {
		Result<std::string> res;

		std::string sanitized_path = path;
		trim( sanitized_path );
		sanitize_alnumslash( sanitized_path );
		if ( path != sanitized_path ) {
			res.setMsg( "tag path contains bad characters: " + path );
			return res;
		}

		std::string root_path = std::filesystem::current_path().string()
			+ "/"	+ config().value("dirname",".CDBNPP");

		if ( std::filesystem::exists( root_path + "/" + sanitized_path ) ) {
			res = uuid_from_str( sanitized_path );
			return res;
		}

		std::string new_path = root_path + "/" + sanitized_path;
		if ( !std::filesystem::create_directories( new_path ) ) {
			res.setMsg( "cannot create directory = " + new_path );
			return res;
		}
		res = uuid_from_str( sanitized_path );
		return res;
	}

	Result<std::string> PayloadAdapterFile::deactivateTag( __attribute__ ((unused)) const std::string& path, __attribute__ ((unused)) int64_t deactiveTime ) {
		Result<std::string> res;
		res.setMsg( "File adapter cannot deactivate tags" );
		return res;
	}

	Result<std::string> PayloadAdapterFile::downloadData( const std::string& uri ) {
		Result<std::string> res;
		if ( !uri.size() ) {
			res.setMsg( "empty uri" );
			return res;
		}

		auto parts = explode( uri, "://" );
		if ( parts.size() != 2 || parts[0] != "file" ) {
			res.setMsg( "bad uri: " + uri );
			return res;
		}

		res = file_get_contents( parts[1] );
		return res;
	}

	DecodedFileNameTuple PayloadAdapterFile::decodeFilename( const std::string& filename ) {
		// file format: <flavor>.c<datetime>_b<datetime>_e<datetime>_d<datetime>_r<runnumber>.dat
		std::string flavor;
		int64_t createTime = 0, beginTime = 0, endTime = 0, deactiveTime = 0;
		int64_t run = 0, seq = 0, is_binary = 0;

		if ( !filename.size() ) {
			// empty filename provided => return empty tuple
			return std::make_tuple( flavor, createTime, beginTime, endTime, deactiveTime, run, seq, is_binary, false );
		}

		// get structName, flavor, timestamps, extension
		std::vector<std::string> dparts = explode( filename, '.' );

		// remove extension for the list
		std::string extension = dparts.back();
		dparts.pop_back();
		string_to_lower_case( extension );

		if ( dparts.size() < 2 ) {
			// filename is not formatted properly => return empty tuple
			return std::make_tuple( flavor, createTime, beginTime, endTime, deactiveTime, run, seq, is_binary, false );
		}

		flavor = dparts[0];

		// parse timestamps and run number
		std::vector<std::string> parts = explode( dparts[1], '_' );

		if ( parts.size() > 0 ) {
			for ( const auto& part : parts ) {
				if ( part[0] == 'c' ) {
					createTime = string_to_time( part.substr(1) );
				} else if ( part[0] == 'b' ) {
					beginTime = string_to_time( part.substr(1) );
				} else if ( part[0] == 'e' ) {
					endTime = string_to_time( part.substr(1) );
				} else if ( part[0] == 'd' ) {
					deactiveTime = string_to_time( part.substr(1) );
				} else if ( part[0] == 'r' ) {
					run = string_to_longlong( part.substr(1) );
				} else if ( part[0] == 's' ) {
					seq = string_to_longlong( part.substr(1) );
				}
			}
		}

		// check extension: .txt | .json => non-binary, .dat and others => binary
		std::vector<std::string> ext{"txt", "json", "xml", "asc", "md"};
		if ( std::find( ext.begin(), ext.end(), extension ) != ext.end() ) {
			is_binary = 0;
		} else {
			is_binary = 1;
		}

		return std::make_tuple( flavor, createTime, beginTime, endTime, deactiveTime, run, seq, is_binary, true );
	}

	Result<std::string> PayloadAdapterFile::getTagSchema( const std::string& tag_path ) {
		Result<std::string> res;

		std::string schema_path = std::filesystem::current_path().string()
			+ "/" + config().value("dirname",".CDBNPP") + "/.schemas";

		trim( tag_path, "/ \n\r\t\v" );
		std::string sanitized_tag_path = sanitize_alnumslash( tag_path );

		std::string schema_name = sanitized_tag_path + ".json";
		std::replace( schema_name.begin(), schema_name.end(), '/', '_');
		string_to_lower_case( schema_name );

		std::string schema_json = file_get_contents( schema_path + "/" + schema_name );
		if ( !schema_json.size() ) {
			res.setMsg( "cannot find schema" );
			return res;
		}

		res = schema_json;
		return res;
	}

	Result<bool> PayloadAdapterFile::setTagSchema( const std::string& tag_path, const std::string& schema_json ) {
		Result<bool> res;

		std::string schema_path = std::filesystem::current_path().string()
			+ "/" + config().value("dirname",".CDBNPP") + "/.schemas";

		if ( !std::filesystem::exists( schema_path ) ) {
			if ( !std::filesystem::create_directories( schema_path ) ) {
				res.setMsg( "cannot create directory = " + schema_path );
				return res;
			}
		}

		trim( tag_path, "/ \n\r\t\v" );
		std::string sanitized_tag_path = sanitize_alnumslash( tag_path );

		std::string schema_name = sanitized_tag_path + ".json";
		std::replace( schema_name.begin(), schema_name.end(), '/', '_');
		string_to_lower_case( schema_name );

		if ( !file_put_contents( schema_path + "/" + schema_name, schema_json ) ) {
			res.setMsg("cannot save schema");
			return res;
		}

		res = true;
		return res;
	}

	Result<bool> PayloadAdapterFile::dropTagSchema( const std::string& tag_path ) {
		Result<bool> res;

		std::string schema_path = std::filesystem::current_path().string()
			+ "/" + config().value("dirname",".CDBNPP") + "/.schemas";

		trim( tag_path, "/ \n\r\t\v" );
		std::string sanitized_tag_path = sanitize_alnumslash( tag_path );

		std::string schema_name = sanitized_tag_path + ".json";
		std::replace( schema_name.begin(), schema_name.end(), '/', '_');
		string_to_lower_case( schema_name );

		std::error_code ec; // helps avoid throwing an exception
		if ( !std::filesystem::remove( schema_path + "/" + schema_name, ec ) ) {
			res.setMsg( "schema did not exist" );
			ec.clear();
			return res;
		}

		res = true;
		return res;
	}

	Result<std::string> PayloadAdapterFile::exportTagsSchemas( __attribute__ ((unused)) bool tags, __attribute__ ((unused)) bool schemas ) {
		Result<std::string> res;
		res.setMsg("file adapter cannot export tags and schemas");
		return res;
	}

	Result<bool> PayloadAdapterFile::importTagsSchemas(const std::string& stringified_json ) {
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

		std::string root_path = std::filesystem::current_path().string() + "/" + config().value("dirname",".CDBNPP"),
			schema_path = root_path + "/.schemas";

		if ( data.contains("tags") ) {
			for ( const auto& [ key, tag ] : data["tags"].items() ) {
				if ( tag["mode"].get<uint64_t>() != 0 ) { continue; } // skip structs
				std::string tag_path = tag["path"].get<std::string>();
				sanitize_alnumslash(tag_path);
				trim( tag_path, "/ \n\r\t\v" );
				if ( !std::filesystem::exists( tag_path ) ) {
					std::filesystem::create_directories( root_path + "/" + tag_path );
				}
			}
		}

		if ( data.contains("schemas") ) {
			for ( const auto& [ key, schema ] : data["schemas"].items() ) {
				std::string tag_path = schema["path"].get<std::string>();
				sanitize_alnumslash( tag_path );
				trim( tag_path, "/ \n\r\t\v" );
				std::string schema_name = tag_path + ".json";
				std::replace( schema_name.begin(), schema_name.end(), '/', '_');
				string_to_lower_case( schema_name );
				file_put_contents( schema_path + "/" + schema_name, schema["data"].get<std::string>() );
			}
		}

		return res;
	}

} // namespace CDBNPP
