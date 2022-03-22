#pragma once

#include <npp/core/module.h>

namespace NPP {
namespace Framework {

	class Analysis : public NPP::Core::Module {
		public:
			Analysis() : Module( "analysis" ) {};
			virtual ~Analysis() = default;

			int32_t execute();

			const std::vector<std::string>& hardRequirements() { return mHardRequirements; };
			const std::vector<std::string>& softRequirements() { return mSoftRequirements; };

		private:
			std::vector<std::string> mHardRequirements{ "tpc", "svt", "bemc" };
			std::vector<std::string> mSoftRequirements{ "eemc" };

	};

} // namespace Framework
} // namespace NPP