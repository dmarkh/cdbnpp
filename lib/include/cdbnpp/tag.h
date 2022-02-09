#pragma once

#include <string>
#include <map>
#include <memory>
#include <unordered_map>

namespace CDBNPP {

	class Tag;

	using STagPtr_t = std::shared_ptr<Tag>;
	using WTagPtr_t = std::weak_ptr<Tag>;
	using IdToTag_t = std::unordered_map<std::string,STagPtr_t>;
	using PathToTag_t = std::map<std::string,STagPtr_t>;

	class Tag {

		public:

			Tag( const std::string& id, const std::string& name, const std::string& pid = "",
				const std::string& tbname = "", int64_t ct = 0, int64_t dt = 0, int64_t mode = 0, const std::string& schema = "" )
				: mId(id), mName(name), mPid(pid), mTbname(tbname), mCt(ct), mDt(dt), mMode(mode), mSchema(schema) {}
			~Tag() = default;

			const std::string id() { return mId; }
			const std::string pid() { return mPid; }
			const std::string name() { return mName; }
			const std::string tbname() { return mTbname; }
			int64_t ct() { return mCt; }
			int64_t dt() { return mDt; }
			int64_t mode() { return mMode; }
			const std::string schema() { return mSchema; }

			void setId( const std::string& id ) { mId = id; }
			void setPid( const std::string& pid ) { mPid = pid; }
			void setName( const std::string& name ) { mName = name; }
			void setTbname( const std::string& tbname ) { mTbname = tbname; }
			void setCt( int64_t ct ) { mCt = ct; }
			void setDt( int64_t dt ) { mDt = dt; }
			void setMode( int64_t mode) { mMode = mode; }
			void setSchema( const std::string& schema ) { mSchema = schema; }

			bool isStruct() { return ( mMode > 0 ) && mTbname.size(); }
			std::string path() { return ( mParent.expired() ? mName : ( mParent.lock()->path() + '/' + mName ) ); }

			const IdToTag_t& children() { return mChildren; }
			void addChild( const STagPtr_t& tag ) { mChildren.insert({ tag->id(), tag }); }
			void removeChild( std::string id ) { mChildren.erase( id ); }
			void clearChildren() { mChildren.clear(); }

			STagPtr_t parent() { return ( mParent.expired() ? nullptr : mParent.lock() ); }
			void setParent( const STagPtr_t& tag ) { mParent = tag; mPid = tag->id(); }

			nlohmann::json toJson() {
				nlohmann::json js = {
					{"id", mId}, {"pid", mPid}, {"name", mName}, {"tbname", mTbname},
					{"ct", mCt}, {"dt", mDt}, {"mode", mMode}, {"schema", mSchema}, {"path", path()}
				};
				return js;
			}

		private:

			std::string mId{};
			std::string mName{};
			std::string mPid{};
			std::string mTbname{};
			int64_t mCt{0};
			int64_t mDt{0};
			int64_t mMode{0};
			std::string mSchema{};
			IdToTag_t mChildren{};
			WTagPtr_t mParent;
	};

} // namespace CDBNPP
