#pragma once

#include "npp/cdb/i_payload_adapter.h"

namespace NPP {
namespace CDB {

	using namespace NPP::Util;

	using DecodedFileNameTuple = std::tuple<std::string /* flavor */, int64_t /* ct */, int64_t /* bt */, int64_t /* et */, int64_t /* dt */, int64_t /* run */, int64_t /* seq */, int64_t /* is_binary */, bool /* is_valid */>;

	class PayloadAdapterFile : public IPayloadAdapter {
		public:
			PayloadAdapterFile();
			virtual ~PayloadAdapterFile() = default;

			// GET API:
			PayloadResults_t getPayloads( const std::set<std::string>& paths, const std::vector<std::string>& flavors,
        const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime = 0, int64_t eventTime = 0, int64_t run = 0, int64_t seq = 0 ) override;
			Result<SPayloadPtr_t> getPayload( const std::string& path, const std::vector<std::string>& flavors,
				const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime = 0, int64_t eventTime = 0, int64_t run = 0, int64_t seq = 0 ) override;

			// SET API:
			Result<SPayloadPtr_t> prepareUpload( const std::string& path ) override;
			Result<std::string> setPayload( const SPayloadPtr_t& payload ) override;

			// ADMIN API:
			Result<std::string> deactivatePayload( const SPayloadPtr_t& payload, int64_t deactiveTime ) override;

			Result<std::string> createTag( const std::string& path, int64_t tag_mode = 0 ) override;
			Result<std::string> createTag( const STagPtr_t& tag ) override;
			Result<std::string> deactivateTag( const std::string& path, int64_t deactiveTime ) override;

      Result<std::string> getTagSchema( const std::string& tag_path ) override;
      Result<bool> setTagSchema( const std::string& tag_path, const std::string& schema_json ) override;
      Result<bool> dropTagSchema( const std::string& tag_path ) override;

      Result<std::string> exportTagsSchemas( bool tags = true, bool schemas = true ) override;
      Result<bool> importTagsSchemas(const std::string& stringified_json ) override;


			// UTILITY API:
			Result<std::string> downloadData( const std::string& uri ) override;

		private:
			DecodedFileNameTuple decodeFilename( const std::string& filename );
	};

} // namespace CDB
} // namespace NPP
