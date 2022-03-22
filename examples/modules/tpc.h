#pragma once

#include <npp/core/module.h>

namespace NPP {
namespace Framework {

	class Tpc : public NPP::Core::Module {
		public:
			Tpc() : Module( "tpc" ) {};
			virtual ~Tpc() = default;

			int32_t execute();

			const std::vector<std::string>& hardRequirements() { return mHardRequirements; };
			const std::vector<std::string>& softRequirements() { return mSoftRequirements; };

		private:
			std::vector<std::string> mHardRequirements{"iomaker"};
			std::vector<std::string> mSoftRequirements{};

	};

} // namespace Framework
} // namespace NPP