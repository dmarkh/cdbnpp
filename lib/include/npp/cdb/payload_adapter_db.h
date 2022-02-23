#pragma once

#include <soci/soci.h>

#include <atomic>

#include "npp/cdb/i_payload_adapter.h"
#include "npp/cdb/tag.h"

namespace NPP {
namespace CDB {

	using namespace NPP::Util;

	class PayloadAdapterDb : public IPayloadAdapter {
		public:
			PayloadAdapterDb();
			virtual ~PayloadAdapterDb() = default;

			// GET API:
			PayloadResults_t getPayloads( const std::set<std::string>& paths, const std::vector<std::string>& flavors,
         const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime = 0, int64_t eventTime = 0, int64_t run = 0, int64_t seq = 0 ) override;
			Result<SPayloadPtr_t> getPayload( const std::string& path, const std::vector<std::string>& flavors,
				 const PathToTimeMap_t& maxEntryTimeOverrides, int64_t maxEntryTime = 0, int64_t eventTime = 0, int64_t run = 0, int64_t seq = 0 ) override;

			// SET API:
			Result<SPayloadPtr_t> prepareUpload( const std::string& path ) override;
			Result<std::string> setPayload( const SPayloadPtr_t& payload ) override;

			// ADMIN API
			Result<std::string> deactivatePayload( const SPayloadPtr_t& payload, int64_t deactiveTime ) override;

			Result<std::string> createTag( const std::string& path, int64_t tag_mode = 0 ) override;
			Result<std::string> createTag( const STagPtr_t& tag ) override;
			Result<std::string> deactivateTag( const std::string& path, int64_t deactiveTime ) override;

			Result<std::string> getTagSchema( const std::string& tag_path ) override;
			Result<bool> setTagSchema( const std::string& tag_path, const std::string& schema_json ) override;
			Result<bool> dropTagSchema( const std::string& tag_path ) override;

      Result<std::string> exportTagsSchemas( bool tags = true, bool schemas = true ) override;
      Result<bool> importTagsSchemas(const std::string& stringified_json ) override;

			// UTILITY API
			Result<std::string> downloadData( const std::string& uri ) override;

			// ADAPTER-SPECIFIC ADMIN API
			Result<bool> createDatabaseTables();
			Result<bool> dropDatabaseTables();
			std::vector<std::string> listDatabaseTables();
			std::vector<std::string> getTags( bool skipStructs = false );

		private:
			// access
			const std::string& getAccessMode() { return mAccessMode; }
			bool setAccessMode( const std::string& mode );
			bool hasAccess(const std::string& a ) {
				return mConfig["adapters"]["db"].contains( a )
					&& mConfig["adapters"]["db"][a].is_array()
					&& mConfig["adapters"]["db"][a].size() > 0;
			}

			// connection
			bool connect( const std::string& dbtype, const std::string& host, int port,
					const std::string& user, const std::string& pass, const std::string& dbname, const std::string& options );
			void disconnect();
			bool reconnect();
			bool isConnected() { return mIsConnected; }
			bool ensureConnection() { if ( isConnected() ) { return true; }; return reconnect(); }

			// metadata
			bool ensureMetadata();
			bool downloadMetadata(); // download tags and schemas into internal maps

			Result<bool> createIOVDataTables( const std::string& tablename, bool create_storage = true );
			Result<std::string> createTag( const std::string& tag_id, const std::string& tag_name, const std::string& tag_pid = "",
					const std::string& tag_tbname = "", int64_t tag_ct = 0, int64_t tag_dt = 0, int64_t tag_mode = 0 );

			bool mIsConnected{false};
			std::string mAccessMode{"get"};
			std::string mDbType{};

			std::atomic<bool> mMetadataAvailable{false};
			IdToTag_t mTags{};
			PathToTag_t mPaths{};
			std::shared_ptr<soci::session> mSession;
	};

} // namespace CDB
} // namespace NPP
