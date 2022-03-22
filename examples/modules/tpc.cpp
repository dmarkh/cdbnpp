#include "tpc.h"

#include <thread>
#include <chrono>

#include <npp/util/log.h>

using namespace NPP::Util;

namespace NPP {
namespace Framework {

	int32_t Tpc::execute() {
    for ( int i = 0; i < 10; i++ ) {
      CDBNPP_LOG_DEBUG << "executing: " << id() << "\n";
      std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    }
		return 0;
	}

} // namespace Framework
} // namespace NPP