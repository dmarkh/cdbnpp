
#include <cdbnpp/cdbnpp.h>

#include <iostream>

namespace CDBNPP {

inline void config_show_adapters( __attribute__ ((unused)) const std::vector<std::string>& args ) {
	Service db;
	db.init("memory+file+db+http");
	std::vector<std::string> adapters = db.enabledAdapters();
	std::cout << "list of payload adapters:\n";
	for ( auto adapter : adapters ) {
		std::cout << " * " << adapter << "\n";
	}
}

} // namespace CDBNPP
