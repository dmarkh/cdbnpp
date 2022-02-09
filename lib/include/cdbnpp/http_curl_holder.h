#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <curl/curl.h>

namespace CDBNPP {

	class HttpCurlHolder;

	using HttpCurlHolderPtr_t = std::shared_ptr<HttpCurlHolder>;
	using HttpPostParams_t = std::vector<std::pair<std::string,std::string>>;

	class HttpCurlHolder {

		public:

			HttpCurlHolder();
			HttpCurlHolder(const HttpCurlHolder& other) = default;
			HttpCurlHolder(HttpCurlHolder&& old) noexcept = default;
			~HttpCurlHolder();

			HttpCurlHolder& operator=(HttpCurlHolder&& old) noexcept = default;
			HttpCurlHolder& operator=(const HttpCurlHolder& other) = default;

			CURL* getHandle() { return handle; }

			void SetUrl( const std::string& url );
			void SetVerbose( bool verbose );
			void SetTimeout( long timeout_ms );
			void SetConnectTimeout( long timeout_ms );
			void SetUserAgent( const std::string& ua );
			void SetVerifySsl( bool verify );

			void SetCommon();

			void SetMaxRetries( unsigned int max_retries ) { mMaxRetries = max_retries; }
			void SetSleepSeconds( unsigned int sleep_seconds ) { mSleepSeconds = sleep_seconds; }

			void SetGET();
			void SetPOST();
			void SetPATCH();

			void SetPostParams( const HttpPostParams_t& params, const std::string& filename, const std::string& filedata );
			void SetPostParams( const std::string& body );

			void AppendHeader( const std::string& header );
			void FinalizeHeaders();

			CURLcode Perform();

			std::string urlEncode(const std::string& s) const;
			std::string urlDecode(const std::string& s) const;

			std::string mResponseString; // used in write function!
			std::string mHeaderString; // used in write function!

		private:

			static std::mutex curl_easy_init_mutex_;
			CURL* handle{nullptr};
			curl_mime* mime{nullptr};
			struct curl_slist* headers{nullptr};
			std::string mUserAgent{"Conditions-Database-Client"};
			unsigned int mMaxRetries{30};
			unsigned int mSleepSeconds{30};

	}; // class HttpCurlHolder

} // namespace CDBNPP
