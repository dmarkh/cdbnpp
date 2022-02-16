
#include <cdbnpp/cdbnpp.h>

#include <chrono>
#include <iostream>

namespace CDBNPP {

inline void db_payload_set( const std::vector<std::string>& args ) {
	if ( args.size() < 4 ) {
		std::cerr << "ERROR: please provide arguments: <path> <file-name|uri> <beginTime|run> <endTime|seq>" << "\n";
		return;
	}

	Service db;
	db.init("db");
	std::string path = args[1];

	std::string uri, file, file_data;

	if ( string_starts_with( args[2], "file://" ) 
			|| string_starts_with( args[2], "http://" ) 
			|| string_starts_with( args[2], "https://" )
			|| string_starts_with( args[2], "db://" ) ) {
		uri = args[2];
	} else {
		file = args[2];
		file_data = file_get_contents( file );
	}

	int64_t bt_or_run = std::stol( args[3] );
	int64_t et_or_seq = args.size() >= 5 ? std::stoll( args[4] ) : 0;

	Result<SPayloadPtr_t> res = db.prepareUpload( path );

	if ( res.invalid() ) {
		std::cerr << "ERROR: payload cannot be prepared, because: " << res.msg() << "\n";
		return;
	}
	SPayloadPtr_t p{res.get()};

	if ( p->mode() == 1 ) {
		p->setBeginTime(bt_or_run);
		p->setEndTime(et_or_seq);
	} else if ( p->mode() == 2 ) {
		p->setRun( bt_or_run );
		p->setSeq( et_or_seq );
	} else {
		std::cerr << "unknown payload mode: " << p->mode() << "\n";
		return;
	}

	if ( file.size() ) {
		std::string fmt = std::filesystem::path( file ).extension();
		sanitize_alnum( fmt );
  	string_to_lower_case(fmt);
		p->setData( file_data, fmt );
	} else {
		p->setURI( uri );
	}

	Result<std::string> rc = db.setPayload( p );
	if ( rc.invalid() ) {
		std::cerr << "ERROR: payload upload failed, because: " << rc.msg() << "\n";
	} else {
		std::cout << "payload was successfully uploaded, id: " << rc.get() << "\n";
	}
}

inline void db_payload_getbyrun( const std::vector<std::string>& args ) {
	if ( args.size() < 3 ) {
		std::cerr << "ERROR: please provide argument: <path> <run> <seq> <maxEntryTime>" << "\n";
		return;
	}

	Service db;
	db.init("db");

	std::string path = args[1];
	int64_t run = std::stol(args[2]);
	int64_t seq = args.size() >= 4 ? std::stol(args[3]) : 0;
	int64_t maxEntryTime = 0;
	if ( args.size() >= 5 ) {
		maxEntryTime = is_integer( args[4] ) ? std::stol(args[4]) : utc_date_to_unixtime( args[4] );
	}

	db.setRun( run );
	db.setSeq( seq );
	db.setMaxEntryTime( maxEntryTime );
	db.setFlavors({"sim", "ofl"}); // default flavors "sim+ofl", if not provided by user

	PayloadResults_t res = db.getPayloads({ path });
	if ( !res.size() ) {
		std::cerr << "no data found for path: " << path << "\n";
	} else {
		for( const auto& r : res ) {
			const auto& p = r.second;
			std::cout << "got payload: " << p << std::endl;
		}
	}
}

inline void db_payload_getbytime( const std::vector<std::string>& args ) {
	if ( args.size() < 3 ) {
		std::cerr << "ERROR: please provide argument: <path> <eventTime> <maxEntryTime>" << "\n";
		return;
	}

	Service db;
	db.init("db");

	std::string path = args[1];
	int64_t eventTime = is_integer( args[2] ) ? std::stol(args[2]) : utc_date_to_unixtime( args[2] );
	int64_t maxEntryTime = 0;
	if ( args.size() >= 4 ) {
		maxEntryTime = is_integer( args[3] ) ? std::stol(args[3]) : utc_date_to_unixtime( args[3] );
	}

	db.setEventTime( eventTime );
	db.setMaxEntryTime( maxEntryTime );

	PayloadResults_t res = db.getPayloads({ path });
	if ( !res.size() ) {
		std::cerr << "no data found for path: " << path << "\n";
	} else {
		for ( const auto& r : res ) {
			const auto& p = r.second;
			std::cout << "got payload: " << p << std::endl;
		}
	}
}

inline void db_schema_get( const std::vector<std::string>& args ) {
  if ( args.size() < 2 ) {
    std::cerr << "please provide <tag-path>/<structName> as an argument" << std::endl;
    return;
  }
  Service db;
  db.init("db");
  Result<std::string> res = db.getTagSchema( args[1] );
  if ( res.valid() ) {
    std::cout << "got schema for " << args[1] << ", schema: " << res.get() << std::endl;
  } else {
    std::cerr << "failed to set schema for " << args[1] << ", " << res.msg() << std::endl;
  }
}

inline void db_schema_set( const std::vector<std::string>& args ) {
	if ( args.size() < 2 ) {
		std::cerr << "please provide <tag-path>/<structName> as an argument" << std::endl;
		return;
	}

	if ( !std::filesystem::exists( args[2] ) || !std::filesystem::is_regular_file( args[2] ) ) {
		std::cerr << "schema file does not exist: " << args[2] << std::endl;
		return;
	}

	std::string schema_json = file_get_contents( args[2] );
	if ( !schema_json.size() ) {
		std::cerr << "schema file is empty" << std::endl;
		return;
	}

	nlohmann::json js = nlohmann::json::parse( schema_json, nullptr, false, false );
	if ( js.is_discarded() ) {
		std::cerr << "schema json is malformed" << std::endl;
		return;
	}

	Service db;
	db.init("db");
	Result<bool> rc = db.setTagSchema( args[1], schema_json );
	if ( rc.valid() ) {
		std::cout << "set schema for " << args[1] << std::endl;
	} else {
		std::cerr << "failed to set schema for " << args[1] << ": " << rc.msg() << std::endl;
	}
}

inline void db_schema_drop( const std::vector<std::string>& args ) {
	if ( args.size() < 2 ) {
		std::cerr << "please provide <tag-path>/<structName> as an argument" << std::endl;
		return;
	}
	Service db;
	db.init("db");
	Result<bool> rc = db.dropTagSchema( args[1] );
	if ( rc.valid() ) {
		std::cout << "dropped schema for " << args[1] << std::endl;
	} else {
		std::cout << "failed to drop schema for " << args[1] << ": " << rc.msg() << std::endl;
	}
}

inline void db_struct_create( const std::vector<std::string>& args ) {
	if ( args.size() < 3 ) {
		std::cerr << "please provide <tag-path>/<struct-name> <mode> <schema-file> as arguments" << std::endl;
		return;
	}

	int64_t mode = string_to_longlong( args[2] );
	if ( mode < 1 || mode > 2 ) {
		std::cerr << "cannot create struct " << args[1] << ", unknown mode: " << mode << "\n";
		return;
	}

	Service db;
	db.init("db");
	Result<std::string> res = db.createTag( args[1], mode );
	if ( res.valid() ) {
		std::cout << "created struct: " << args[1] << ", mode: " << args[2] << ", uuid: " << res.get() << "\n";
	} else {
		std::cerr << "failed to create struct: " << args[1] << ", mode: " << args[2] << ", msg: " << res.msg() << "\n";
		return;
	}

	if ( res.valid() && args.size() >= 4 ) {
		if ( !std::filesystem::exists( args[3] ) || !std::filesystem::is_regular_file( args[3] ) ) {
			std::cerr << "schema file does not exist: " << args[3] << std::endl;
			return;
		}
		std::string schema_json = file_get_contents( args[3] );
		if ( !schema_json.size() ) {
			std::cerr << "schema file is empty" << std::endl;
			return;
		}

		nlohmann::json js = nlohmann::json::parse( schema_json, nullptr, false, false );
    if ( js.is_discarded() ) {
			std::cerr << "schema json is malformed" << std::endl;
      return;
    }

		Result<bool> schema_rc = db.setTagSchema( args[1], schema_json );
		if ( schema_rc.valid() ) {
			std::cout << "uploaded schema for struct " << args[1] << ", mode = " << mode << std::endl;
		} else {
			std::cerr << "failed to upload schema for struct " << args[1] << ", mode = " << mode << " using file " << args[3] << ": " << schema_rc.msg() << std::endl;
			return;
		}
	}
}

inline void db_tags_create( const std::vector<std::string>& args ) {
	if ( args.size() < 2 ) {
		std::cerr << "please provide <tag-path> as an argument!" << std::endl;
		return;
	}
	Service db;
	db.init("db");
	Result<std::string> res = db.createTag( args[1], 0 );
	if ( res.valid() ) {
		std::cout << "created tag: " << args[1] << ", uuid: " << res.get() << "\n";
	} else {
		std::cout << "failed to create tag: " << args[1] << ", msg: " << res.msg() << "\n";
	}
}

inline void db_tags_deactivate( const std::vector<std::string>& args ) {
	if ( args.size() < 3 ) {
		std::cerr << "please provide <tag-path> and <deactive-time> as arguments" << std::endl;
		return;
	}
	Service db;
	db.init("db");
	uint64_t dt = string_to_longlong( args[2] );
	Result<std::string> res = db.deactivateTag( args[1], dt );
	if ( res.valid() ) {
		std::cout << "deactivated tag: " << args[1] << ", dt: " << dt << ", uuid: " << res.get() << "\n";
	} else {
		std::cout << "failed to deactivate tag: " << args[1] << ", dt: " << dt <<  ", msg: " << res.msg() << "\n";
	}
}


inline void db_tags_list( __attribute__ ((unused)) const std::vector<std::string>& args ) {
	Service db;
	db.init("db");
  std::shared_ptr<PayloadAdapterDb> adapter = std::dynamic_pointer_cast<PayloadAdapterDb>( db.getPayloadAdapterDb() );
	std::vector<std::string> tags = adapter->getTags();
	if ( tags.size() ) {
		std::cout << "Tags ("<< tags.size() <<"): " << "\n";
		for ( auto& tag : tags ) {
			std::cout << " \t" << tag << "\n";
		}
	} else {
		std::cout << "no tags found" << std::endl;
	}
}

inline void db_tables_create( __attribute__ ((unused)) const std::vector<std::string>& args ) {
	Service db;
	db.init("db");
  std::shared_ptr<PayloadAdapterDb> adapter = std::dynamic_pointer_cast<PayloadAdapterDb>( db.getPayloadAdapterDb() );
	Result<bool> rc = adapter->createDatabaseTables();
  if ( rc.valid() ) {
    std::cout << "database tables were created - initialization complete" << std::endl;
  } else {
    std::cerr << "failed to create initial database tables, " << rc.msg() << std::endl;
  }
}

inline void db_tables_list( __attribute__ ((unused)) const std::vector<std::string>& args ) {
	Service db;
	db.init("db");
  std::shared_ptr<PayloadAdapterDb> adapter = std::dynamic_pointer_cast<PayloadAdapterDb>( db.getPayloadAdapterDb() );
	std::vector<std::string> tables = adapter->listDatabaseTables();
	if ( tables.size() ) {
		std::cout << tables.size() << " tables found" << std::endl;
		for ( auto& table: tables ) {
			std::cout << " " << table << std::endl;
		}
	} else {
		std::cout << "no tables found, database is not initialized yet?" << std::endl;
	}
}

inline void db_tables_drop( __attribute__ ((unused)) const std::vector<std::string>& args ) {
	Service db;
	db.init("db");
  std::shared_ptr<PayloadAdapterDb> adapter = std::dynamic_pointer_cast<PayloadAdapterDb>( db.getPayloadAdapterDb() );
	Result<bool> rc = adapter->dropDatabaseTables();
  if ( rc.valid() ) {
    std::cout << "all database tables dropped" << std::endl;
  } else {
    std::cerr << "failed to drop all database tables, " << rc.msg() << std::endl;
  }
}

inline void db_tags_export( const std::vector<std::string>& args ) {
	if ( args.size() < 2 ) {
		std::cerr << "please provide <output-file> as an argument!" << std::endl;
		return;
	}
	Service db;
	db.init("db");
	Result<std::string> rc = db.exportTagsSchemas();
  if ( rc.valid() ) {
		bool res = file_put_contents( args[1], rc.get() );
		if ( res ) {
	    std::cout << "exported data to " << args[1] << std::endl;
		} else {
			std::cout << "cannot use an ouput file: " << args[1] << "\n";
		}
  } else {
    std::cerr << "failed to export tags and schemas: " << rc.msg() << std::endl;
  }
}

inline void db_tags_import( const std::vector<std::string>& args ) {
	if ( args.size() < 2 ) {
		std::cerr << "please provide <input-file> as an argument!" << std::endl;
		return;
	}
	std::string data = file_get_contents( args[1] );
	if ( !data.size() ) {
		std::cerr << "empty or non-existing input file: " << args[1] << std::endl;
		return;
	}

	Service db;
	db.init("db");
	Result<bool> rc = db.importTagsSchemas( data );
  if ( rc.valid() ) {
	   std::cout << "successfully imported tags and schemas from " << args[1] << ( rc.get() ? "" : "(with errors)" ) << std::endl;
  } else {
    std::cerr << "failed to import tags and schemas: " << rc.msg() << std::endl;
  }
}


} // namespace CDBNPP
