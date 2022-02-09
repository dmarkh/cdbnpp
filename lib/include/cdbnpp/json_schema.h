#pragma once

#include <nlohmann/json.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

#include "cdbnpp/log.h"

namespace CDBNPP {

  bool inline validate_json_using_schema( const nlohmann::json& data_json, const nlohmann::json& schema_json ) {
		try {
      valijson::Schema mySchema;
      valijson::SchemaParser parser;
      valijson::adapters::NlohmannJsonAdapter mySchemaAdapter(schema_json);
      parser.populateSchema(mySchemaAdapter, mySchema);

		  valijson::Validator validator;
		  valijson::ValidationResults results;
		  valijson::adapters::NlohmannJsonAdapter myTargetAdapter(data_json);
		  if (!validator.validate(mySchema, myTargetAdapter, &results)) {
		    valijson::ValidationResults::Error error;
		    unsigned int errorNum = 1;
		    while ( results.popError(error) ) {
    		  std::string context;
		      std::vector<std::string>::iterator itr = error.context.begin();
		      for (; itr != error.context.end(); ++itr ) {
    		    context += *itr;
		      }
		      CDBNPP_LOG_ERROR << "Schema Error #" << errorNum << "\n"
    		    << "  context: " << context << "\n"
		        << "  desc:    " << error.description << std::endl;
    		  ++errorNum;
    		}
				return false;
			}
    } catch( std::exception const & e ) {
      CDBNPP_LOG_ERROR << "json schema validation exception:" << e.what() << "\n";
      return false;
    }
		return true;
	}

  bool inline validate_json_using_schema( const nlohmann::json& data_json, const std::string& schema ) {
		nlohmann::json schema_json;
    try {
      schema_json = nlohmann::json::parse( schema, nullptr, false, false );
      if ( schema_json.is_discarded() ) {
        CDBNPP_LOG_ERROR << "malformed schema json" << std::endl;
        return false;
      }
    } catch( std::exception const & e ) {
      CDBNPP_LOG_ERROR << "json schema validation exception:" << e.what() << "\n";
      return false;
    }
    return validate_json_using_schema( data_json, schema_json );
	}

  bool inline validate_json_using_schema( const std::string& data, const nlohmann::json& schema_json ) {
		nlohmann::json data_json;
    try {
      data_json = nlohmann::json::parse( data, nullptr, false, false );
      if ( data_json.is_discarded() ) {
        CDBNPP_LOG_ERROR << "malformed data json" << std::endl;
        return false;
      }
    } catch( std::exception const & e ) {
      CDBNPP_LOG_ERROR << "json schema validation exception:" << e.what() << "\n";
      return false;
    }
    return validate_json_using_schema( data_json, schema_json );
	}

  bool inline validate_json_using_schema( const std::string& data, const std::string& schema ) {
		nlohmann::json data_json;
		nlohmann::json schema_json;
    try {
      data_json = nlohmann::json::parse( data, nullptr, false, false );
      if ( data_json.is_discarded() ) {
        CDBNPP_LOG_ERROR << "malformed data json" << std::endl;
        return false;
      }
      schema_json = nlohmann::json::parse( schema, nullptr, false, false );
      if ( schema_json.is_discarded() ) {
        CDBNPP_LOG_ERROR << "malformed schema json" << std::endl;
        return false;
      }
    } catch( std::exception const & e ) {
      CDBNPP_LOG_ERROR << "json schema validation exception:" << e.what() << "\n";
      return false;
    }
    return validate_json_using_schema( data_json, schema_json );
  }

} // namespace CDBNPP
