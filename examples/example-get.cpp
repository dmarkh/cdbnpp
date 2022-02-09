
#include <cdbnpp/cdbnpp.h>

#include <iostream>

using namespace CDBNPP;
using namespace nlohmann;

int main() {

	// First, make sure that CDBNPP_CONFIG env var points
	// to the location of the .cdbnpp.json file ( see ./configs/ )
	// or you have a config file in your current directory

	Service& cdb = ServiceS::Instance();

	// initialize memory (caching) adapter
	// initialize database (direct-db) adapter
	// ...full list: "memory+file+db+http"
	cdb.init("memory+db");

  cdb.setMaxEntryTime( "20230101T000000" );

	// set default flavors (aka global_tags aka channels)
	// here, "sim" flavor will be probed first,
	// then "ofl" if "sim" is not found
	cdb.setFlavors({"sim","ofl"});

	// set Event Time for IOV search
	// this is what NPP frameworks do for each event
  cdb.setEventTime( "20220101T010203" );

	// alternatively, use Run Number + Run Sequence based lookups:
	// db.setRun( N ); db.setSeq( M );

	// Request payloads by tag/folder path:
	// ..of course, assuming that you have data in the DB
	PayloadResults_t results = cdb.getPayloads(
		{
			"Calibrations/TPC/t0offset",
			"Geometry/TPC/survey",
			"Conditions/CAD/beamInfo"
		} /* paths */
		, true /* fetch_data = true */ );

	for ( const auto &[path, payload] : results ) {
		std::cout << "path: " << path << ", payload URI: " << payload->URI()
			<< ", has data: " << ( payload->dataSize() ? "yes" : "no" ) << std::endl;
  }

	if ( results.count("Calibrations/TPC/t0offset") ) {
		SPayloadPtr_t& payload = results["Calibrations/TPC/t0Offset"];
		// assuming that data was saved as JSON / BSON/ UBJSON / CBOR / MSGPACK
		json val = payload->dataAsJson();
		// ... use val here ...
	} else {
		std::cout << "no results found :(" << std::endl;
	}

	return EXIT_SUCCESS;
}
