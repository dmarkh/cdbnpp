
#include <iostream>

#include "cdbnpp/http_response.h"
#include "cdbnpp/log.h"

namespace CDBNPP {

	HttpResponse::HttpResponse( std::shared_ptr<HttpCurlHolder> curl, std::string&& p_text, std::string&& p_header, long p_error = 0 )
		:  text(std::move(p_text)), header(std::move(p_header)), error(p_error), mCurl(std::move(curl)) {

			if ( !mCurl ) {
				std::cerr << "CDBNPP FATAL ERROR: no curl handle" << std::endl;
				std::exit(EXIT_FAILURE);
			}
			if ( !mCurl->getHandle() ) {
				std::cerr << "CDBNPP FATAL ERROR: no curl handle" << std::endl;
				std::exit(EXIT_FAILURE);
			}

			curl_easy_getinfo(mCurl->getHandle(), CURLINFO_RESPONSE_CODE, &status_code);
			curl_easy_getinfo(mCurl->getHandle(), CURLINFO_TOTAL_TIME, &elapsed);
			char* url_string{nullptr};
			curl_easy_getinfo(mCurl->getHandle(), CURLINFO_EFFECTIVE_URL, &url_string);
			url = url_string;

#if LIBCURL_VERSION_NUM >= 0x073700
			curl_easy_getinfo(mCurl->getHandle(), CURLINFO_SIZE_DOWNLOAD_T, &downloaded_bytes);
			curl_easy_getinfo(mCurl->getHandle(), CURLINFO_SIZE_UPLOAD_T, &uploaded_bytes);
#else
			double downloaded_bytes_double, uploaded_bytes_double;
			curl_easy_getinfo(mCurl->getHandle(), CURLINFO_SIZE_DOWNLOAD, &downloaded_bytes_double);
			curl_easy_getinfo(mCurl->getHandle(), CURLINFO_SIZE_UPLOAD, &uploaded_bytes_double);
			downloaded_bytes = downloaded_bytes_double;
			uploaded_bytes = uploaded_bytes_double;
#endif

		}

} // namespace CDBNPP
