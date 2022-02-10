
#include "cdbnpp/payload_adapter_memory.h"

#include <algorithm>

#include "cdbnpp/log.h"

namespace CDBNPP {

	PayloadAdapterMemory::PayloadAdapterMemory() : IPayloadAdapter("memory") {}

	PayloadResults_t PayloadAdapterMemory::getPayloads( const std::set<std::string>& paths, const std::vector<std::string>& flavors,
			const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime, int64_t eventTime, int64_t run, int64_t seq ) {
		PayloadResults_t res;
		for ( const auto& path : paths ) {
			Result<SPayloadPtr_t> rc = getPayload( path, flavors, maxEntryTimeOverrides, maxEntryTime, eventTime, run, seq );
			if ( rc.valid() ) {
				res.insert({ path, rc.get() });
			}
		}
		return res;
	}

	Result<SPayloadPtr_t> PayloadAdapterMemory::getPayload( const std::string& path, const std::vector<std::string>& service_flavors,
			const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime, int64_t eventTime, int64_t run, int64_t seq ) {

		Result<SPayloadPtr_t> res;

		auto [ flavors, directory, structName, is_path_valid ] = Payload::decodePath( path );

		if ( !is_path_valid ) {
			res.setMsg( "request path has not been decoded, path: " + path );
			return res;
		}

		if ( !service_flavors.size() && !flavors.size() ) {
			res.setMsg( "request does not specify flavor, path: " + path );
			return res;
		}

		if ( !directory.size() ) {
			res.setMsg( "request does not specify path: " + path );
			return res;
		}

		if ( !structName.size() ) {
			res.setMsg( "request does not specify structName: " + path );
			return res;
		}

		std::string dirpath = directory + "/" + structName;
    // check for path-specific maxEntryTime overrides
    if ( maxEntryTimeOverrides.size() ) {
      for ( const auto& [ opath, otime ] : maxEntryTimeOverrides ) {
        if ( string_starts_with( dirpath, opath ) ) {
          maxEntryTime = otime;
          break;
        }
      }
    }

		for ( const auto& flavor : ( flavors.size() ? flavors : service_flavors ) ) {
			// "directory = directory" because clang cannot capture it, it is a c++ standard issue until c++20
			auto it = std::find_if( mCache.begin(), mCache.end(), [ &flavor, directory = directory, structName = structName, eventTime, maxEntryTime, run, seq ]( const auto& item ) {
					return ( item->flavor() == flavor && item->structName() == structName
							&& item->directory() == directory
							&& ( maxEntryTime > 0 ? ( item->createTime() <= maxEntryTime ) : true )
							&& ( maxEntryTime > 0 && item->deactiveTime() > 0 ? item->deactiveTime() < maxEntryTime : true )
							&& (
								( item->mode() == 1
									&& item->beginTime() <= eventTime && item->endTime() >= eventTime 
								) || ( item->mode() == 2
									&& item->run() == run
									&& item->seq() == seq
								)
							)
					  );
					});
			if ( it == mCache.end() ) {	continue;	}
			res = SPayloadPtr_t(*it);
			return res;
		}

		return res;
	}

	Result<std::string> PayloadAdapterMemory::setPayload( const SPayloadPtr_t& payload ) {

		Result<std::string> res;

		if ( !payload->ready() || payload->endTime() == 0 ) {
			res.setMsg("payload is not ready or endTime is not set");
			return res;
		}

		// add to cache
		mCache.push_back( payload );
		mCacheSizeBytes += payload->dataSize();
		maintainCacheWithinLimits();
		res = std::string(payload->id());

		return res;
	}

	Result<std::string> PayloadAdapterMemory::deactivatePayload( __attribute__((unused)) const SPayloadPtr_t& payload, __attribute__((unused)) int64_t deactiveTime ) {
		Result<std::string> res;
		res.setMsg( "memory adapter cannot deactivate payloads" );
		return res;
	}

	Result<SPayloadPtr_t> PayloadAdapterMemory::prepareUpload( __attribute__((unused)) const std::string& path ) {
		Result<SPayloadPtr_t> res;
		res.setMsg("memory adapter cannot prepare uploads");
		return res;
	}

	Result<std::string> PayloadAdapterMemory::createTag( __attribute__((unused)) const STagPtr_t& tag ) {
		Result<std::string> res;
		res.setMsg("memory adapter cannot create tags");
		return res;
	}

	Result<std::string> PayloadAdapterMemory::createTag( __attribute__((unused)) const std::string& path, __attribute__((unused)) int64_t tag_mode ) {
		Result<std::string> res;
		res.setMsg("memory adapter cannot create tags");
		return res;
	}

	Result<std::string> PayloadAdapterMemory::deactivateTag( __attribute__((unused)) const std::string& path, __attribute__((unused)) int64_t deactiveTime ) {
		Result<std::string> res;
		res.setMsg("memory adapter cannot deactivate tags");
		return res;
	}

	bool PayloadAdapterMemory::maintainCacheWithinLimits() {
		if ( mCache.size() <= 1 ) { return false; }
		if ( mCacheSizeBytes < mCacheSizeLimitHi && mCache.size() < mCacheItemLimitHi ) { return false; }
		// if cache size in bytes or in item count is bigger than HI limit, bring it down to LO limit
		while ( mCacheSizeBytes > mCacheSizeLimitLo || mCache.size() > mCacheItemLimitLo ) {
			size_t sz = mCache.front()->dataSize();
			mCache.pop_front();
			mCacheSizeBytes -= sz;
		}
		return true;
	}

	Result<std::string> PayloadAdapterMemory::downloadData( __attribute__((unused)) const std::string& uri ) {
		Result<std::string> res;
		res.setMsg("memory (aka caching) adapter does not resolve uri by design");
		return res;
	}

	Result<std::string> PayloadAdapterMemory::getTagSchema( __attribute__((unused)) const std::string& tag_path ) {
		Result<std::string> res;
		res.setMsg("memory adapter cannot get tag schemas");
		return res;
	}

	Result<bool> PayloadAdapterMemory::setTagSchema( __attribute__((unused)) const std::string& tag_path, __attribute__((unused)) const std::string& schema_json ) {
		Result<bool> res;
		res.setMsg("memory adapter cannot set tag schemas");
		return res;
	}

	Result<bool> PayloadAdapterMemory::dropTagSchema( __attribute__((unused)) const std::string& tag_path ) {
		Result<bool> res;
		res.setMsg("memory adapter cannot drop tag schemas");
		return res;
	}

	Result<std::string> PayloadAdapterMemory::exportTagsSchemas( __attribute__((unused)) bool tags, __attribute__((unused)) bool schemas ) {
		Result<std::string> res;
		res.setMsg("memory adapter cannot export tags and schemas");
		return res;
	}

	Result<bool> PayloadAdapterMemory::importTagsSchemas( __attribute__((unused)) const std::string& stringified_json ) {
		Result<bool> res;
		res.setMsg("memory adapter cannot import tags and schemas");
		return res;
	}

} // namespace CDBNPP
