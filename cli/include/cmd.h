
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

#include <npp/util/singleton.h>

namespace NPP {
namespace CLI {

	using namespace NPP::Util;

	typedef std::map<std::string, std::tuple<std::string,std::string,std::function<void( const std::vector<std::string>& args )>>> MapOfCommands;

	class Cmd {
		public:
			void registerCommand( std::string name, std::string args, std::string desc, std::function<void( const std::vector<std::string>& args )> func ) {
				mCommands.insert({ name, std::make_tuple( args, desc, func ) });
			}

			void process( int argc, const char *argv[] ) {
				std::vector<std::string> args( argv+1, argv+argc );

				// check for no arguments => help page
				if ( !args.size() || args[0] == "-h" || args[0] == "--help" ) {
					std::cout << "Usage: cdbnpp-cli --cmd=<command> <arg1> <arg2> ... <argN> \n"
						<< std::left << std::setw(36) << " COMMAND"
						<< std::left << std::setw(40) <<" ARGUMENTS" 
						<< std::left << std::setw(30) <<" DESCRIPTION" 
						<< "\n";
					for ( auto const& cmd : mCommands ) {
						std::cout 
							<< std::left << std::setw(6)  << "  --cmd=" 
							<< std::left << std::setw(30) << cmd.first
							<< std::left << std::setw(40) << std::get<0>(cmd.second) 
							<< std::get<1>(cmd.second) << "\n";
					}
					std::cout << "\t--help \t Shows this page\n";
					exit( EXIT_FAILURE );
				}

				// check for missing "--cmd" => warning
				std::string prefix("--cmd=");
				if ( args[0].compare(0, prefix.size(), prefix) ) {
					std::cerr << "Please provide valid command starting with \"--cmd=\"\n";
					exit( EXIT_FAILURE );
				}

				// check for empty commands
				std::string cmd = args[0].substr(prefix.size());
				if ( cmd == "" ) {
					std::cerr << "Please provide non-empty command starting with \"--cmd=\"\n";
					exit(EXIT_FAILURE);
				}

				// check for the unknown command
				auto cmdit = mCommands.find( cmd );
				if ( cmdit == mCommands.end() ) {
					std::cerr << "Please provide valid command starting with \"--cmd=\" and having proper arguments if needed\n";
					exit(EXIT_FAILURE);
				}

				// finally, execute command!
				std::get<2>(cmdit->second)( args );
			}

		private:
			MapOfCommands mCommands{};
	};

	typedef Singleton<Cmd, CreateMeyers> CmdS;

} // namespace CLI
} // namespace NPP
