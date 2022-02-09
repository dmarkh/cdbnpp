#pragma once

#include <ostream>

namespace CDBNPP {

	class Log {
		public:
			static void setError( std::ostream* stream ) { CDBNPPErrStream = stream; }
			static void setInfo ( std::ostream* stream ) { CDBNPPInfStream = stream; }
			static void setDebug( std::ostream* stream ) { CDBNPPDbgStream = stream; }
			inline static std::ostream* CDBNPPErrStream{nullptr};
			inline static std::ostream* CDBNPPInfStream{nullptr};
			inline static std::ostream* CDBNPPDbgStream{nullptr};
	};

#define CDBNPP_RESET  "\033[0m"
#define CDBNPP_RED    "\033[1m\033[31m"
#define CDBNPP_GREEN  "\033[1m\033[32m"
#define CDBNPP_BLUE   "\033[1m\033[34m"

#ifdef __GNUG__
#	define CDBNPP_FUNCTION __PRETTY_FUNCTION__
#else
#	define CDBNPP_FUNCTION __func__
#endif

#define CDBNPP_LOG_ERROR \
	if ( Log::CDBNPPErrStream ) \
	*Log::CDBNPPErrStream << CDBNPP_RED << CDBNPP_FUNCTION << ", " << __LINE__ << " [ERROR] " << CDBNPP_RESET

#define CDBNPP_LOG_INFO \
	if ( Log::CDBNPPInfStream ) \
	*Log::CDBNPPInfStream << CDBNPP_RESET << CDBNPP_FUNCTION << ", " << __LINE__ << " [INFO] " << CDBNPP_RESET

#define CDBNPP_LOG_DEBUG \
	if ( Log::CDBNPPDbgStream ) \
	*Log::CDBNPPDbgStream << CDBNPP_BLUE << CDBNPP_FUNCTION << ", " << __LINE__ << " [DEBUG] " << CDBNPP_RESET

#define CDBNPP_LOG_ERROR_IF(cond) \
	if ( Log::CDBNPPErrStream && (cond) ) \
	*Log::CDBNPPErrStream << CDBNPP_RED << CDBNPP_FUNCTION << ", " << __LINE__ << " [ERROR] " << CDBNPP_RESET

#define CDBNPP_LOG_INFO_IF(cond) \
	if ( Log::CDBNPPInfStream && (cond) ) \
	*Log::CDBNPPInfStream << CDBNPP_RESET << CDBNPP_FUNCTION << ", " << __LINE__ << " [INFO] " << CDBNPP_RESET

#define CDBNPP_LOG_DEBUG_IF(cond) \
	if ( Log::CDBNPPDbgStream && (cond) ) \
	*Log::CDBNPPDbgStream << CDBNPP_BLUE << CDBNPP_FUNCTION << ", " << __LINE__ << " [DEBUG] " << CDBNPP_RESET


} // namespace CDBNPP


