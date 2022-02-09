#pragma once

#include <ctime>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cdbnpp/i_payload_adapter.h"
#include "cdbnpp/payload.h"
#include "cdbnpp/payload_adapter_db.h"
#include "cdbnpp/result.h"
#include "cdbnpp/singleton.h"

namespace CDBNPP {

	class Service;

	using ServiceS = Singleton<Service, CreateMeyers>;

	class Service {

		public:
			Service() = default;
			~Service() = default;

			void init( const std::string& adapters = "memory+file+db+http" );

			// GET API:
			PayloadResults_t getPayloads( const std::set<std::string>& paths, bool fetch_data = true );
			Result<std::string> setPayload( const SPayloadPtr_t& payload );

			// SET API:
			Result<SPayloadPtr_t> prepareUpload( const std::string& path ); // new upload
			SPayloadPtr_t& prepareUpload( SPayloadPtr_t& payload ); // for re-upload

			// ADMIN API:
			Result<std::string> deactivatePayload( const SPayloadPtr_t& payload, int64_t deactiveTime );
      Result<std::string> createTag( const std::string& path, int64_t tag_mode = 0 );
			Result<std::string> createTag( const STagPtr_t& tag );
      Result<std::string> deactivateTag( const std::string& path, int64_t deactiveTime );

      Result<std::string> getTagSchema( const std::string& tag_path );
      Result<bool> setTagSchema( const std::string& tag_path, const std::string& schema_json );
      Result<bool> dropTagSchema( const std::string& tag_path );
      Result<std::string> exportTagsSchemas( bool tags = true, bool schemas = true );
      Result<bool> importTagsSchemas(const std::string& stringified_json );

			// OTHER

			int64_t maxEntryTime() { return mMaxEntryTime; }
			int64_t eventTime() { return mEventTime; }
			int64_t run() { return mRun; }
			int64_t seq() { return mSeq; }
			const std::vector<std::string>& flavors() { return mFlavors; }

			static const std::set<std::string> formats() { return { "dat", "json", "bson", "ubjson", "cbor", "msgpack" }; }

			const nlohmann::json& config() const { return mConfig; }

			// global params, mode = 1 uses ET,MET mode = 2 uses Run,Seq
			void setMaxEntryTime( int64_t maxEntryTime ) { mMaxEntryTime = maxEntryTime; }
			void setMaxEntryTime( const std::string& maxEntryTime ) { mMaxEntryTime = string_to_time( maxEntryTime ); }
			void setEventTime( int64_t eventTime ) { mEventTime = eventTime; }
			void setEventTime( const std::string& eventTime ) { mEventTime = string_to_time( eventTime ); }
			void setRun( int64_t run ) { mRun = run; }
			void setSeq( int64_t seq ) { mSeq = seq; }
			void setFlavors( const std::vector<std::string>& flavors ) { mFlavors = flavors; }
			void setConfig( const std::string& cfg ) { mConfig = nlohmann::json::parse(cfg,nullptr,false,true); }
			void setConfig( const nlohmann::json& cfg ) { mConfig = cfg; }

			bool isEnabledMemory() { return mPayloadAdapterMemory != nullptr; }
			bool isEnabledFile() { return mPayloadAdapterFile != nullptr; }
			bool isEnabledDb() { return mPayloadAdapterDb != nullptr; }
			bool isEnabledHttp() { return mPayloadAdapterHttp != nullptr; }
			std::vector<std::string> enabledAdapters();

			IPayloadAdapterPtr_t& getPayloadAdapterMemory() { return mPayloadAdapterMemory; }
			IPayloadAdapterPtr_t& getPayloadAdapterFile() { return mPayloadAdapterFile; }
			IPayloadAdapterPtr_t& getPayloadAdapterDb() { return mPayloadAdapterDb; }
			IPayloadAdapterPtr_t& getPayloadAdapterHttp() { return mPayloadAdapterHttp; }

			Result<bool> resolveURI( SPayloadPtr_t& payload );

		private:

			bool validateConfigFile();

			int64_t mEventTime{0};
			int64_t mMaxEntryTime{0};
			int64_t mRun{0};
			int64_t mSeq{0};
			std::vector<std::string> mFlavors{ {"ofl"} };

			IPayloadAdapterPtr_t mPayloadAdapterMemory{nullptr};
			IPayloadAdapterPtr_t mPayloadAdapterFile{nullptr};
			IPayloadAdapterPtr_t mPayloadAdapterDb{nullptr};
			IPayloadAdapterPtr_t mPayloadAdapterHttp{nullptr};

			std::vector<IPayloadAdapterPtr_t> mEnabledAdapters{};

			nlohmann::json mConfig{};

	};


} // namespace CDBNPP
