
#include "cdbnpp/service.h"

#include <iostream>
#include <unordered_set>

#include "cdbnpp/json_schema.h"
#include "cdbnpp/log.h"
#include "cdbnpp/payload_adapter_memory.h"
#include "cdbnpp/payload_adapter_file.h"
#include "cdbnpp/payload_adapter_db.h"
#include "cdbnpp/payload_adapter_http.h"
#include "cdbnpp/util.h"
#include "cdbnpp/uuid.h"

namespace CDBNPP {

	void Service::init( const std::string& adapters ) {
		if ( !mConfig.empty() && mConfig != nlohmann::json::value_t::null ) {
			if ( mConfig.is_discarded() ) {
				std::cerr << "CDBNPP FATAL ERROR: config file is set but is malformed" << std::endl;
				std::exit(EXIT_FAILURE);
			} else if ( !validateConfigFile() ) {
				std::cerr << "CDBNPP FATAL ERROR: config file is set but fails schema validation:" << std::endl;
				std::exit(EXIT_FAILURE);
			}
		} else {
			// search for a config file

			// look for a config in the current directory first
			std::vector<std::string> conf_paths;

			// check current directory first
			std::string local_cdbnpp_config = std::filesystem::current_path().string() + std::string("/.cdbnpp.json");
			conf_paths.push_back( local_cdbnpp_config );

			// check home directory next
			const char* env_home_cdbnpp_config = std::getenv("HOME");
			if ( env_home_cdbnpp_config && strlen( env_home_cdbnpp_config ) > 1 ) {
				std::string home_cdbnpp_config = std::string( env_home_cdbnpp_config ) + std::string("/.cdbnpp.json");
				conf_paths.push_back( home_cdbnpp_config );
			};

			// finally, check env var
			const char* env_cdbnpp_config = std::getenv("CDBNPP_CONFIG");
			if ( env_cdbnpp_config && strlen( env_cdbnpp_config ) > 1 ) { conf_paths.push_back( env_cdbnpp_config ); };

			for ( const auto& path : conf_paths ) {
				if ( !std::filesystem::exists( path ) ) { continue; }
				std::string conf = file_get_contents( path );
				if ( empty( conf ) ) { continue; }
				try {
					mConfig = nlohmann::json::parse( conf, nullptr, false, false );
				} catch( std::exception const & e ) {
					// CDBNPP config file found but is not a valid json, skipping
					mConfig = nullptr;
					continue;
				}
				if ( mConfig.is_discarded() ) {
					// CDBNPP config file found but is malformed, skipping
					mConfig = nullptr;
					continue;
				}
				if ( !validateConfigFile() ) {
					// CDBNPP config file found but fails schema validation, skipping
					mConfig = nullptr;
					continue;
				}
				break;
			}

			if ( mConfig == nullptr || mConfig.is_null() || mConfig.is_discarded() ) {
				std::cerr << "CDBNPP FATAL ERROR: unable to find usable CDBNPP config file" << std::endl;
				std::exit(EXIT_FAILURE);
			}
		}

		// configure payload adapters - all of them
		mPayloadAdapterMemory = std::make_shared<PayloadAdapterMemory>();
		mPayloadAdapterMemory->setConfig( mConfig );

		mPayloadAdapterFile = std::make_shared<PayloadAdapterFile>();
		mPayloadAdapterFile->setConfig( mConfig );

		mPayloadAdapterDb = std::make_shared<PayloadAdapterDb>();
		mPayloadAdapterDb->setConfig( mConfig );

		mPayloadAdapterHttp = std::make_shared<PayloadAdapterHttp>();
		mPayloadAdapterHttp->setConfig( mConfig );

		// create a list of requested ones
		std::vector<std::string> requested_adapters = explode_and_trim_and_sanitize( string_to_lower_case(adapters), '+' );

		for ( const auto& adapter : requested_adapters ) {
			if ( adapter == "memory" ) {
				if ( mConfig["adapters"]["memory"].contains("cache_size_limit") ) {
					PayloadAdapterMemory* aptr = dynamic_cast<PayloadAdapterMemory*>( mPayloadAdapterMemory.get() );
					aptr->setCacheSizeLimit( mConfig["adapters"]["memory"]["cache_size_limit"]["lo"], mConfig["adapters"]["memory"]["cache_size_limit"]["hi"] );
				}
				if ( mConfig["adapters"]["memory"].contains("cache_item_limit") ) {
					PayloadAdapterMemory* aptr = dynamic_cast<PayloadAdapterMemory*>( mPayloadAdapterMemory.get() );
					aptr->setCacheItemLimit( mConfig["adapters"]["memory"]["cache_item_limit"]["lo"], mConfig["adapters"]["memory"]["cache_item_limit"]["hi"] );
				}
				mEnabledAdapters.push_back( mPayloadAdapterMemory );
			} else if ( adapter == "file" ) {
				mEnabledAdapters.push_back( mPayloadAdapterFile );
			} else if ( adapter == "db" ) {
				mEnabledAdapters.push_back( mPayloadAdapterDb );
			} else if ( adapter == "http" ) {
				mEnabledAdapters.push_back( mPayloadAdapterHttp );
			}
		}
	}

