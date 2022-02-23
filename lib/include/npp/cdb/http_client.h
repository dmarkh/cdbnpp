#pragma once

#include <memory>

#include "npp/cdb/http_curl_holder.h"
#include "npp/cdb/http_response.h"

namespace NPP {
namespace CDB {

	class HttpClient;

	using HttpClientPtr_t = std::shared_ptr<HttpClient>;

	class HttpClient {
		public:
			HttpClient();
			~HttpClient();

			void setToken( const std::string& token ) { mToken = token; }
			void setUserAgent( const std::string& userAgent ) { mUserAgent = userAgent; }
			void setVerbose( bool verbose ) { mVerbose = verbose; }
			void setTimeout( long timeout_ms ) { mTimeoutMs = timeout_ms; }
			void setConnectTimeout( long timeout_ms ) { mConnectTimeoutMs = timeout_ms; }
			void setVerifySsl( bool verifySsl ) { mVerifySsl = verifySsl; }
			void setMaxRetries( unsigned int r ) { mMaxRetries = r; }
			void setSleepSeconds( unsigned int s ) { mSleepSeconds = s; }

			HttpResponse Get( const std::string& url );
			HttpCurlHolderPtr_t PrepareGet( const std::string& url );

			HttpResponse Post( const std::string& url, const HttpPostParams_t& params, const std::string& filename = "", const std::string& filedata = "" );
			HttpCurlHolderPtr_t PreparePost( const std::string& url, const HttpPostParams_t& params, const std::string& filename = "", const std::string& filedata = "");

			HttpResponse Post( const std::string& url, const std::string& body, const std::string& header );
			HttpCurlHolderPtr_t PreparePost( const std::string& url, const std::string& body, const std::string& header );

			HttpResponse Patch( const std::string& url, const std::string& body, const std::string& header );
			HttpCurlHolderPtr_t PreparePatch( const std::string& url, const std::string& body, const std::string& header );

			std::string urlEncode(const std::string& s) const { return mEncoder.urlEncode(s); }
			std::string urlDecode(const std::string& s) const { return mEncoder.urlDecode(s); }

		private:
			void SetCommon( HttpCurlHolderPtr_t& curl_ );

			HttpCurlHolder mEncoder;
			bool mVerbose{false};
			long mTimeoutMs{1000};
			long mConnectTimeoutMs{1000};
			std::string mUserAgent{"CDBNPP-Http-Client"};
			bool mVerifySsl{false};
			std::string mToken{};
			unsigned int mMaxRetries{30};
			unsigned int mSleepSeconds{30};
	};

} // namespace CDB
} // namespace NPP
