
#include "cdbnpp/http_curl_holder.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "cdbnpp/log.h"

namespace CDBNPP {

	size_t cdbWriteFunction(char* ptr, size_t size, size_t nmemb, std::string* data) {
		size *= nmemb;
		data->append(ptr, size);
		return size;
	}

	std::mutex HttpCurlHolder::curl_easy_init_mutex_{};

	HttpCurlHolder::HttpCurlHolder() {
		curl_easy_init_mutex_.lock();
		handle = curl_easy_init();
		curl_easy_init_mutex_.unlock();
		if ( handle ) {
			SetCommon();
		} else {
			std::cerr << "CDBNPP HttpClient FATAL ERROR: cannot get curl handle" << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}

	HttpCurlHolder::~HttpCurlHolder() {
		curl_easy_cleanup(handle);
		curl_mime_free(mime);
		curl_slist_free_all(headers);
	}

	void HttpCurlHolder::SetUrl( const std::string& url ) {
		curl_easy_setopt( handle, CURLOPT_URL, url.c_str() );
	}

	void HttpCurlHolder::SetVerbose(bool verbose) {
		curl_easy_setopt( handle, CURLOPT_VERBOSE, verbose ? 1L : 0L );
	}

	void HttpCurlHolder::SetTimeout(long timeout_ms ) {
		curl_easy_setopt( handle, CURLOPT_TIMEOUT_MS, timeout_ms );
	}

	void HttpCurlHolder::SetConnectTimeout(long timeout_ms ) {
		curl_easy_setopt( handle, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms );
	}

	void HttpCurlHolder::SetUserAgent(const std::string& ua) {
		curl_easy_setopt( handle, CURLOPT_USERAGENT, ua.c_str() );
	}

	void HttpCurlHolder::SetVerifySsl(bool verify) {
		curl_easy_setopt( handle, CURLOPT_SSL_VERIFYPEER, verify ? 1L : 0L );
		curl_easy_setopt( handle, CURLOPT_SSL_VERIFYHOST, verify ? 2L : 0L );
	}

	void HttpCurlHolder::SetCommon() {
		curl_easy_setopt( handle, CURLOPT_NOPROGRESS, 1L );
		curl_easy_setopt( handle, CURLOPT_FAILONERROR, true );
		curl_easy_setopt( handle, CURLOPT_USERAGENT, mUserAgent.c_str() );
		curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, cdbWriteFunction );
		curl_easy_setopt( handle, CURLOPT_WRITEDATA, &mResponseString );
		curl_easy_setopt( handle, CURLOPT_HEADERFUNCTION, cdbWriteFunction );
		curl_easy_setopt( handle, CURLOPT_HEADERDATA, &mHeaderString );
		curl_easy_setopt( handle, CURLOPT_ACCEPT_ENCODING, "");
		curl_easy_setopt( handle, CURLOPT_FOLLOWLOCATION, 1L );
		curl_easy_setopt( handle, CURLOPT_MAXREDIRS, 10L );
		curl_easy_setopt( handle, CURLOPT_AUTOREFERER, 1L);
	}

	void HttpCurlHolder::SetGET() {
		curl_easy_setopt( handle, CURLOPT_NOBODY, 1L);
		curl_easy_setopt( handle, CURLOPT_CUSTOMREQUEST, nullptr);
		curl_easy_setopt( handle, CURLOPT_HTTPGET, 1L);
	}

	void HttpCurlHolder::SetPOST() {
		curl_easy_setopt( handle, CURLOPT_NOBODY, 0L);
		curl_easy_setopt( handle, CURLOPT_HTTPGET, 0L);
		curl_easy_setopt( handle, CURLOPT_POST, 1L );
		curl_easy_setopt( handle, CURLOPT_CUSTOMREQUEST, "POST");
	}

	void HttpCurlHolder::SetPATCH() {
		curl_easy_setopt( handle, CURLOPT_NOBODY, 0L);
		curl_easy_setopt( handle, CURLOPT_HTTPGET, 0L);
		curl_easy_setopt( handle, CURLOPT_POST, 1L );
		curl_easy_setopt( handle, CURLOPT_CUSTOMREQUEST, "PATCH");
	}

	void HttpCurlHolder::SetPostParams( const HttpPostParams_t& params, const std::string& filename, const std::string& filedata ) {
		if ( !params.size() ) { return; }
		if ( !mime ) {
			mime = curl_mime_init( handle );
		}
		curl_mimepart *part;
		for ( const auto& [key, value] : params ) {
			part = curl_mime_addpart(mime);
			curl_mime_data( part, value.c_str(), CURL_ZERO_TERMINATED );
			curl_mime_name( part, key.c_str() );
		}
		if ( filename.length() ) {
			part = curl_mime_addpart(mime);
			curl_mime_filedata( part, filename.c_str() );
			curl_mime_name(part, "payload_file");
		} else if ( filedata.length() ) {
			part = curl_mime_addpart(mime);
			curl_mime_data( part, filedata.c_str(), filedata.length() );
			curl_mime_filename(part, "payload.dat");
			curl_mime_name(part, "payload_file");
		}
		curl_easy_setopt( handle, CURLOPT_MIMEPOST, mime);
	}

	void HttpCurlHolder::SetPostParams( const std::string& body ) {
		curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body.c_str());
		curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, -1L);
	}

	void HttpCurlHolder::AppendHeader( const std::string& header ) {
		headers = curl_slist_append(headers, header.c_str());
	}

	void HttpCurlHolder::FinalizeHeaders() {
		curl_easy_setopt( handle, CURLOPT_HTTPHEADER, headers );
	}

	CURLcode HttpCurlHolder::Perform() {
		CURLcode rc = CURLE_OK;
		long http_code;
		for ( unsigned int i = 0; i < ( mMaxRetries + 1 ); ++i ) {
			rc = curl_easy_perform( handle );
			http_code = 0;
			curl_easy_getinfo ( handle, CURLINFO_RESPONSE_CODE, &http_code );
			if ( rc == CURLE_OK ) { break; }
			if ( http_code < 500 ) {
				// successful request, missing endpoint, unauthorized access errors => cannot retry
				break;
			}
			// retry http error 500..599 codes => sleep for N seconds, rinse and repeat
			std::this_thread::sleep_for( std::chrono::milliseconds( (int)( mSleepSeconds * 1e3 ) ) );
			mResponseString = "";
			mHeaderString = "";
		}
		return rc;
	}

	std::string HttpCurlHolder::urlEncode(const std::string& s) const {
		if ( !handle ) {
			std::cerr << "CDBNPP FATAL ERROR: no curl handle" << std::endl;
			std::exit(EXIT_FAILURE);
		}
		char* output = curl_easy_escape(handle, s.c_str(), static_cast<int>(s.length()));
		if (output) {
			std::string result = output;
			curl_free(output);
			return result;
		}
		return "";
	}

	std::string HttpCurlHolder::urlDecode(const std::string& s) const {
		if ( !handle ) {
			std::cerr << "CDBNPP FATAL ERROR: no curl handle" << std::endl;
			std::exit(EXIT_FAILURE);
		}
		char* output = curl_easy_unescape(handle, s.c_str(), static_cast<int>(s.length()), nullptr);
		if (output) {
			std::string result = output;
			curl_free(output);
			return result;
		}
		return "";
	}

} // namespace CDBNPP
