
#include "cdbnpp/payload_adapter_db.h"

#include "cdbnpp/base64.h"
#include "cdbnpp/json_schema.h"
#include "cdbnpp/http_client.h"
#include "cdbnpp/log.h"
#include "cdbnpp/rng.h"
#include "cdbnpp/util.h"
#include "cdbnpp/uuid.h"

namespace CDBNPP {

	using namespace soci;

	PayloadAdapterDb::PayloadAdapterDb() : IPayloadAdapter("db") {}

	PayloadResults_t PayloadAdapterDb::getPayloads( const std::set<std::string>& paths, const std::vector<std::string>& flavors,
			const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime, int64_t eventTime, int64_t run, int64_t seq ) {
		PayloadResults_t res;
		for ( const auto& path : paths ) {
			Result<SPayloadPtr_t> rc = getPayload( path, flavors, maxEntryTimeOverrides, maxEntryTime, eventTime, run, seq );
			if ( rc.valid() ) {
				res.insert({ path, rc.get() });
			}
		}
		return res;
	}

	Result<SPayloadPtr_t> PayloadAdapterDb::getPayload( const std::string& path, const std::vector<std::string>& service_flavors,
			const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime, int64_t eventTime, int64_t eventRun, int64_t eventSeq ) {

		Result<SPayloadPtr_t> res;

		if ( !ensureMetadata() ) {
			res.setMsg("cannot get metadata");
			return res;
		}

		if ( !setAccessMode("get") ) {
			res.setMsg( "cannot switch to GET mode");
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg("cannot ensure database connection");
			return res;
		}

		auto [ flavors, directory, structName, is_path_valid ] = Payload::decodePath( path );

		if ( !is_path_valid ) {
			res.setMsg( "request path has not been decoded, path: " + path );
			return res;
		}

		if ( !service_flavors.size() && !flavors.size() ) {
			res.setMsg( "no flavor specified, path: " + path );
			return res;
		}

		if ( !directory.size() ) {
			res.setMsg( "request does not specify directory, path: " + path );
			return res;
		}

		if ( !structName.size() ) {
			res.setMsg( "request does not specify structName, path: " + path );
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
			res.setMsg( "cannot find tag for the " + dirpath );
			return res;
		}

		// get tbname from tag
		std::string tbname = tagit->second->tbname(), pid = tagit->second->id();
		if ( !tbname.size() ) {
			res.setMsg( "requested path points to the directory, need struct: " + dirpath );
			return res;
		}

		for ( const auto& flavor : ( flavors.size() ? flavors : service_flavors ) ) {

			std::string id{""}, uri{""}, fmt{""};
			uint64_t bt = 0, et = 0, ct = 0, dt = 0, run = 0, seq = 0, mode = tagit->second->mode();

			if ( mode == 2 ) {
				// fetch id, run, seq
				try {
					std::string query = "SELECT id, uri, bt, et, ct, dt, run, seq, fmt FROM cdb_iov_" + tbname + " "
						+ "WHERE "
						+ "flavor = :flavor "
						+ "AND run = :run "
						+ "AND seq = :seq "
						+ ( maxEntryTime ? "AND ct <= :mt " : "" )
						+ ( maxEntryTime ? "AND ( dt = 0 OR dt > :mt ) " : "" )
						+ "ORDER BY ct DESC LIMIT 1";

					if ( maxEntryTime ) {
						mSession->once << query, into(id), into(uri), into(bt), into(et), into(ct), into(dt), into(run), into(seq), into(fmt),
							use( flavor, "flavor"), use( eventRun, "run" ), use( eventSeq, "seq" ), use( maxEntryTime, "mt" );
					} else {
						mSession->once << query, into(id), into(uri), into(bt), into(et), into(ct), into(dt), into(run), into(seq), into(fmt),
							use( flavor, "flavor"), use( eventRun, "run" ), use( eventSeq, "seq" );
					}

				} catch( std::exception const & e ) {
					res.setMsg( "database exception: " + std::string(e.what()) );
					return res;
				}

				if ( !id.size() ) { continue; } // nothing found

			} else if ( mode == 1 ) {
				// fetch id, bt, et

				try {
					std::string query = "SELECT id, uri, bt, et, ct, dt, run, seq, fmt FROM cdb_iov_" + tbname + " "
						+ "WHERE "
						+ "flavor = :flavor "
						+ "AND bt <= :et AND ( et = 0 OR et > :et ) "
						+ ( maxEntryTime ? "AND ct <= :mt " : "" )
						+ ( maxEntryTime ? "AND ( dt = 0 OR dt > :mt ) " : "" )
						+ "ORDER BY bt DESC LIMIT 1";
					if ( maxEntryTime ) {
						mSession->once << query, into(id), into(uri), into(bt), into(et), into(ct), into(dt), into(run), into(seq), into(fmt),
							use( flavor, "flavor"), use( eventTime, "et" ), use( maxEntryTime, "mt" );
					} else {
						mSession->once << query, into(id), into(uri), into(bt), into(et), into(ct), into(dt), into(run), into(seq), into(fmt),
							use( flavor, "flavor"), use( eventTime, "et" );
					}
				} catch( std::exception const & e ) {
					res.setMsg( "database exception: " + std::string(e.what()) );
					return res;
				}

				if ( !id.size() ) { continue; } // nothing found

				if ( et == 0 ) {
					// if no endTime, do another query to establish endTime
					try {
						std::string query = "SELECT bt FROM cdb_iov_" + tbname + " "
							+ "WHERE "
							+ "flavor = :flavor "
							+ "AND bt >= :et "
							+ "AND ( et = 0 OR et < :et )"
							+ ( maxEntryTime ? "AND ct <= :mt " : "" )
							+ ( maxEntryTime ? "AND ( dt = 0 OR dt > :mt ) " : "" )
							+ "ORDER BY bt ASC LIMIT 1";
						if ( maxEntryTime ) {
							mSession->once << query, into(et),
								use( flavor, "flavor"), use( eventTime, "et" ), use( maxEntryTime, "mt" );
						} else {
							mSession->once << query, into(et),
								use( flavor, "flavor"), use( eventTime, "et" );
						}
					} catch( std::exception const & e ) {
						res.setMsg( "database exception: " + std::string(e.what()) );
						return res;
					}
					if ( !et ) {
						et = std::numeric_limits<uint64_t>::max();
					}
				}
			}

			// create payload ptr, populate with data
			auto p = std::make_shared<Payload>(
					id, pid, flavor, structName, directory,
					ct, bt, et, dt, run, seq
					);
			p->setURI( uri );

			res = p;
			return res;
		}

		return res;
	}

