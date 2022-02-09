#pragma once

#include "cdbnpp/i_payload_adapter.h"

namespace CDBNPP {

	using DecodedFileNameTuple = std::tuple<std::string, std::string, int64_t, int64_t, int64_t, int64_t, int64_t, int64_t, int64_t, bool>;

	class PayloadAdapterFile : public IPayloadAdapter {

		public:

			PayloadAdapterFile();
			virtual ~PayloadAdapterFile() = default;

			// GET API:
			PayloadResults_t getPayloads( const std::set<std::string>& paths, const std::vector<std::string>& flavors,
          int64_t eventTime = 0, int64_t maxEntryTime = 0, int64_t run = 0, int64_t seq = 0 ) override;
			Result<SPayloadPtr_t> getPayload( const std::string& path, const std::vector<std::string>& flavors,
				int64_t eventTime = 0, int64_t maxEntryTime = 0, int64_t run = 0, int64_t seq = 0 ) override;

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

} // namespace CDBNPP
