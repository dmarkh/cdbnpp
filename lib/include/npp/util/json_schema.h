#pragma once

#include <nlohmann/json.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

#include "npp/util/log.h"
#include "npp/util/result.h"

namespace NPP {
namespace Util {

  Result<bool> inline validate_json_using_schema( const nlohmann::json& data_json, const nlohmann::json& schema_json ) {
		Result<bool> res;
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
				std::string errMsg;
		    while ( results.popError(error) ) {
    		  std::string context;
		      std::vector<std::string>::iterator itr = error.context.begin();
		      for (; itr != error.context.end(); ++itr ) {
    		    context += *itr;
		      }
					errMsg += "Schema Error #" + std::to_string(errorNum) + "\n"
    		    + "  context: " + context + "\n"
		        + "  desc:    " + error.description + "\n";
    		  ++errorNum;
    		}
				res.setMsg( errMsg );
				return res;
			}
    } catch( std::exception const & e ) {
      res.setMsg( "json schema validation exception:" + std::string(e.what()) );
      return res;
    }
		res = true;
		return res;
	}

  Result<bool> inline validate_json_using_schema( const nlohmann::json& data_json, const std::string& schema ) {
		Result<bool> res;
		nlohmann::json schema_json;
    try {
      schema_json = nlohmann::json::parse( schema, nullptr, false, false );
      if ( schema_json.is_discarded() ) {
        res.setMsg( "malformed schema json" );
        return res;
      }
    } catch( std::exception const & e ) {
      res.setMsg( "json schema validation exception:" + std::string(e.what()) );
      return res;
    }
    return validate_json_using_schema( data_json, schema_json );
	}

  Result<bool> inline validate_json_using_schema( const std::string& data, const nlohmann::json& schema_json ) {
		Result<bool> res;
		nlohmann::json data_json;
    try {
      data_json = nlohmann::json::parse( data, nullptr, false, false );
      if ( data_json.is_discarded() ) {
        res.setMsg( "malformed data json" );
        return res;
      }
    } catch( std::exception const & e ) {
      res.setMsg( "json schema validation exception:" + std::string(e.what()) );
      return res;
    }
    return validate_json_using_schema( data_json, schema_json );
	}

  Result<bool> inline validate_json_using_schema( const std::string& data, const std::string& schema ) {
		Result<bool> res;
		nlohmann::json data_json;
		nlohmann::json schema_json;
    try {
      data_json = nlohmann::json::parse( data, nullptr, false, false );
      if ( data_json.is_discarded() ) {
        res.setMsg( "malformed data json" );
        return res;
      }
      schema_json = nlohmann::json::parse( schema, nullptr, false, false );
      if ( schema_json.is_discarded() ) {
        res.setMsg( "malformed schema json" );
        return res;
      }
    } catch( std::exception const & e ) {
      res.setMsg( "json schema validation exception:" + std::string(e.what()) );
      return res;
    }
    return validate_json_using_schema( data_json, schema_json );
  }

} // namespace Util
} // namespace NPP
