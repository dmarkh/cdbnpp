#pragma once

#include <future>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "module.h"

namespace NPP {
	namespace Core {

		class Chain {
			public:
				Chain() = default;
				~Chain() = default;

				void add( const SModulePtr_t& module ) { mModules.insert({ module->id(), module }); }

				uint32_t getMaxThreads() { return mMaxThreads; }
				void setMaxThreads( uint32_t max_threads ) { mMaxThreads = std::min( max_threads, std::thread::hardware_concurrency() ); }

				int32_t execute( uint32_t n_times ) {
					int32_t rc = 0;
					for ( uint32_t i = 0; i < n_times; ++i ) {
						rc = doIteration();
						if ( rc != 0 ) {
							break;
						}
					}
					return 0;
				}

			private:
				uint32_t mMaxThreads{2};
				std::unordered_map<std::string,SModulePtr_t> mModules{};

				bool requirementsSatisfied( const SModulePtr_t& module, const std::unordered_set<std::string>& finished_modules ) {
					const std::vector<std::string>& hreqs = module->hardRequirements();
					for ( const auto& req : hreqs ) {
						if ( finished_modules.count( req ) == 0 ) {
							// ok, required module has not been processed yet
							return false;
						}
					}
					const std::vector<std::string>& sreqs = module->softRequirements();
					for ( const auto& req : sreqs ) {
						if ( mModules.count( req ) && !finished_modules.count( req ) ) {
							return false;
						}
					}
					return true;
				};

				int32_t doIteration() {
					std::unordered_set<std::string> active_modules{};
					std::unordered_map<std::string,SModulePtr_t> available_modules{};
					std::unordered_set<std::string> processed_modules{};
					std::queue<std::pair<std::string,std::future<int32_t>>> running_modules{};
					std::unordered_set<std::string> finished_modules{};

					for ( const auto& [ module_id, module ] : mModules ) {
						active_modules.insert( module_id );
					}

					do {

						while ( available_modules.size() && running_modules.size() < mMaxThreads ) {
							auto it = available_modules.begin();
							running_modules.push( std::make_pair( it->first, std::async( std::launch::async, &Module::execute, it->second ) ) );
							processed_modules.insert( it->first );
							available_modules.erase( it );
						}
						if ( running_modules.size() ) {
							running_modules.front().second.wait();
							int32_t rc = running_modules.front().second.get();
							if ( rc != 0 ) {
								// wait for running threads (if any) to finish, then abort this iteration
								while ( running_modules.size() ) {
									running_modules.front().second.wait();
									running_modules.pop();
								}
								return rc;
							}
							finished_modules.insert( running_modules.front().first );
							running_modules.pop();
						}

						for ( auto it = active_modules.begin(); it != active_modules.end(); ) {
							if ( available_modules.count( *it )
									|| processed_modules.count( *it )
									|| finished_modules.count( *it )
									|| !requirementsSatisfied( mModules[ *it ], finished_modules ) ) {
								++it;
							} else {
								available_modules.insert({ *it, mModules[ *it ] });
								it = active_modules.erase( it );
							}
						}

					} while( available_modules.size() || running_modules.size() );

					return 0;
				}
		};
	} // namespace Framework
} // namespace NPP
