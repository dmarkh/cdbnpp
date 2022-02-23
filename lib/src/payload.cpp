
#include "npp/cdb/payload.h"

#include <iostream>

#include "npp/util/log.h"
#include "npp/util/util.h"

namespace NPP {
namespace CDB {

	using namespace NPP::Util;

	nlohmann::json Payload::dataAsJson() const {
		if ( mFmt == "json" ) {
			return nlohmann::json::parse( mData.begin(), mData.end(), nullptr, false, true );
		} else if ( mFmt == "bson" ) {
			return nlohmann::json::from_bson( mData.begin(), mData.end(), false, false );
		} else if ( mFmt == "ubjson" ) {
			return nlohmann::json::from_ubjson( mData.begin(), mData.end(), false, false );
		} else if ( mFmt == "cbor" ) {
			return nlohmann::json::from_cbor( mData.begin(), mData.end(), false, false );
		} else if ( mFmt == "msgpack" ) {
			return nlohmann::json::from_msgpack( mData.begin(), mData.end(), false, false );
		}
		return mData;
	}

	void Payload::setData( const nlohmann::json& data, const std::string& fmt ) {
		if ( fmt == "bson" ) {
			nlohmann::json::to_bson( data, mData);
			mFmt = fmt;
		} else if ( fmt == "ubjson" ) {
			nlohmann::json::to_ubjson( data, mData );
			mFmt = fmt;
		} else if ( fmt == "cbor" ) {
			nlohmann::json::to_cbor( data, mData );
			mFmt = fmt;
		} else if ( fmt == "msgpack" ) {
			nlohmann::json::to_msgpack( data, mData );
			mFmt = fmt;
		} else {
			mData = data.dump();
			mFmt = "json";
		}
	}

	void Payload::setData( const std::string& data, const std::string& fmt ) {
		mData = data;
		if ( fmt == "json" || fmt == "bson" || fmt == "ubjson" || fmt == "cbor" || fmt == "msgpack" ) {
			mFmt = fmt;
		} else {
			mFmt = "dat";
		}
	}

	DecodedPathTuple Payload::decodePath( const std::string& path ) {
		// expected path formats:
		//   "ofl:Calibrations/TPC/tpcT0"
		//   "sim+ofl:Calibrations/TPC/tpcT0"
		//   "Calibrations/TPC/tpcT0"
		// decoded tuple format:
		//   0: ["sim","ofl"], 1: "Calibrations/TPC", 2: "tpcT0", 3: true|false

		std::string directory = "", structName = "";
		std::vector<std::string> flavors{};

		if ( !path.size() ) {
			// empty path provided => return empty tuple
			return std::make_tuple( flavors, directory, structName, false );
		}

		std::vector<std::string> parts = explode_and_trim( path, ':' );
		if ( parts.size() > 2 ) {
			// broken path provided => return empty tuple
			return std::make_tuple( flavors, directory, structName, false );
		}

		std::vector<std::string> path_parts{};

		if ( parts.size() < 2 ) {
			// no flavors provided, skipping
			path_parts = explode( parts[0], '/' );
		} else {
			flavors = explode_and_trim_and_sanitize( parts[0], '+' );
			path_parts = explode( parts[1], '/' );
		}

		structName = path_parts.back();
		sanitize_alnum( structName );
		path_parts.pop_back();

		directory = implode( path_parts, "/" );
		trim( directory, "/ \n\r\t\v" );

		return std::make_tuple( flavors, directory, structName, true );
	}

} // namespace CDB
} // namespace NPP
