#pragma once

#include "cdbnpp/util.h"

namespace CDBNPP {

	class Upload {

		public:

			Upload() {};
			Upload( const std::string& path = "", int64_t bt = 0, int64_t et = 0, int64_t run = 0 ) 
				: mPath(path), mBeginTime(bt), mEndTime(et), mRun(run) {};

			~Upload() = default;

			const std::string& path() { return mPath; } // ofl:Calibrations/tpc/tpcT0
			const std::string& data() { return mData; }
			int64_t beginTime() { return mBeginTime; }
			int64_t endTime() { return mEndTime; }
			int64_t run() { return mRun; }

			void setPath( const std::string& path ) { mPath = path; sanitize_alnumslashuscore(mPath); }
			void setData( const std::string& data ) { mData = data; }
			void setData( const nlohmann::json& data ) { mData = data.dump(); }
			void setBeginTime( int64_t tm ) { mBeginTime = (tm >= 0 ? tm : 0); }
			void setEndTime( int64_t tm ) { mEndTime = (tm >= 0 ? tm : 0); }
			void setRun( int64_t run ) { mRun = (run >= 0 ? run : 0); }

		private:

			std::tuple<std::string,std::string,std::string,bool> decodePath( const std::string& path );

			std::string mPath{};
			std::string mData{};
			int64_t mBeginTime{0};
			int64_t mEndTime{0};
			int64_t mRun{0};
	};

} // namespace CDBNPP
