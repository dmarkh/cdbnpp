
#include <npp/cdb/cdb.h>

#include <iostream>
#include <string>

using namespace NPP::CDB;
using namespace nlohmann;

int main() {

	// First, make sure that CDBNPP_CONFIG env var points
	// to the location of the .cdbnpp.json file ( see ./configs/ )
	// or you have a config file in your current directory

	Service& cdb = ServiceS::Instance();

	// initialize memory (caching) adapter
	// initialize database (direct-db) adapter
	// ...full list: "memory+file+db+http"
	cdb.init("memory+http");

	Result<SPayloadPtr_t> res = cdb.prepareUpload("ofl:Calibrations/tpc/tpcT0");
	if ( res.invalid() ) {
		// something went deeply wrong
		return EXIT_FAILURE;
	}
	SPayloadPtr_t payload = res.get();
	// ...then set IOV:
	payload->setBeginTime("");
	payload->setEndTime("");
	//..OR..
	payload->setRun(123);
	payload->setSeq(456);

	// ...then, set external data ref...
	payload->setURI("http://my-data-server.gov/data/payload.bson");

	// ..OR..
	payload->setURI("file:///my/cvmfs/mount/point/payload.msgpack");

	// ..OR..
	json val = R"({
		"chi2": 5.2,
		"pos": 3.141
  })"_json;
	payload->setData( val, "ubjson" );

	// ..OR..
	std::string data = "BINARYDATA";
	payload->setData( data /*, fmt = "dat" */ );

	Result<std::string> rc = cdb.setPayload( payload );
	if ( rc.invalid() ) {
		std::cout << "upload failed because: " << rc.msg() << "\n";
	} else {
		std::cout << "upload succeeded, uuid: " << rc.get() << "\n";
	}

	return EXIT_SUCCESS;
}
