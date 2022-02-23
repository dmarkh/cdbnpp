#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <jwt/jwt.hpp>

#include "npp/util/uuid.h"
#include "npp/util/result.h"

#include "npp/cdb/payload.h"
#include "npp/cdb/upload.h"
#include "npp/cdb/tag.h"

namespace NPP {
namespace CDB {

	using namespace NPP::Util;

	class IPayloadAdapter;
	using IPayloadAdapterPtr_t = std::shared_ptr<IPayloadAdapter>;
	using PathToTimeMap_t = std::unordered_map<std::string,uint64_t>;

	class IPayloadAdapter {
		public:
			IPayloadAdapter( const std::string& id ) : mId(id) {}
			virtual ~IPayloadAdapter() = default;

			const std::string& id() { return mId; }
			const nlohmann::json& config() { return mConfig; }

			// GET API:
			virtual PayloadResults_t getPayloads( const std::set<std::string>& paths, const std::vector<std::string>& flavors,
				const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime = 0, int64_t eventTime = 0, int64_t run = 0, int64_t seq = 0 ) = 0;
			virtual Result<SPayloadPtr_t> getPayload( const std::string& path, const std::vector<std::string>& flavors,
				const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime = 0, int64_t eventTime = 0,  int64_t run = 0, int64_t seq = 0 ) = 0;

			// SET API:
			virtual Result<SPayloadPtr_t> prepareUpload( const std::string& path ) = 0;
			virtual Result<std::string> setPayload( const SPayloadPtr_t& payload ) = 0;

			// ADMIN API:
			virtual Result<std::string> deactivatePayload( const SPayloadPtr_t& payload, int64_t deactiveTime ) = 0;

			virtual Result<std::string> createTag( const std::string& path, int64_t tag_mode = 0 ) = 0;
			virtual Result<std::string> createTag( const STagPtr_t& tag ) = 0;
			virtual Result<std::string> deactivateTag( const std::string& path, int64_t deactiveTime ) = 0;

      virtual Result<std::string> getTagSchema( const std::string& tag_path ) = 0;
      virtual Result<bool> setTagSchema( const std::string& tag_path, const std::string& schema_json ) = 0;
      virtual Result<bool> dropTagSchema( const std::string& tag_path ) = 0;
			virtual Result<std::string> exportTagsSchemas( bool tags = true, bool schemas = true ) = 0;
			virtual Result<bool> importTagsSchemas(const std::string& stringified_json ) = 0;

			// UTILITY FUNCS
			virtual Result<std::string> downloadData( const std::string& uri ) = 0; // resolves "external" URIs
			virtual void setConfig( nlohmann::json config ) { mConfig = config; }

		protected:
			std::string mId;
			nlohmann::json mConfig{};
	};

} // namespace CDB
} // namespace NPP
