#pragma once

#include <deque>
#include <memory>
#include <string>

#include "cdbnpp/i_payload_adapter.h"

#define CDBNPP_MEGABYTES 1024*1024

namespace CDBNPP {


	class PayloadAdapterMemory : public IPayloadAdapter {

		public:

			PayloadAdapterMemory();
			virtual ~PayloadAdapterMemory() = default;

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

			// UTILITY:
			Result<std::string> downloadData( const std::string& uri ) override;

			// OTHER
			size_t cacheSize() { return mCacheSizeBytes; }
			size_t cacheItemCount() { return mCache.size(); }
			void setCacheSizeLimit( size_t lo, size_t hi ) { mCacheSizeLimitLo = lo; mCacheSizeLimitHi = hi; }
			void setCacheItemLimit( size_t lo, size_t hi ) { mCacheItemLimitLo = lo; mCacheSizeLimitHi = hi; }

		private:

			bool maintainCacheWithinLimits();

			std::deque<SPayloadPtr_t> mCache{};
			size_t mCacheSizeBytes{0};
			size_t mCacheSizeLimitLo{ 50 * CDBNPP_MEGABYTES};
			size_t mCacheSizeLimitHi{100 * CDBNPP_MEGABYTES};
			size_t mCacheItemLimitLo{ 5000};
			size_t mCacheItemLimitHi{10000};

	};

} // namespace CDBNPP
