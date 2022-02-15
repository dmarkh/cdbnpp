
#include <cdbnpp/cdbnpp.h>

namespace CDBNPP {

inline void memory_test_setget( __attribute__ ((unused)) const std::vector<std::string>& args ) {
  Service db;
  db.init("memory");

	SPayloadPtr_t p1 = std::make_shared<Payload>( "id123" /* id */, "pid345" /* pid */, "ofl" /* flavor */,
        "struct1" /* structName */, "Calibrations/tpc" /* directory */,
        123123 /* ct */, 123123 /* bt */, 123456 /* et */, 0 /* dt */, 123 /* run */, 345 /* seq */
		);
	p1->setMode(1);
	p1->setURI("db://test/id123");
	std::string data = "{ \"test\": 10 }";
	std::string fmt = "json";
	p1->setData( data, fmt );

	SPayloadPtr_t p2 = std::make_shared<Payload>( "id456" /* id */, "pid345" /* pid */, "ofl" /* flavor */,
        "struct1" /* structName */, "Calibrations/tpc" /* directory */,
        123456 /* ct */, 123456 /* bt */, 123789 /* et */, 0 /* dt */, 456 /* run */, 789 /* seq */
		);

	p2->setMode(2);
	p2->setURI("db://test/id123");
	data = "{ \"test\": 20 }";
	fmt = "json";
	p2->setData( data, fmt );

	std::shared_ptr<PayloadAdapterMemory> adapter = std::dynamic_pointer_cast<PayloadAdapterMemory>( db.getPayloadAdapterMemory() );
	Result<std::string> res1 = adapter->setPayload( p1 );
	if ( res1.invalid() ) {
		std::cerr << "ERROR: cannot set test payload1 to memory adapter" << std::endl;
	}
	Result<std::string> res2 = adapter->setPayload( p2 );
	if ( res2.invalid() ) {
		std::cerr << "ERROR: cannot set test payload2 to memory adapter" << std::endl;
	}

	db.setEventTime( 123125 );
	db.setMaxEntryTime( 9999999999 );

  PayloadResults_t res = db.getPayloads({ "ofl:Calibrations/tpc/struct1" });
  if ( res.size() == 1 ) {
		auto it = res.find("Calibrations/tpc/struct1");
		if ( it == res.end() ) {
			std::cerr << "ERROR: incorrect result returned while searching for payload 1 by time " << std::endl;
		} else {
			auto p = it->second;
			if ( p->id() == p1->id() ) {
				std::cout << "SUCCESS: got proper payload 1 by time" << std::endl;
			} else {
				std::cerr << "ERROR: payload 1 get by time failed" << std::endl;
			}
		}
  } else {
		std::cerr << "ERROR: search for payload 1 by time yielded " << res.size() << "results" << std::endl;
	}

	db.setEventTime( 0 );
	db.setRun( 456 );
	db.setSeq( 789 );
	res = db.getPayloads({ "ofl:Calibrations/tpc/struct1" });
  if ( res.size() == 1 ) {
		auto it = res.find("Calibrations/tpc/struct1");
		if ( it == res.end() ) {
			std::cerr << "ERROR: incorrect result returned while searching for payload 1 by time " << std::endl;
		} else {
			auto p = it->second;
			if ( p->id() == p2->id() ) {
				std::cout << "SUCCESS: got proper payload 2 by run/seq" << std::endl;
			} else {
				std::cerr << "ERROR: payload 2 get by run/seq failed" << std::endl;
			}
		}
  } else {
		std::cerr << "ERROR: search for payload 2 by run/seq yielded " << res.size() << " results" << std::endl;
	}
}

} // namespace CDBNPP
