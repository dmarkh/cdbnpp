#pragma once

#include <npp/core/module.h>

namespace NPP {
namespace Framework {

	class IOMaker : public NPP::Core::Module {
		public:
			IOMaker() : Module( "iomaker" ) {};
			virtual ~IOMaker() = default;

			int32_t execute();

			const std::vector<std::string>& hardRequirements() { return mHardRequirements; };
			const std::vector<std::string>& softRequirements() { return mSoftRequirements; };

		private:
			std::vector<std::string> mHardRequirements{};
			std::vector<std::string> mSoftRequirements{};

	};

} // namespace Framework
} // namespace NPP