	std::vector<std::string> Service::enabledAdapters() {
		std::vector<std::string> res{};
		std::transform( mEnabledAdapters.begin(), mEnabledAdapters.end(), std::back_inserter(res),
				[](auto& adapter ) -> std::string { return adapter->id(); });
		return res;
	}

	PayloadResults_t Service::getPayloads( const std::set<std::string>& paths, bool fetch_data ) {
		PayloadResults_t res{};

		std::set<std::string> remaining_paths = paths;

		for ( auto& adapter : mEnabledAdapters ) {
			PayloadResults_t resolved_paths = adapter->getPayloads( remaining_paths, mFlavors, mMaxEntryTimeOverrides, mMaxEntryTime, mEventTime, mRun, mSeq );
			for ( const auto& [key, value] : resolved_paths ) {
				size_t num = remaining_paths.erase( key );
				if ( num == 0 ) {
					remaining_paths.erase( value->flavor() + ":" + key );
				}
				bool ok = res.insert({ value->directory() + "/" + value->structName(), value }).second;
				if ( !ok ) {
					CDBNPP_LOG_DEBUG << "WARNING: " << value->flavor() + ":" + value->directory() + "/" + value->structName() << " was already resolved, cannot insert again!" << std::endl;
				}
				if ( adapter->id() != "memory" && adapter->id() != "file" && mPayloadAdapterMemory != nullptr ) {
					mPayloadAdapterMemory->setPayload( value );
				}
			}
			if ( !remaining_paths.size() ) {
				break;
			}
		}

		if ( fetch_data ) {
			for ( auto& [ key, value ] : res ) {
				if ( !value->data().size() ) {
					resolveURI( value );
				}
			}
		}

		return res;
	}

	Result<std::string> Service::setPayload( const SPayloadPtr_t& payload ) {
		Result<std::string> res;
		for ( auto& adapter : mEnabledAdapters ) {
			if ( adapter->id() == "memory" ) { continue; }
			res = adapter->setPayload( payload );
			if ( res.valid() ) {
				return res;
			}
		}
		return res;
	}

	Result<SPayloadPtr_t> Service::prepareUpload( const std::string& path ) {
		Result<SPayloadPtr_t> res;
		for ( auto& adapter : mEnabledAdapters ) {
			if ( adapter->id() != "memory" ) {
				res = adapter->prepareUpload( path );
				if ( res.valid() ) { break; }
			}
		}
		return res;
	}

	SPayloadPtr_t& prepareUpload( SPayloadPtr_t& payload ) {
		payload->setId( generate_uuid() ); // regenerate uuid
		payload->setCreateTime( time(0) ); // set create time to NOW
		payload->setDeactiveTime( 0 ); // this entry is not deactivated yet
		payload->setBeginTime( 0 );
		payload->setEndTime( 0 );
		payload->setRun( 0 );
		payload->setSeq( 0 );
		payload->setURI( "" );
		payload->clearData();

		return payload;
	}