	Result<std::string> PayloadAdapterDb::setPayload( const SPayloadPtr_t& payload ) {

		Result<std::string> res;

		if ( !payload->ready() ) {
			res.setMsg( "payload is not ready to be stored" );
			return res;
		}

		if ( !ensureMetadata() ) {
			res.setMsg( "db adapter cannot download metadata");
			return res;
		}


		if ( payload->dataSize() && payload->format() != "dat" ) {
			// validate json against schema if exists
			Result<std::string> schema = getTagSchema( payload->directory() + "/" + payload->structName() );
			if ( schema.valid() ) {
				if ( !validate_json_using_schema( payload->dataAsJson(), schema.get() )) {
					res.setMsg( "schema validation failed" );
					return res;
				}
			}
		}

		// get tag, fetch tbname
		auto tagit = mPaths.find( payload->directory() + "/" + payload->structName() );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "cannot find payload tag in the database: " + payload->directory() + "/" + payload->structName() );
			return res;
		}

		std::string tbname = tagit->second->tbname();
		sanitize_alnumuscore(tbname);

		// unpack values for SOCI
		std::string id = payload->id(), pid = payload->pid(), flavor = payload->flavor(), fmt = payload->format();
		int64_t ct = std::time(nullptr), bt = payload->beginTime(), et = payload->endTime(),
			run = payload->run(), seq = payload->seq();

		if ( !setAccessMode("set") ) {
			res.setMsg( "cannot switch to SET mode");
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg("db adapter cannot connect to the database");
			return res;
		}

		// insert iov into cdb_iov_<table-name>, data into cdb_data_<table-name>
		try {
			int64_t dt = 0;
			transaction tr( *mSession.get() );

			if ( !payload->URI().size() && payload->dataSize() ) {
				// if uri is empty and data is not empty, store data locally to the database
				size_t data_size = payload->dataSize();
				std::string data = base64::encode( payload->data() );

				CDBNPP_LOG_ERROR << "mAccessMode: " << mAccessMode << "\n";

				mSession->once << ( "INSERT INTO cdb_data_" + tbname + " ( id, pid, ct, dt, data, size ) VALUES ( :id, :pid, :ct, :dt, :data, :size )" )
					,use(id), use(pid), use(ct), use(dt), use(data), use(data_size);

				payload->setURI( "db://" + tbname + "/" + id );
			}

			std::string uri = payload->URI();
			mSession->once << ( "INSERT INTO cdb_iov_" + tbname + " ( id, pid, flavor, ct, bt, et, dt, run, seq, uri, fmt ) VALUES ( :id, :pid, :flavor, :ct, :bt, :et, :dt, :run, :seq, :uri, :bin )" )
				,use(id), use(pid), use(flavor), use(ct), use(bt), use(et), use(dt), use(run), use(seq), use(uri), use(fmt);

			tr.commit();
			res = id;
		} catch( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}

		return res;
	}

	Result<std::string> PayloadAdapterDb::deactivatePayload( const SPayloadPtr_t& payload, int64_t deactiveTime ) {
		Result<std::string> res;

		if ( !payload->ready() ) {
			res.setMsg( "payload is not ready to be used for deactivation" );
			return res;
		}

		if ( !ensureMetadata() ) {
			res.setMsg( "db adapter cannot download metadata");
			return res;
		}

		if ( !setAccessMode("admin") ) {
			res.setMsg( "cannot switch to ADMIN mode");
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg("db adapter cannot connect to the database");
			return res;
		}

		// get tag, fetch tbname
		auto tagit = mPaths.find( payload->directory() + "/" + payload->structName() );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "cannot find payload tag in the database: " + payload->directory() + "/" + payload->structName() );
			return res;
		}

		std::string tbname = tagit->second->tbname();
		sanitize_alnumuscore(tbname);
		std::string id = payload->id();
		sanitize_alnumdash(id);

		try {
			mSession->once << "UPDATE cdb_iov_" + tbname + " SET dt = :dt WHERE id = :id",
				use(deactiveTime), use( id );
		} catch ( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}

		res = id;
		return res;
	}

	Result<SPayloadPtr_t> PayloadAdapterDb::prepareUpload( const std::string& path ) {

		Result<SPayloadPtr_t> res;

		// check if Db Adapter is configured for writes
		if ( !ensureMetadata() ) {
			res.setMsg( "db adapter cannot download metadata" );
			return res;
		}

		auto [ flavors, directory, structName, is_path_valid ] = Payload::decodePath( path );

		if ( !flavors.size() ) {
			res.setMsg( "no flavor provided: " + path );
			return res;
		}

		if ( !is_path_valid ) {
			res.setMsg( "bad path provided: " + path );
			return res;
		}

		// see if path exists in the known tags map
		auto tagit = mPaths.find( directory + "/" + structName );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "path does not exist in the db. path: " + path );
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


	Result<bool> PayloadAdapterDb::dropTagSchema( const std::string& tag_path ) {
		Result<bool> res;

		if ( !ensureMetadata() ) {
			res.setMsg("cannot ensure metadata");
			return res;
		}

		if ( !setAccessMode("admin") ) {
			res.setMsg("cannot set ADMIN mode");
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg("cannot ensure connection");
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

		// find tbname and id for the tag
		std::string tbname = (tagit->second)->tbname();
		if ( !tbname.size() ) {
			res.setMsg( "tag is not a struct... path: " + sanitized_path );
			return res;
		}

		try {
			mSession->once << "DELETE FROM cdb_schemas WHERE pid = :pid",
				use(tag_pid);
		} catch( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}

		res = true;
		return res;
	}

	Result<bool> PayloadAdapterDb::createDatabaseTables() {

		Result<bool> res;

		if ( !setAccessMode("admin") ) {
			res.setMsg( "cannot switch to ADMIN mode" );
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg( "cannot connect to the database" );
			return res;
		}

		try {
			transaction tr( *mSession.get() );
			// cdb_tags
			{
				soci::ddl_type ddl = mSession->create_table("cdb_tags");
				ddl.column("id", soci::dt_string, 36 )("not null");
				ddl.column("pid", soci::dt_string, 36 )("not null");
				ddl.column("name", soci::dt_string, 128 )("not null");
				ddl.column("ct", soci::dt_unsigned_long_long )("not null");
				ddl.column("dt", soci::dt_unsigned_long_long )("not null default 0");
				ddl.column("mode", soci::dt_unsigned_long_long )("not null default 0"); // 0 = tag, 1 = struct bt,et; 2 = struct run,seq
				ddl.column("tbname", soci::dt_string, 512 )("not null");
				ddl.primary_key("cdb_tags_pk", "pid,name,dt");
				ddl.unique("cdb_tags_id", "id");
			}
			// cdb_schemas
			{
				soci::ddl_type ddl = mSession->create_table("cdb_schemas");
				ddl.column("id", soci::dt_string, 36 )("not null");
				ddl.column("pid", soci::dt_string, 36 )("not null");
				ddl.column("ct", soci::dt_unsigned_long_long )("not null");
				ddl.column("dt", soci::dt_unsigned_long_long )("null default 0");
				ddl.column("data", soci::dt_string )("not null");
				ddl.primary_key("cdb_schemas_pk", "pid,dt");
				ddl.unique("cdb_schemas_id", "id");
			}
			tr.commit();

		} catch( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}

		res = true;
		return res;
	}

	Result<bool> PayloadAdapterDb::createIOVDataTables( const std::string& tablename, bool create_storage ) {
		Result<bool> res;

		if ( !setAccessMode("admin") ) {
			res.setMsg("cannot get ADMIN mode");
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg("cannot ensure connection");
			return res;
		}

		try {
			transaction tr( *mSession.get() );
			// cdb_iov_<tablename>
			{
				soci::ddl_type ddl = mSession->create_table("cdb_iov_"+tablename);
				ddl.column("id", soci::dt_string, 36 )("not null");
				ddl.column("pid", soci::dt_string, 36 )("not null");
				ddl.column("flavor", soci::dt_string, 128 )("not null");
				ddl.column("ct", soci::dt_unsigned_long_long )("not null");
				ddl.column("dt", soci::dt_unsigned_long_long )("not null default 0");
				ddl.column("bt", soci::dt_unsigned_long_long )("not null default 0"); // used with mode = 0
				ddl.column("et", soci::dt_unsigned_long_long )("not null default 0"); // used with mode = 0
				ddl.column("run", soci::dt_unsigned_long_long )("not null default 0"); // used with mode = 1
				ddl.column("seq", soci::dt_unsigned_long_long )("not null default 0"); // used with mode = 1
				ddl.column("fmt", soci::dt_string, 36 )("not null"); // see Service::formats()
				ddl.column("uri", soci::dt_string, 2048 )("not null");
				ddl.primary_key("cdb_iov_"+tablename+"_pk", "pid,bt,run,seq,dt,flavor");
				ddl.unique("cdb_iov_"+tablename+"_id", "id");
			}

			mSession->once << "CREATE INDEX cdb_iov_" + tablename + "_ct ON cdb_iov_" + tablename + " (ct)";

			if ( create_storage ) {
				// cdb_data_<tablename>
				{
					soci::ddl_type ddl = mSession->create_table("cdb_data_"+tablename);
					ddl.column("id", soci::dt_string, 36 )("not null");
					ddl.column("pid", soci::dt_string, 36 )("not null");
					ddl.column("ct", soci::dt_unsigned_long_long )("not null");
					ddl.column("dt", soci::dt_unsigned_long_long )("not null default 0");
					ddl.column("data", soci::dt_string )("not null");
					ddl.column("size", soci::dt_unsigned_long_long )("not null default 0");
					ddl.primary_key("cdb_data_"+tablename+"_pk", "id,pid,dt");
					ddl.unique("cdb_data_"+tablename+"_id", "id");
				}
				mSession->once << "CREATE INDEX cdb_data_" + tablename + "_pid ON cdb_data_" + tablename + " (pid)";
				mSession->once << "CREATE INDEX cdb_data_" + tablename + "_ct ON cdb_data_" + tablename + " (ct)";
			}
			tr.commit();
		} catch( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}
		res = true;
		return res;
	}

	std::vector<std::string> PayloadAdapterDb::listDatabaseTables() {
		std::vector<std::string> tables;

		if ( !setAccessMode("admin") ) {
			return tables;
		}

		if ( !ensureConnection() ) {
			return tables;
		}

		try {
			std::string name;
			soci::statement st = (mSession->prepare_table_names(), into(name));
			st.execute();
			while (st.fetch()) {
				tables.push_back( name );
			}
		} catch( std::exception const & e ) {
			// database exception: " << e.what()
			tables.clear();
		}

		return tables;
	}

	Result<bool> PayloadAdapterDb::dropDatabaseTables() {
		Result<bool> res;

		std::vector<std::string> tables = listDatabaseTables();

		if ( !setAccessMode("admin") ) {
			res.setMsg( "cannot switch to ADMIN mode" );
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg( "cannot connect to the database" );
			return res;
		}

		try {
			for ( const auto& tbname : tables ) {
				mSession->drop_table( tbname );
			}
		} catch( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}
		res = true;
		return res;
	}

	bool PayloadAdapterDb::setAccessMode( const std::string& mode ) {

		if ( !hasAccess(mode) ) {
			// cannot set access mode
			return false;
		}

		if ( mAccessMode == mode ) {
			return true;
		}

		mAccessMode = mode;

		if ( isConnected() ) {
			return reconnect();
		}

		return true;
	}

	void PayloadAdapterDb::disconnect() {
		if ( !isConnected() ) { return; }
		try {
			mSession->close();
			mIsConnected = false;
			mDbType = "";
		} catch ( std::exception const & e ) {
			// database exception, e.what()
		}
	}

	bool PayloadAdapterDb::connect( const std::string& dbtype, const std::string& host, int port,
			const std::string& user, const std::string& pass, const std::string& dbname, const std::string& options ) {

		if ( isConnected() ) {
			disconnect();
		}

		if ( mSession == nullptr ) {
			mSession = std::make_shared<soci::session>();
		}

		std::string connect_string{
			dbtype + "://" + ( host.size() ? "host=" + host : "")
				+ ( port > 0 ? " port=" + std::to_string(port) : "" )
				+ ( user.size() ? " user=" + user : "" )
				+ ( pass.size() ? " password=" + pass : "" )
				+ ( dbname.size() ? " dbname=" + dbname : "" )
				+ ( options.size() ? " " + options : "" )
		};

		try {
			mSession->open( connect_string );
		} catch ( std::exception const & e ) {
			// database exception: " << e.what()
			return false;
		}

		mIsConnected = true;
		mDbType = dbtype;
		return true;
	}

	bool PayloadAdapterDb::reconnect() {
		if ( !hasAccess( mAccessMode ) ) {
			return false;
		}
		if ( isConnected() ) {
			disconnect();
		}
		int port{0};
		std::string dbtype{}, host{}, user{}, pass{}, dbname{}, options{};
		nlohmann::json node;
		size_t idx = RngS::Instance().random_inclusive<size_t>( 0, mConfig["adapters"]["db"][ mAccessMode ].size() - 1 );
		node = mConfig["adapters"]["db"][ mAccessMode ][ idx ];
		if ( node.contains("dbtype") ) {
			dbtype = node["dbtype"];
		}
		if ( node.contains("host") ) {
			host = node["host"];
		}
		if ( node.contains("port") ) {
			port = node["port"];
		}
		if ( node.contains("user") ) {
			user = node["user"];
		}
		if ( node.contains("pass") ) {
			pass = node["pass"];
		}
		if ( node.contains("dbname") ) {
			dbname = node["dbname"];
		}
		if ( node.contains("options") ) {
			options = node["options"];
		}
		return connect( dbtype, host, port, user, pass, dbname, options );
	}

	bool PayloadAdapterDb::downloadMetadata() {
		if ( !ensureConnection() ) {
			return false;
		}
		mTags.clear();
		mPaths.clear();
		// download tags and populate lookup map: tag ID => Tag obj
		try {

			std::string id, name, pid, tbname, schema_id;
			int64_t ct, dt, mode;
			soci::indicator ind;

			statement st = ( mSession->prepare << "SELECT t.id, t.name, t.pid, t.tbname, t.ct, t.dt, t.mode, COALESCE(s.id,'') as schema_id FROM cdb_tags t LEFT JOIN cdb_schemas s ON t.id = s.pid",
					into(id), into(name), into(pid), into(tbname), into(ct), into(dt), into(mode), into(schema_id, ind) );
			st.execute();
			while (st.fetch()) {
				mTags.insert({ id, std::make_shared<Tag>( id, name, pid,	tbname,	ct,	dt, mode, ind == i_ok ? schema_id : "" ) });
			}

		} catch ( std::exception const & e ) {
			// database exception: " << e.what()
			return false;
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

		// TODO: filter mTags and mPaths based on maxEntryTime

		return true;
	}

	Result<std::string> PayloadAdapterDb::createTag( const STagPtr_t& tag ) {
		return createTag( tag->id(), tag->name(), tag->pid(), tag->tbname(), tag->ct(), tag->dt(), tag->mode() );
	}

	Result<std::string> PayloadAdapterDb::createTag( const std::string& path, int64_t tag_mode ) {

		Result<std::string> res;

		if ( !ensureMetadata() ) {
			res.setMsg( "db adapter cannot download metadata" );
			return res;
		}

		if ( !setAccessMode("admin") ) {
			res.setMsg( "cannot switch to ADMIN mode" );
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg("cannot connect to database");
			return res;
		}

		std::string sanitized_path = path;
		sanitize_alnumslash( sanitized_path );

		if ( sanitized_path != path ) {
			res.setMsg( "path contains unwanted characters, path: " + path );
			return res;
		}

		// check for existing tag
		if ( mPaths.find( sanitized_path ) != mPaths.end() ) {
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

		return createTag( tag_id, tag_name, tag_pid, tag_tbname, tag_ct, tag_dt, tag_mode );
	}

	Result<std::string> PayloadAdapterDb::deactivateTag( const std::string& path, int64_t deactiveTime ) {
		Result<std::string> res;

		if ( !ensureMetadata() ) {
			res.setMsg( "db adapter cannot download metadata" );
			return res;
		}

		if ( !setAccessMode("admin") ) {
			res.setMsg( "db adapter is not configured for writes" );
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg("cannot connect to database");
			return res;
		}

		std::string sanitized_path = path;
		sanitize_alnumslash( sanitized_path );

		if ( sanitized_path != path ) {
			res.setMsg( "path contains unwanted characters, path: " + path );
			return res;
		}

		// check for existing tag
		auto tagit = mPaths.find( sanitized_path );

		if ( tagit == mPaths.end() ) {
			res.setMsg( "cannot find path in the database, path: " + path );
			return res;
		}

		std::string tag_id = tagit->second->id();

		if ( !tag_id.size() ) {
			res.setMsg( "empty tag id found" );
			return res;
		}

		try {
			mSession->once << "UPDATE cdb_tags SET dt = :dt WHERE id = :id",
				use(deactiveTime), use(tag_id);
		} catch ( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}

		res = tag_id;
		return res;
	}

	Result<std::string> PayloadAdapterDb::createTag( const std::string& tag_id, const std::string& tag_name, const std::string& tag_pid,
			const std::string& tag_tbname, int64_t tag_ct, int64_t tag_dt, int64_t tag_mode ) {

		Result<std::string> res;

		if ( !tag_id.size() ) {
			res.setMsg( "cannot create new tag: empty tag id provided" );
			return res;
		}

		if ( !tag_name.size() ) {
			res.setMsg( "cannot create new tag: empty tag name provided" );
			return res;
		}

		if ( tag_pid.size() && mTags.find( tag_pid ) == mTags.end() ) {
			res.setMsg( "parent tag provided but not found in the map" );
			return res;
		}

		if ( !setAccessMode("admin") ) {
			res.setMsg( "cannot switch to ADMIN mode" );
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg( "db adapter cannot ensure connection" );
			return res;
		}

		// insert new tag into the database
		try {
			mSession->once << "INSERT INTO cdb_tags ( id, name, pid, tbname, ct, dt, mode ) VALUES ( :id, :name, :pid, :tbname, :ct, :dt, :mode ) ",
				use(tag_id), use(tag_name), use(tag_pid), use(tag_tbname), use(tag_ct), use(tag_dt), use(tag_mode);
		} catch ( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}

		// if it is a struct, create IOV + Data tables
		if ( tag_tbname.size() ) {
			Result<bool> rc = createIOVDataTables( tag_tbname );
			if ( rc.invalid() ) {
				res.setMsg( "failed to create IOV Data Tables for tag: " + tag_name + ", tbname: " + tag_tbname );
				return res;
			}
		}

		// create new tag object and set parent-child relationship
		STagPtr_t new_tag = std::make_shared<Tag>( tag_id, tag_name, tag_pid, tag_tbname, tag_ct, tag_dt );
		if ( tag_pid.size() ) {
			STagPtr_t parent_tag = ( mTags.find( tag_pid ) )->second;
			new_tag->setParent( parent_tag );
			parent_tag->addChild( new_tag );
		}

		// add new tag to maps
		mTags.insert({ tag_id, new_tag });
		mPaths.insert({ new_tag->path(), new_tag });

		res = tag_id;

		return res;
	}

	std::vector<std::string> PayloadAdapterDb::getTags( bool skipStructs ) {

		std::vector<std::string> tags;

		if ( !ensureMetadata() ) {
			return tags;
		}

		if ( !setAccessMode("get") ) {
			return tags;
		}

		if ( !ensureConnection() ) {
			return tags;
		}

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
				if ( value->schema().size() ) { schema = "s"; }
				tags.push_back( "[s/" + schema + "/" + std::to_string( value->mode() ) + "] " + key );
			}
		}
		return tags;
	}

	Result<std::string> PayloadAdapterDb::downloadData( const std::string& uri ) {
		Result<std::string> res;

		if ( !uri.size() ) {
			res.setMsg("empty uri");
			return res;
		}

		// uri = db://<tablename>/<item-uuid>
		auto parts = explode( uri, "://" );
		if ( parts.size() != 2 || parts[0] != "db" ) {
			res.setMsg("bad uri");
			return res;
		}

		if ( !setAccessMode("get") ) {
			res.setMsg( "db adapter is not configured" );
		}

		if ( !ensureConnection() ) {
			res.setMsg("cannot ensure database connection");
			return res;
		}

		auto tbparts = explode( parts[1], "/" );
		if ( tbparts.size() != 2 ) {
			res.setMsg("bad uri tbname");
			return res;
		}

		std::string storage_name = tbparts[0], id = tbparts[1], data;

		try {
			mSession->once << ("SELECT data FROM cdb_data_" + storage_name + " WHERE id = :id"), into(data), use( id );
		} catch( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}

		if ( !data.size() ) {
			res.setMsg("no data");
			return res;
		}

		res = base64::decode(data);
		return res;
	}

	Result<std::string> PayloadAdapterDb::getTagSchema( const std::string& tag_path ) {
		Result<std::string> res;

		if ( !ensureMetadata() ) {
			res.setMsg("cannot download metadata");
			return res;
		}

		if ( !setAccessMode("get") ) {
			res.setMsg("cannot switch to GET mode");
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg("cannot connect to the database");
			return res;
		}

		auto tagit = mPaths.find( tag_path );
		if ( tagit == mPaths.end() ) {
			res.setMsg("cannot find path");
			return res;
		}

		std::string pid = tagit->second->id();

		std::string schema{""};
		try {
			mSession->once << "SELECT data FROM cdb_schemas WHERE pid = :pid ", into(schema), use(pid);
		} catch( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}

		if ( schema.size() ) {
			res = schema;
		} else {
			res.setMsg("no schema");
		}

		return res;
	}

	Result<std::string> PayloadAdapterDb::exportTagsSchemas( bool tags, bool schemas ) {
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
			res.setMsg( "neither tags nor schemas were requested for export");
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
							{ "id", value->schema() }, { "pid", value->id() }, { "path", value->path() }, {"data", schema_str.get() } 
							});
				}
				// cannot get schema for a tag, contact db admin :(
			}
		}

		res = output.dump(2);
		return res;
	}

	Result<bool> PayloadAdapterDb::importTagsSchemas(const std::string& stringified_json ) {
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
				Result<std::string> rc = createTag(
						tag["id"].get<std::string>(),
						tag["name"].get<std::string>(),
						tag["pid"].get<std::string>(),
						tag["tbname"].get<std::string>(),
						tag["ct"].get<uint64_t>(),
						tag["dt"].get<uint64_t>(),
						tag["mode"].get<uint64_t>()
						);
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

	Result<bool> PayloadAdapterDb::setTagSchema( const std::string& tag_path, const std::string& schema_json ) {
		Result<bool> res;

		if ( !schema_json.size() ) {
			res.setMsg( "empty tag schema provided" );
			return res;
		}

		if ( !ensureMetadata() ) {
			res.setMsg("cannot ensure metadata");
			return res;
		}

		if ( !setAccessMode("admin") ) {
			res.setMsg("cannot set ADMIN mode");
			return res;
		}

		if ( !ensureConnection() ) {
			res.setMsg("cannot ensure connection");
			return res;
		}

		// make sure tag_path exists and is a struct
		std::string sanitized_path = tag_path;
		sanitize_alnumslash( sanitized_path );
		if ( sanitized_path != tag_path ) {
			res.setMsg("sanitized tag path != input tag path... path: " + sanitized_path );
			return res;
		}

		std::string tag_pid = "";
		auto tagit = mPaths.find( sanitized_path );
		if ( tagit == mPaths.end() ) {
			res.setMsg( "cannot find tag path in the database... path: " + sanitized_path );
			return res;
		}
		tag_pid = (tagit->second)->id();

		// find tbname and id for the tag
		std::string tbname = (tagit->second)->tbname();
		if ( !tbname.size() ) {
			res.setMsg( "tag is not a struct... path: " + sanitized_path );
			return res;
		}

		std::string existing_id = "";
		try {
			mSession->once << "SELECT id FROM cdb_schemas WHERE pid = :pid ", into(existing_id), use(tag_pid);
		} catch( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}
		if ( existing_id.size() ) {
			res.setMsg( "cannot set schema as it already exists for " + sanitized_path );
			return res;
		}

		// validate schema against schema
		std::string schema = R"({
			"$id": "file://.cdbnpp_config.json",
			"$schema": "https://json-schema.org/draft/2020-12/schema",
			"description": "CDBNPP Config File",
			"type": "object",
			"required": [ "$id", "$schema", "properties", "required" ],
			"properties": {
				"$id": {
					"type": "string"
				},
				"$schema": {
					"type": "string"
				},
				"properties": {
					"type": "object"
				},
				"required": {
					"type": "array",
					"items": {
						"type": "string"
					}
				}
			}
		})";

		if ( !validate_json_using_schema( schema_json, schema ) ) {
			res.setMsg( "proposed schema is invalid" );
			return res;
		}

		// insert schema into cdb_schemas table
		try {
			std::string schema_id = generate_uuid(), data = schema_json;
			long long ct = 0, dt = 0;
			mSession->once << "INSERT INTO cdb_schemas ( id, pid, data, ct, dt ) VALUES( :schema_id, :pid, :data, :ct, :dt )",
				use(schema_id), use(tag_pid), use(data), use(ct), use(dt);
		} catch( std::exception const & e ) {
			res.setMsg( "database exception: " + std::string(e.what()) );
			return res;
		}
		res = true;
		return res;
	}


} // namespace CDBNPP
