#pragma once

#include <random>
#include <string>
#include <vector>

#include <picosha2/picosha2.h>
#include <sole/sole.h>

namespace CDBNPP {

	inline std::string generate_uuid() {
		return sole::uuid1().str(); // time-based uuid
	}

  inline std::string uuid_from_str( const std::string& inputstr ) {
		// modified SHA256 version
		std::string seedstr = "CDBNPP" + inputstr;
    std::vector<unsigned char> hash( 18 );
    picosha2::hash256( seedstr.begin(), seedstr.end(), hash.begin(), hash.end() );
    std::string res = picosha2::bytes_to_hex_string( hash.begin(), hash.end() );
    res[8] = '-'; res[13] = '-'; res[18] = '-'; res[23] = '-';
		// to reproduce in other languages: generate SHA256 as a hex-string,
		// shorten hex string to 36 chars, insert dashes at 8th,13th,18th,23rd positions
    return res;
	}

  inline std::string uuid_from_str_rand( const std::string& input ) {
		// seeded MT/rand version
		std::string seedstr = "CDBNPP" + input;
    const char* digits = "0123456789abcdef";
    std::seed_seq sseq( seedstr.begin(), seedstr.end() );
    std::mt19937 gen( sseq );
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);
    std::string res(36,' ');
    std::generate( res.begin(), res.end(), [&]()->char { return digits[dis(gen)]; });
    res[8] = '-'; res[13] = '-'; res[18] = '-'; res[23] = '-';
    return res;
	}

} // namespace CDBNPP