	Result<std::string> Service::deactivatePayload( const SPayloadPtr_t& payload, int64_t deactiveTime ) {
		Result<std::string> res;
		for ( auto& adapter : mEnabledAdapters ) {
			res = adapter->deactivatePayload( payload, deactiveTime );
			if ( res.valid() ) { break; }
		}
		return res;
	}

	Result<std::string> Service::createTag( const std::string& path, int64_t tag_mode ) {
		Result<std::string> res;
		for ( auto& adapter : mEnabledAdapters ) {
			res = adapter->createTag( path, tag_mode );
			if ( res.valid() ) { break; }
		}
		return res;
	}

	Result<std::string> Service::createTag( const STagPtr_t& tag ) {
		Result<std::string> res;
		for ( auto& adapter : mEnabledAdapters ) {
			res = adapter->createTag( tag );
			if ( res.valid() ) { break; }
		}
		return res;
	}

	Result<std::string> Service::deactivateTag( const std::string& path, int64_t deactiveTime ) {
		Result<std::string> res;
		for ( auto& adapter : mEnabledAdapters ) {
			res = adapter->deactivateTag( path, deactiveTime );
			if ( res.valid() ) { break; }
		}
		return res;
	}

	Result<std::string> Service::getTagSchema( const std::string& tag_path ) {
		Result<std::string> res;
		for ( auto& adapter : mEnabledAdapters ) {
			res = adapter->getTagSchema( tag_path );
			if ( res.valid() ) { break; }
		}
		return res;
	}

	Result<bool> Service::setTagSchema( const std::string& tag_path, const std::string& schema_json ) {
		Result<bool> res;
		for ( auto& adapter : mEnabledAdapters ) {
			res = adapter->setTagSchema( tag_path, schema_json );
			if ( res.valid() ) { break; }
		}
		return res;
	}

	Result<bool> Service::dropTagSchema( const std::string& tag_path ) {
		Result<bool> res;
		for ( auto& adapter : mEnabledAdapters ) {
			res = adapter->dropTagSchema( tag_path );
			if ( res.valid() ) { break; }
		}
		return res;
	}

	Result<std::string> Service::exportTagsSchemas( bool tags, bool schemas ) {
		Result<std::string> res;
		for ( auto& adapter : mEnabledAdapters ) {
			res = adapter->exportTagsSchemas( tags, schemas );
			if ( res.valid() ) { break; }
		}
		return res;
	}

	Result<bool> Service::importTagsSchemas(const std::string& stringified_json ) {
		Result<bool> res;
		for ( auto& adapter : mEnabledAdapters ) {
			res = adapter->importTagsSchemas( stringified_json );
			if ( res.valid() ) { break; }
		}
		return res;
	}

	Result<bool> Service::resolveURI( SPayloadPtr_t& payload ) {
		Result<bool> res;
		if ( payload->dataSize() ) {
			res.setMsg("payload already contains data");
			return res;
		}

		const std::string& uri = payload->URI();
		auto parts = explode( uri, "://" );
		if ( parts.size() != 2 ) {
			res.setMsg("bad uri encountered: " + uri );
			return res;
		}

		string_to_lower_case( parts[0] );
		sanitize_alnum( parts[0] );

		Result<std::string> data;
		if ( parts[0] == "file" ) {
			data = mPayloadAdapterFile->downloadData( uri );
		} else if ( parts[0] == "http" || parts[0] == "https" ) {
			data = mPayloadAdapterHttp->downloadData( uri );
		} else if ( parts[0] == "db" ) {
			data = mPayloadAdapterDb->downloadData( uri );
		} else {
			res.setMsg("unknown uri");
		}

		if ( data.valid() ) {
			auto fparts = explode( parts[1], "." );
			std::string fmt = fparts.back();
			sanitize_alnum(fmt);
			string_to_lower_case(fmt);
			payload->setData( data.get(), fmt );
		}

		return res;
	}

