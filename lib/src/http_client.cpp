
#include "cdbnpp/http_client.h"

namespace CDBNPP {

	HttpClient::HttpClient() {}
	HttpClient::~HttpClient() {}

	HttpResponse HttpClient::Get( const std::string& url ) {
		std::shared_ptr<HttpCurlHolder> curl = PrepareGet( url );
		CURLcode rc = curl->Perform();
		return HttpResponse( curl, std::move(curl->mResponseString), std::move(curl->mHeaderString), rc );
	}

	std::shared_ptr<HttpCurlHolder> HttpClient::PrepareGet( const std::string& url ) {
		std::shared_ptr<HttpCurlHolder> curl = std::make_shared<HttpCurlHolder>();
		SetCommon( curl );
		curl->SetUrl( url );
		curl->SetGET();
		curl->FinalizeHeaders();
		return curl;
	}

	HttpResponse HttpClient::Post( const std::string& url, const HttpPostParams_t& params, const std::string& filename, const std::string& filedata ) {
		std::shared_ptr<HttpCurlHolder> curl = PreparePost( url, params, filename, filedata );
		CURLcode rc = curl->Perform();
		return HttpResponse( curl, std::move(curl->mResponseString), std::move(curl->mHeaderString), rc );
	}

	std::shared_ptr<HttpCurlHolder> HttpClient::PreparePost( const std::string& url, const HttpPostParams_t& params, const std::string& filename, const std::string& filedata ) {
		std::shared_ptr<HttpCurlHolder> curl = std::make_shared<HttpCurlHolder>();
		SetCommon( curl );
		curl->SetUrl( url );
		curl->SetPOST();
		curl->SetPostParams( params, filename, filedata );
		curl->FinalizeHeaders();
		return curl;
	}

	HttpResponse HttpClient::Post( const std::string& url, const std::string& body, const std::string& header ) {
		std::shared_ptr<HttpCurlHolder> curl = PreparePost( url, body, header );
		CURLcode rc = curl->Perform();
		return HttpResponse( curl, std::move(curl->mResponseString), std::move(curl->mHeaderString), rc );
	}

	std::shared_ptr<HttpCurlHolder> HttpClient::PreparePost( const std::string& url, const std::string& body, const std::string& header ) {
		std::shared_ptr<HttpCurlHolder> curl = std::make_shared<HttpCurlHolder>();
		SetCommon( curl );
		curl->SetUrl( url );
		curl->SetPOST();
		curl->SetPostParams( body );
		if ( header.length() ) {
			curl->AppendHeader( header );
		}
		curl->FinalizeHeaders();
		return curl;
	}

	HttpResponse HttpClient::Patch( const std::string& url, const std::string& body, const std::string& header ) {
		std::shared_ptr<HttpCurlHolder> curl = PreparePatch( url, body, header );
		CURLcode rc = curl->Perform();
		return HttpResponse( curl, std::move(curl->mResponseString), std::move(curl->mHeaderString), rc );
	}

	std::shared_ptr<HttpCurlHolder> HttpClient::PreparePatch( const std::string& url, const std::string& body, const std::string& header ) {
		std::shared_ptr<HttpCurlHolder> curl = std::make_shared<HttpCurlHolder>();
		SetCommon( curl );
		curl->SetUrl( url );
		curl->SetPATCH();
		curl->SetPostParams( body );
		if ( header.length() ) {
			curl->AppendHeader( header );
		}
		curl->FinalizeHeaders();
		return curl;
	}

	void HttpClient::SetCommon( std::shared_ptr<HttpCurlHolder>& curl ) {
		curl->SetVerbose( mVerbose );
		curl->SetTimeout( mTimeoutMs );
		curl->SetConnectTimeout( mConnectTimeoutMs );
		curl->SetUserAgent( mUserAgent );
		curl->SetVerifySsl( mVerifySsl );
		if ( mToken.size() ) {
			curl->AppendHeader( "Authorization: Bearer " + mToken );
		}
		curl->SetMaxRetries( mMaxRetries );
		curl->SetSleepSeconds( mSleepSeconds );
	}

} // namespace CDBNPP
