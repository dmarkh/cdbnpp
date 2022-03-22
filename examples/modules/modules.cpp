#include <cstdlib>
#include <iostream>

#include "iomaker.h"
#include "tpc.h"
#include "svt.h"
#include "bemc.h"
#include "eemc.h"
#include "analysis.h"

#include <npp/core/chain.h>
#include <npp/util/log.h>

using namespace NPP::Core;
using namespace NPP::Framework;
using namespace NPP::Util;

int main() {

	Log::setError( &std::cerr );
	Log::setInfo(  &std::cout );
	Log::setDebug( &std::cout );

	uint32_t max_threads_requested{10};

	auto chain = std::make_shared<Chain>();
	chain->setMaxThreads( max_threads_requested );
	uint32_t max_threads_received = chain->getMaxThreads();

	CDBNPP_LOG_DEBUG << "requested " << max_threads_requested << " max threads, received " << max_threads_received << " threads" << "\n";

	// modules could be inserted into Chain in any order
	chain->add( std::make_shared<IOMaker>() );
	chain->add( std::make_shared<Tpc>() );
	chain->add( std::make_shared<Svt>() );
	chain->add( std::make_shared<Bemc>() );
	chain->add( std::make_shared<Eemc>() );
	chain->add( std::make_shared<Analysis>() );

	// run chain
	int32_t rc = chain->execute( 1 );
	std::cout << "chain return code: " << rc << "\n";

	return EXIT_SUCCESS;
}
