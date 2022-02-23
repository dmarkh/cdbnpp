#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <curl/curl.h>

#include "npp/cdb/http_curl_holder.h"

namespace NPP {
namespace CDB {

	class HttpResponse {
		public:
			long status_code{0};
			std::string text{""};
			std::string header{""};
			std::string url{""};
			long error{0};
			double elapsed{0};
			size_t uploaded_bytes{0};
			size_t downloaded_bytes{0};

			HttpResponse() = default;
			HttpResponse( HttpCurlHolderPtr_t curl, std::string&& p_text, std::string&& p_header, long p_error);

			HttpResponse(const HttpResponse& other) = default;
			HttpResponse(HttpResponse&& old) noexcept = default;
			~HttpResponse() noexcept = default;

			HttpResponse& operator=(HttpResponse&& old) noexcept = default;
			HttpResponse& operator=(const HttpResponse& other) = default;

		private:
			HttpCurlHolderPtr_t mCurl;
	}; // class HttpResponse

} // namespace CDB
} // namespace NPP
