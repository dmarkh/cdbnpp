#pragma once

#include <algorithm>
#include <cctype>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace CDBNPP {

	template<class T>
		T from_string(const std::string& s)
		{
			std::istringstream stream (s);
			T t;
			stream >> t;
			return t;
		}

	inline bool is_integer(const std::string & s) {
		if ( s.empty() ) { return false; }
		int pos = 0;
		if ( s[0] == '-' || s[0] == '+' ) { pos = 1; }
		return ( s.find_first_not_of( "0123456789", pos ) == std::string::npos );
	}

	inline void rtrim(std::string& str, const std::string& chr = " \n\r\t\v") {
		size_t endpos = str.find_last_not_of(chr);
		if( std::string::npos != endpos ) {
			str = str.substr( 0, endpos+1 );
		}
	}

	inline void ltrim(std::string& str, const std::string& chr = " \n\r\t\v") {
		size_t startpos = str.find_first_not_of(chr);
		if( std::string::npos != startpos ) {
			str = str.substr( startpos, str.size() - startpos);
		}
	}

	inline void trim(std::string& str, const std::string& chr = " \n\r\t\v") {
		ltrim( str, chr );
		rtrim( str, chr );
	}

	inline void trim(std::vector<std::string>& str, const std::string& chr = " \n\r\t\v") {
		for (std::vector<std::string>::iterator iter = str.begin(); iter != str.end(); ++iter) {
			rtrim(*iter, chr);
			ltrim(*iter, chr);
		}
	}

	inline std::string rtrim( const std::string& str, const std::string& chr = " \n\r\t\v") {
		std::string ret(str);
		rtrim( ret, chr );
		return ret;
	}

	inline std::string ltrim( const std::string& str, const std::string& chr = " \n\r\t\v") {
		std::string ret(str);
		ltrim( ret, chr );
		return ret;
	}

	inline std::string trim( const std::string& str, const std::string& chr = " \n\r\t\v") {
		std::string ret = rtrim(str, chr);
		ltrim(ret, chr);
		return ret;
	}

	inline void sanitize_alnumuscore(std::string& str) {
		str.erase( std::remove_if( str.begin(), str.end(),
					[](char const c) { return !( std::isalnum(c) || '_' == c ); } ), str.end() );
	}

	inline void sanitize_alnumslashuscore(std::string& str) {
		str.erase( std::remove_if( str.begin(), str.end(),
					[](char const c) { return !( std::isalnum(c) || '/' == c || '_' == c ); } ), str.end() );
	}

	inline std::string sanitize_alnumuscore(const std::string& str) {
		std::string ret(str);
		sanitize_alnumuscore(ret);
		return ret;
	}

	inline void sanitize_alnum(std::string& str) {
		str.erase( std::remove_if( str.begin(), str.end(),
					[](char const c) { return !std::isalnum(c); } ), str.end() );
	}

	inline std::string sanitize_alnum(const std::string& str) {
		std::string ret(str);
		sanitize_alnum(ret);
		return ret;
	}

	inline void sanitize_alnumslash(std::string& str) {
		str.erase( std::remove_if( str.begin(), str.end(),
					[](char const c) { return !( std::isalnum(c) || '/' == c ); } ), str.end() );
	}

	inline void sanitize_alnumdash(std::string& str) {
		str.erase( std::remove_if( str.begin(), str.end(),
					[](char const c) { return !( std::isalnum(c) || '-' == c ); } ), str.end() );
	}

	inline std::string sanitize_alnumslash(const std::string& str) {
		std::string ret(str);
		sanitize_alnumslash(ret);
		return ret;
	}

	inline std::string sanitize_alnumdash(const std::string& str) {
		std::string ret(str);
		sanitize_alnumdash(ret);
		return ret;
	}

	inline std::string sanitize_alnumslashuscore(const std::string& str) {
		std::string ret(str);
		sanitize_alnumslashuscore(ret);
		return ret;
	}

	inline int64_t tz_offset() {
		int64_t now = time(NULL);
		struct tm lres;
		struct tm gres;
		localtime_r(&now,&lres);
		gmtime_r(&now,&gres);
		int64_t local = mktime(&lres);
		int64_t gmt = mktime(&gres);
		return (gmt - local);
	}

	inline const std::string unixtime_to_utc_date( int64_t ut, const std::string& format = "%Y%m%dT%H%M%S" ) {
		struct tm res{};
		gmtime_r(&ut, &res);
		char date[20];
		strftime(date, sizeof(date), format.c_str(), &res);
		return std::string( date );
	}

	inline int64_t utc_date_to_unixtime( const std::string& dt, const std::string& format = "%Y%m%dT%H%M%S" ) {
		struct std::tm tm{};
		std::istringstream ss( dt );
		ss >> std::get_time( &tm, format.c_str() );
		int64_t res = mktime(&tm);
		return res - tz_offset();
	}

	inline int32_t string_to_int( const std::string& str ) {
    if ( is_integer( str ) ) {
      return std::stoi( str );
		}
		return 0;
	}

	inline int64_t string_to_longlong( const std::string& str ) {
    if ( is_integer( str ) ) {
      return std::stoll( str );
		}
		return 0;
	}

	inline int64_t string_to_time( const std::string& str ) {
    if ( is_integer( str ) ) {
      return std::stoll( str );
    } else {
      return utc_date_to_unixtime( str, "%Y%m%dT%H%M%S" );
    }
	}

	inline std::string file_get_contents( const std::string& filename ) {
		std::ifstream file( filename, std::ios::in | std::ios::binary);
		if (!file.is_open()) {
			return "";
		}
		const std::size_t& size = std::filesystem::file_size( filename );
		std::string content(size, '\0');
		file.read( content.data(), size );
		file.close();
		return content;
	}

	inline bool file_put_contents( const std::string& filename, const std::string& content ) {
		std::ofstream file( filename, std::ios::out | std::ios::binary);
		if (!file.is_open()) {
			return false;
		}
		file.write( content.data(), content.size() );
		file.close();
		return true;
	}

	inline std::string file_get_contents_c( const std::string& filename ) {
		std::FILE *fp = std::fopen(filename.c_str(), "rb");
		if (fp) {
			std::string contents;
			std::fseek(fp, 0, SEEK_END);
			contents.resize(std::ftell(fp));
			std::rewind(fp);
			std::fread(&contents[0], 1, contents.size(), fp);
			std::fclose(fp);
			return contents;
		}
		return "";
	}


	inline void string_to_lower_case( std::string& str ) {
		std::transform( str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); } );
	}

	inline void string_to_upper_case( std::string& str ) {
		std::transform( str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::toupper(c); } );
	}

	inline std::string string_to_lower_case( const std::string& stri ) {
		std::string str = stri;
		std::transform( str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); } );
		return str;
	}

	inline std::string string_to_upper_case( const std::string& stri ) {
		std::string str = stri;
		std::transform( str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::toupper(c); } );
		return str;
	}

	inline bool string_starts_with( const std::string& haystack, const std::string& needle ) {
		return ( haystack.compare(0, needle.size(), needle) == 0 );
	}

	inline bool string_ends_with( const std::string& haystack, const std::string& needle ) {
		return ( haystack.compare( haystack.size() - needle.size(), needle.size(), needle ) == 0 );
	}

	inline bool string_contains( const std::string& haystack, const std::string& needle ) {
		return ( haystack.find( needle ) != std::string::npos );
	}

	template <typename T>
		std::string implode(const std::vector<T>& v, const std::string& delim)
		{
			if ( !v.size() ) { return ""; }
			std::ostringstream o;
			std::copy(v.begin(), --v.end(), std::ostream_iterator<T>(o, delim.c_str()));
			o << *(--v.end());
			return o.str();
		}

	inline std::vector<std::string> explode( const std::string& str, const std::string& delim ) {
		std::vector<std::string> result;
		if ( !str.size() || !delim.size() ) { return result; }
		size_t last = 0, next = 0, sz = delim.size();
		while ( ( next = str.find( delim, last ) ) != std::string::npos ) {
			result.push_back( str.substr(last, next - last) );
			last = next + sz;
		}
		result.push_back( str.substr( last ) );
		return result;
	}

	inline std::set<std::string> explode_to_set( const std::string& str, const std::string& delim ) {
		std::set<std::string> result;
		if ( !str.size() || !delim.size() ) { return result; }
		size_t last = 0, next = 0, sz = delim.size();
		while ( ( next = str.find( delim, last ) ) != std::string::npos ) {
			result.insert( str.substr(last, next - last) );
			last = next + sz;
		}
		result.insert( str.substr( last ) );
		return result;
	}

	inline std::vector<std::string> explode_and_trim( const std::string& str, const std::string& delim ) {
		std::vector<std::string> result;
		if ( !str.size() || !delim.size() ) { return result; }
		size_t last = 0, next = 0, sz = delim.size();
		while ( ( next = str.find( delim, last ) ) != std::string::npos ) {
			result.push_back( trim( str.substr(last, next - last) ) );
			last = next + sz;
		}
		result.push_back( trim( str.substr( last ) ) );
		return result;
	}

	inline std::set<std::string> explode_to_set_and_trim( const std::string& str, const std::string& delim ) {
		std::set<std::string> result;
		if ( !str.size() || !delim.size() ) { return result; }
		size_t last = 0, next = 0, sz = delim.size();
		while ( ( next = str.find( delim, last ) ) != std::string::npos ) {
			result.insert( trim( str.substr(last, next - last) ) );
			last = next + sz;
		}
		result.insert( trim( str.substr( last ) ) );
		return result;
	}

	inline std::vector<std::string> explode( const std::string& str, const char& delim ) {
		std::vector<std::string> result;
		if ( !str.size() ) { return result; }
		std::string token;
		std::istringstream iss(str);
		while ( getline(iss, token, delim) ) {
			result.push_back(token);
		}
		return result;
	}

	inline std::set<std::string> explode_to_set( const std::string& str, const char& delim ) {
		std::set<std::string> result;
		if ( !str.size() ) { return result; }
		std::string token;
		std::istringstream iss(str);
		while ( getline(iss, token, delim) ) {
			result.insert(token);
		}
		return result;
	}

	inline std::vector<std::string> explode_and_trim(const std::string& str, const char& delim) {
		std::vector<std::string> result;
		if ( !str.size() ) { return result; }
		std::string token;
		std::istringstream iss(str);
		while ( getline(iss, token, delim) ) {
			trim(token);
			result.push_back(token);
		}
		return result;
	}

	inline std::set<std::string> explode_to_set_and_trim(const std::string& str, const char& delim) {
		std::set<std::string> result;
		if ( !str.size() ) { return result; }
		std::string token;
		std::istringstream iss(str);
		while ( getline(iss, token, delim) ) {
			trim(token);
			result.insert(token);
		}
		return result;
	}

	inline std::vector<std::string> explode_and_trim_and_sanitize(const std::string& str, const char& delim) {
		std::vector<std::string> result;
		if ( !str.size() ) { return result; }
		std::string token;
		std::istringstream iss(str);
		while ( getline(iss, token, delim) ) {
			trim(token);
			sanitize_alnum(token);
			result.push_back(token);
		}
		return result;
	}

	template<class T>
		bool vector_contains( const std::vector<T>& vec, const T& item ) {
			return std::find( vec.begin(), vec.end(), item ) != vec.end();
		}

	template<class T>
	std::set<std::string> get_map_keys( const std::map<std::string,T>& m ) {
		std::set<std::string> res{};
		std::transform( m.begin(), m.end(), std::inserter( res, res.end() ),
    		[]( const auto& pair ) { return pair.first; });
		return res;
	}

	template< typename ContainerT, typename PredicateT >
  void container_erase_if( ContainerT& items, const PredicateT& predicate ) {
    for( auto it = items.begin(); it != items.end(); ) {
      if( predicate(*it) ) it = items.erase(it);
      else ++it;
    }
  }

} // namespace CDBNPP
