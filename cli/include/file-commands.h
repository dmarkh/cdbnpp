
#include <npp/cdb/cdb.h>

#include <iostream>

namespace NPP {
namespace CLI {

using namespace NPP::CDB;
using namespace nlohmann;

inline void file_tags_create( const std::vector<std::string>& args ) {
	if ( args.size() < 2 ) {
		std::cerr << "please provide <tag-path> as an argument!" << std::endl;
		return;
	}

	Service db;
	db.init("file");
  Result<std::string> res = db.createTag( args[1], 0 );

  if ( res.valid() ) {
    std::cout << "created tag: " << args[1] << ", uuid: " << res.get() << "\n";
  } else {
    std::cout << "failed to create tag: " << args[1] << ", msg: " << res.msg() << "\n";
  }
}

inline void file_payload_getbytime( const std::vector<std::string>& args ) {
  if ( args.size() < 3 ) {
    std::cerr << "ERROR: please provide argument: <path> <eventTime> <maxEntryTime>" << "\n";
    return;
  }

	Service db;
	db.init("file");

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

inline void file_payload_getbyrun( const std::vector<std::string>& args ) {
  if ( args.size() < 3 ) {
    std::cerr << "ERROR: please provide argument: <path> <run> <seq> <maxEntryTime>" << "\n";
    return;
  }

	Service db;
	db.init("file");

  std::string path = args[1];
  int64_t run = std::stol(args[2]); // could throw if args[2] is not a parsable number!
  int64_t seq = args.size() >= 4 ? std::stol(args[3]) : 0;
  int64_t maxEntryTime = args.size() >= 5 ? std::stol(args[4]) : 0;

  db.setRun( run );
  db.setSeq( seq );
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

inline void file_payload_setbyrun( const std::vector<std::string>& args ) {
  if ( args.size() < 4 ) {
    std::cerr << "ERROR: please provide arguments: <path> <file-name> <run> <seq>" << "\n";
    return;
  }

	Service db;
	db.init("file");

  std::string path = args[1];
  std::string file = args[2];

  std::string file_data = file_get_contents( file );
	if ( !file_data.size() ) {
		std::cerr << "file cannot be read or is empty: " << file << "\n";
		return;
	}

  int64_t run = std::stol( args[3] );
  int64_t seq = args.size() >= 5 ? std::stoll( args[4] ) : 0;

  Result<SPayloadPtr_t> res = db.prepareUpload( path );
  if ( res.invalid() ) {
    std::cerr << "ERROR: payload cannot be prepared for the upload, " << res.msg() << "\n";
    return;
  }
	SPayloadPtr_t p{res.get()};

  p->setRun( run );
  p->setSeq( seq );

	std::string fmt = std::filesystem::path(file).extension();
	sanitize_alnum(fmt);
	string_to_lower_case(fmt);
	p->setData( file_data, fmt );

  Result<std::string> rc = db.setPayload( p );
  if ( rc.invalid() ) {
    std::cerr << "ERROR: payload upload failed, " << rc.msg() << "\n"; 
  } else {
    std::cout << "payload was successfully uploaded, id: " << rc.get() << "\n";
  }
}

inline void file_payload_setbytime( const std::vector<std::string>& args ) {
  if ( args.size() < 4 ) {
    std::cerr << "ERROR: please provide arguments: <path> <file-name> <beginTime> <endTime>" << "\n";
    return;
  }

	Service db;
	db.init("file");

  std::string path = args[1];
  std::string file = args[2];

  std::string file_data = file_get_contents( file );
	if ( !file_data.size() ) {
		std::cerr << "file cannot be read or is empty: " << file << "\n";
		return;
	}

  int64_t bt = is_integer( args[3] ) ? std::stol( args[3] ) : utc_date_to_unixtime( args[3] );
  int64_t et = 0;
	if ( args.size() >= 5 ) {
		et = is_integer( args[4] ) ? std::stoll( args[4] ) : utc_date_to_unixtime( args[4] );
	}

  Result<SPayloadPtr_t> res = db.prepareUpload( path );

  if ( res.invalid() ) {
    std::cerr << "ERROR: payload cannot be prepared, " << res.msg() << "\n";
    return;
  }
	SPayloadPtr_t p{res.get()};

  p->setBeginTime( bt );
  p->setEndTime( et );

	std::string fmt = std::filesystem::path(file).extension();
	sanitize_alnum(fmt);
	string_to_lower_case(fmt);
	p->setData( file_data, fmt );

  Result<std::string> rc = db.setPayload( p );
  if ( rc.invalid() ) {
    std::cerr << "ERROR: payload upload failed, " << rc.msg() << "\n";
  } else {
    std::cout << "payload was successfully uploaded, id: " << rc.get() << "\n";
  }
}

inline void file_convert_json2ubjson( const std::vector<std::string>& args ) {
  if ( args.size() < 3 ) {
    std::cerr << "ERROR: please provide arguments: <input-file> <output-file>" << "\n";
    return;
  }

	if ( !std::filesystem::exists( args[1] ) ) {
		std::cerr << "ERROR: input file does not exist" << "\n";
		return;
	}

	if ( std::filesystem::exists( args[2] ) ) {
		std::cerr << "ERROR: output file already exists" << "\n";
		return;
	}

	std::string input_data = file_get_contents( args[1] );
	json js = json::parse( input_data.begin(), input_data.end(), nullptr, false, true );
	if ( js.empty() || js.is_discarded() || js.is_null() ) {
		std::cerr << "ERROR: input data is not a valid json" << "\n";
		return;
	}

	std::string output_data{};
	nlohmann::json::to_ubjson( js, output_data );
	if ( !file_put_contents( args[2], output_data ) ) {
		std::cerr << "ERROR: cannot write output data" << "\n";
		return;
	}

	std::cout << "success!" << "\n";
}

inline void file_convert_json2cbor( const std::vector<std::string>& args ) {
  if ( args.size() < 3 ) {
    std::cerr << "ERROR: please provide arguments: <input-file> <output-file>" << "\n";
    return;
  }

	if ( !std::filesystem::exists( args[1] ) ) {
		std::cerr << "ERROR: input file does not exist" << "\n";
		return;
	}

	if ( std::filesystem::exists( args[2] ) ) {
		std::cerr << "ERROR: output file already exists" << "\n";
		return;
	}

	std::string input_data = file_get_contents( args[1] );
	json js = json::parse( input_data.begin(), input_data.end(), nullptr, false, true );
	if ( js.empty() || js.is_discarded() || js.is_null() ) {
		std::cerr << "ERROR: input data is not a valid json" << "\n";
		return;
	}

	std::string output_data{};
	nlohmann::json::to_cbor( js, output_data );
	if ( !file_put_contents( args[2], output_data ) ) {
		std::cerr << "ERROR: cannot write output data" << "\n";
		return;
	}

	std::cout << "success!" << "\n";
}

inline void file_convert_json2msgpack( const std::vector<std::string>& args ) {
  if ( args.size() < 3 ) {
    std::cerr << "ERROR: please provide arguments: <input-file> <output-file>" << "\n";
    return;
  }

	if ( !std::filesystem::exists( args[1] ) ) {
		std::cerr << "ERROR: input file does not exist" << "\n";
		return;
	}

	if ( std::filesystem::exists( args[2] ) ) {
		std::cerr << "ERROR: output file already exists" << "\n";
		return;
	}

	std::string input_data = file_get_contents( args[1] );
	json js = json::parse( input_data.begin(), input_data.end(), nullptr, false, true );
	if ( js.empty() || js.is_discarded() || js.is_null() ) {
		std::cerr << "ERROR: input data is not a valid json" << "\n";
		return;
	}

	std::string output_data{};
	nlohmann::json::to_msgpack( js, output_data );
	if ( !file_put_contents( args[2], output_data ) ) {
		std::cerr << "ERROR: cannot write output data" << "\n";
		return;
	}

	std::cout << "success!" << "\n";
}

inline void file_convert_ubjson2json( const std::vector<std::string>& args ) {
  if ( args.size() < 3 ) {
    std::cerr << "ERROR: please provide arguments: <input-file> <output-file>" << "\n";
    return;
  }

	if ( !std::filesystem::exists( args[1] ) ) {
		std::cerr << "ERROR: input file does not exist" << "\n";
		return;
	}

	if ( std::filesystem::exists( args[2] ) ) {
		std::cerr << "ERROR: output file already exists" << "\n";
		return;
	}

	std::string input_data = file_get_contents( args[1] );
	json js = nlohmann::json::from_ubjson( input_data.begin(), input_data.end(), false, false );
	if ( js.empty() || js.is_discarded() || js.is_null() ) {
		std::cerr << "ERROR: input data is not a valid json" << "\n";
		return;
	}

	if ( !file_put_contents( args[2], js.dump(2) ) ) {
		std::cerr << "ERROR: cannot write output data" << "\n";
		return;
	}

	std::cout << "success!" << "\n";
}

inline void file_convert_cbor2json( const std::vector<std::string>& args ) {
  if ( args.size() < 3 ) {
    std::cerr << "ERROR: please provide arguments: <input-file> <output-file>" << "\n";
    return;
  }

	if ( !std::filesystem::exists( args[1] ) ) {
		std::cerr << "ERROR: input file does not exist" << "\n";
		return;
	}

	if ( std::filesystem::exists( args[2] ) ) {
		std::cerr << "ERROR: output file already exists" << "\n";
		return;
	}

	std::string input_data = file_get_contents( args[1] );
	json js = nlohmann::json::from_cbor( input_data.begin(), input_data.end(), false, false );
	if ( js.empty() || js.is_discarded() || js.is_null() ) {
		std::cerr << "ERROR: input data is not a valid json" << "\n";
		return;
	}

	if ( !file_put_contents( args[2], js.dump(2) ) ) {
		std::cerr << "ERROR: cannot write output data" << "\n";
		return;
	}

	std::cout << "success!" << "\n";
}

inline void file_convert_msgpack2json( const std::vector<std::string>& args ) {
  if ( args.size() < 3 ) {
    std::cerr << "ERROR: please provide arguments: <input-file> <output-file>" << "\n";
    return;
  }

	if ( !std::filesystem::exists( args[1] ) ) {
		std::cerr << "ERROR: input file does not exist" << "\n";
		return;
	}

	if ( std::filesystem::exists( args[2] ) ) {
		std::cerr << "ERROR: output file already exists" << "\n";
		return;
	}

	std::string input_data = file_get_contents( args[1] );
	json js = nlohmann::json::from_msgpack( input_data.begin(), input_data.end(), false, false );
	if ( js.empty() || js.is_discarded() || js.is_null() ) {
		std::cerr << "ERROR: input data is not a valid json" << "\n";
		return;
	}

	if ( !file_put_contents( args[2], js.dump(2) ) ) {
		std::cerr << "ERROR: cannot write output data" << "\n";
		return;
	}

	std::cout << "success!" << "\n";
}


} // namespace CLI
} // namespace NPP
