#pragma once

#include <nlohmann/json.hpp>

#include <ctime>
#include <deque>
#include <string>
#include <tuple>
#include <vector>

#include "cdbnpp/util.h"

namespace CDBNPP {

	class Payload;

	using DecodedPathTuple = std::tuple< std::vector<std::string>, std::string, std::string, bool>;
	using SPayloadPtr_t = std::shared_ptr<Payload>;
	using WPayloadPtr_t = std::weak_ptr<Payload>;
	using PayloadResults_t = std::unordered_map<std::string, SPayloadPtr_t>;

	class Payload {

		public:

			Payload() = default;

			Payload( const std::string& id, const std::string& pid, const std::string& flavor,
				const std::string& structName, const std::string& directory,
				int64_t ct, int64_t bt, int64_t et, int64_t dt, int64_t run, int64_t seq ) 
				: mId(id), mPid(pid), mFlavor(flavor), mStructName(structName), mDirectory(directory),
					mCreateTime(ct), mBeginTime(bt), mEndTime(et), mDeactiveTime(dt),
					mRun(run), mSeq(seq) {};

			~Payload() = default;

			bool valid() {
				return (
					mDirectory.size() && mStructName.size() 
					&& ( mBeginTime > 0 || mRun > 0 ) 
					&& ( ( mBeginTime > 0 && mEndTime != 0 ) ? ( mBeginTime < mEndTime ) : true )
				);
			}
			bool decoded() { return ( valid() && mFlavor.size() && mPid.size() ); }
			bool ready() { return ( decoded() && ( mURI.size() || mData.size() ) ); }

			const std::string& id() const { return mId; }
			const std::string& pid() const { return mPid; }
			const std::string& flavor() const { return mFlavor; }
			const std::string& directory() const { return mDirectory; }
			const std::string& structName() const { return mStructName; }
			const std::string  URI() const { return mURI; }

			int64_t createTime() const { return mCreateTime; }
			std::string createTimeAsStr() const { return unixtime_to_utc_date(mCreateTime); }
			int64_t beginTime() const { return mBeginTime; }
			std::string beginTimeAsStr() const { return unixtime_to_utc_date(mBeginTime); }
			int64_t endTime() const { return mEndTime; }
			std::string endTimeAsStr() const { return unixtime_to_utc_date(mEndTime); }
			int64_t deactiveTime() const { return mDeactiveTime; }
			std::string deactiveTimeAsStr() const { return unixtime_to_utc_date(mDeactiveTime); }
			int64_t run() const { return mRun; }
			int64_t seq() const { return mSeq; }

			const std::string& format() const { return mFmt; }
			int64_t mode() const { return mMode; } // 1 = struct by time, 2 = struct by run,seq

			const std::string& data() const { return mData; }
			nlohmann::json dataAsJson() const;

			size_t dataSize() const { return mData.length(); }

			void setId( const std::string& id ) { mId = id; }
			void setPid( const std::string& pid ) { mPid = pid; }
			void setFlavor( const std::string& flavor ) { mFlavor = flavor; }
			void setDirectory( const std::string& directory ) { mDirectory = directory; }
			void setStructName( const std::string& structName ) { mStructName = structName; }

			void setURI( const std::string& uri ) {
				mURI = uri;
				auto parts = explode( uri, '.' );
				std::string fmt = parts.back();
				string_to_lower_case(fmt);
				sanitize_alnum(fmt);
				setData( std::string(""), fmt );
			}

			void setCreateTime( int64_t createTime ) { mCreateTime = createTime; }
			void setCreateTime( const std::string& createTime ) { mCreateTime = string_to_time(createTime); }
			void setDeactiveTime( int64_t deactiveTime ) { mDeactiveTime = deactiveTime; }
			void setDeactiveTime( const std::string& deactiveTime ) { mDeactiveTime = string_to_time(deactiveTime); }

			void setBeginTime( int64_t beginTime) { mBeginTime = beginTime; if ( mBeginTime != 0 ) { mMode = 1; } }
			void setBeginTime( const std::string& beginTime) { mBeginTime = string_to_time( beginTime ); if ( mBeginTime != 0 ) { mMode = 1; } }
			void setEndTime( int64_t endTime ) { mEndTime = endTime; if ( mEndTime != 0 ) { mMode = 1; } }
			void setEndTime( const std::string& endTime ) { mEndTime = string_to_time( endTime ); if ( mEndTime != 0 ) { mMode = 1; } }

			void setRun( int64_t run ) { mRun = run; if ( mRun != 0 ) { mMode = 2; } }
			void setSeq( int64_t seq ) { mSeq = seq; if ( mSeq != 0 ) { mMode = 2; } }

			void setMode( int64_t mode ) { mMode = mode; }

			void setData( const std::string& data, const std::string& fmt = "dat" );
			void setData( const nlohmann::json& data, const std::string& fmt = "json" );

			void clearData() { mData = ""; mFmt = ""; }

			static DecodedPathTuple decodePath( const std::string& path );

			inline friend std::ostream& operator << (std::ostream& os, const Payload* p) {
		    os << "id: " << p->id() << ", pid: " << p->pid() << ", flavor: " << p->flavor() << ", structName: " << p->structName()
					<< ", dir: " << p->directory() << ", URI: " << p->URI() << ", ct: " << p->createTime() << ", dt: " << p->deactiveTime()
					<< ", bt: " << p->beginTime() << ", et: " << p->endTime() << ", run: " << p->run() << ", seq: " << p->seq()
					<< ", mode: " << p->mode() << ", data_size: " << p->data().size() << ", fmt: " << p->format();
    		return os;
			}

		private:

			std::string mId{};						// <uuid> = specific payload UUID
			std::string mPid{};						// <uuid> = specific parent structID ( aka tag-struct )
			std::string mFlavor{};				// i.e. "sim" or "ofl" or whatnot
			std::string mStructName{};		// i.e. "tpcT0"
			std::string mDirectory{};			// i.e. "Calibrations/TPC"
			std::string mURI{};						// http://www.star.bnl.gov/data/<puuid>/<uuid>.json

			int64_t mCreateTime{0};
			int64_t mBeginTime{0};
			int64_t mEndTime{0};
			int64_t mDeactiveTime{0};
			int64_t mRun{0};
			int64_t mSeq{0};

			int64_t mMode{0}; // 0 = by time, 1 = by run
			std::string mData{};
			std::string mFmt{}; // dat, json, bson, ubjson, cbor, msgpack

	};


} // namespace CDBNPP
