#pragma once

#include <memory>
#include <string>
#include <vector>

namespace NPP {
namespace Core {

	class Module;
	using SModulePtr_t = std::shared_ptr<Module>;

	class Module {
		public:
			Module( const std::string& id ) : mId(id) {};
			virtual ~Module() = default;

		virtual const std::string& id() { return mId; };

		virtual const std::vector<std::string>& hardRequirements() = 0;
		virtual const std::vector<std::string>& softRequirements() = 0;
		virtual int32_t execute() = 0;

		private:
			std::string mId{};
	};

} // namespace Framework
} // namespace NPP
