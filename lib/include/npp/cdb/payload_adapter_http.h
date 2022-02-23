#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#include "npp/cdb/http_client.h"
#include "npp/cdb/i_payload_adapter.h"
#include "npp/cdb/tag.h"

namespace NPP {
namespace CDB {

	using namespace NPP::Util;

	class PayloadAdapterHttp : public IPayloadAdapter {
		public:
			PayloadAdapterHttp();
			virtual ~PayloadAdapterHttp() = default;

			// GET API:
			PayloadResults_t getPayloads( const std::set<std::string>& paths, const std::vector<std::string>& flavors,
        const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime = 0, int64_t eventTime = 0, int64_t run = 0, int64_t seq = 0 ) override;
			Result<SPayloadPtr_t> getPayload( const std::string& path, const std::vector<std::string>& flavors,
				const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime = 0, int64_t eventTime = 0, int64_t run = 0, int64_t seq = 0 ) override; // ???

			// SET API:
			Result<SPayloadPtr_t> prepareUpload( const std::string& path ) override; 
			Result<std::string> setPayload( const SPayloadPtr_t& payload ) override; // POST

			// ADMIN API:
			Result<std::string> deactivatePayload( const SPayloadPtr_t& payload, int64_t deactiveTime ) override; // POST

			Result<std::string> createTag( const std::string& path, int64_t tag_mode = 0 ) override; // POST
			Result<std::string> createTag( const STagPtr_t& tag ) override;
			Result<std::string> deactivateTag( const std::string& path, int64_t deactiveTime ) override; // POST

      Result<std::string> getTagSchema( const std::string& tag_path ) override; // GET
      Result<bool> setTagSchema( const std::string& tag_path, const std::string& schema_json ) override; // POST
			Result<bool> dropTagSchema( const std::string& tag_path ) override;

      Result<std::string> exportTagsSchemas( bool tags = true, bool schemas = true ) override;
      Result<bool> importTagsSchemas(const std::string& stringified_json ) override;

			// UTILITY API:
			Result<std::string> downloadData( const std::string& uri ) override; // GET

			// ADAPTER-SPECIFIC ADMIN API:
			Result<bool> createDatabaseTables(); // POST
			std::vector<std::string> listDatabaseTables(); // POST
			Result<bool> dropDatabaseTables(); // POST
			std::vector<std::string> getTags( bool skipStructs = false ); // = downloadMetadata, GET

			// OTHER
			bool hasAccess(const std::string& a ) {
				return mConfig.contains("adapters")
					&& mConfig["adapters"].contains("http")
					&& mConfig["adapters"]["http"].contains( a )
					&& mConfig["adapters"]["http"][a].is_array()
					&& mConfig["adapters"]["http"][a].size() > 0;
			}
      bool ensureMetadata();
			bool downloadMetadata();
			std::string generateJWT( const std::string& access = "get", uint64_t idx = 0 );

		private:
			void setHttpConfig();
			HttpResponse makeGetRequest(  const std::string& access, const std::string& url );
			HttpResponse makePostRequest( const std::string& access, const std::string& url, const HttpPostParams_t& params );

			std::atomic<bool> mMetadataAvailable{false};
      IdToTag_t mTags{};
      PathToTag_t mPaths{};

			HttpClientPtr_t mHttpClient{nullptr};
	};

} // namespace CDB
} // namespace NPP