	bool Service::validateConfigFile() {
		std::string config_schema = R"(
{
  "$id":"file://.cdbnpp_config.json",
  "$schema":"https://json-schema.org/draft/2020-12/schema",
  "description":"CDBNPP Config File",
  "type":"object",
  "required":[
    "adapters"
  ],
  "properties":{
    "adapters":{
      "type":"object",
      "properties":{
        "memory":{
          "type":"object",
          "required":[
            "cache_size_limit",
            "cache_item_limit"
          ],
          "properties":{
            "cache_size_limit":{
              "type":"object",
              "properties":{
                "lo":{
                  "type":"integer",
                  "min":0
                },
                "hi":{
                  "type":"integer",
                  "min":0
                }
              }
            },
            "cache_item_limit":{
              "type":"object",
              "properties":{
                "lo":{
                  "type":"integer",
                  "min":0
                },
                "hi":{
                  "type":"integer",
                  "min":0
                }
              }
            }
          }
        },
        "file":{
          "type":"object",
          "required":[
            "dirname"
          ],
          "properties":{
            "dirname":{
              "type":"string"
            }
          }
        },
        "http":{
          "type":"object",
          "required":[
            "get",
            "set",
						"admin",
						"config"
          ],
          "properties":{
            "get":{
              "type":"array",
              "items":{
		"type":"object",
			"required":[
				"url",
			"user",
			"pass"
			],
			"properties":{
				"url":{
					"type":"string"
				},
				"user":{
					"type":"string"
				},
				"pass":{
					"type":"string"
				}
			}
	}
	},
	     "set":{
		     "type":"array",
		     "items":{
			     "type":"object",
			     "required":[
				     "url",
			     "user",
			     "pass"
			     ],
			     "properties":{
				     "url":{
					     "type":"string"
				     },
				     "user":{
					     "type":"string"
				     },
				     "pass":{
					     "type":"string"
				     }
			     }
		     }
	     },
	     "admin":{
		     "type":"array",
		     "items":{
			     "type":"object",
			     "required":[
				     "url",
			     "user",
			     "pass"
			     ],
			     "properties":{
				     "url":{
					     "type":"string"
				     },
				     "user":{
					     "type":"string"
				     },
				     "pass":{
					     "type":"string"
				     }
			     }
		     }
	     }
	}
	},
	     "db":{
		     "type":"object",
		     "required": [
			     "get",
		     "set",
		     "admin"
		     ],
		     "properties":{
			     "get":{
				     "type":"array",
				     "items":{
					     "type":"object",
					     "required":[ "dbtype" ],
					     "properties":{
						     "dbtype":{
							     "type":"string"
						     },
						     "host":{
							     "type":"string"
						     },
						     "port":{
							     "type":"number"
						     },
						     "user":{
							     "type":"string"
						     },
						     "pass":{
							     "type":"string"
						     },
						     "dbname":{
							     "type":"string"
						     },
						     "options":{
							     "type":"string"
						     }
					     }
				     }
			     },
			     "set":{
				     "type":"array",
				     "items":{
					     "type":"object",
					     "required":[ "dbtype" ],
					     "properties":{
						     "dbtype":{
							     "type":"string"
						     },
						     "host":{
							     "type":"string"
						     },
						     "port":{
							     "type":"number"
						     },
						     "user":{
							     "type":"string"
						     },
						     "pass":{
							     "type":"string"
						     },
						     "dbname":{
							     "type":"string"
						     },
						     "options":{
							     "type":"string"
						     }
					     }
				     }
			     },
			     "admin":{
				     "type":"array",
				     "items":{
					     "type":"object",
					     "required":[ "dbtype" ],
					     "properties":{
						     "dbtype":{
							     "type":"string"
						     },
						     "host":{
							     "type":"string"
						     },
						     "port":{
							     "type":"number"
						     },
						     "user":{
							     "type":"string"
						     },
						     "pass":{
							     "type":"string"
						     },
						     "dbname":{
							     "type":"string"
						     },
						     "options":{
							     "type":"string"
						     }
					     }
				     }
			     }
		     }
	     }
	}
	}
	}
	}
)";

return validate_json_using_schema( mConfig, config_schema );
}

} // namespace CDBNPP
