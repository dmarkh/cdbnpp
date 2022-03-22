#pragma once

#include <npp/core/module.h>

namespace NPP {
	namespace Framework {

		class Eemc : public NPP::Core::Module {
			public:
				Eemc() : Module( "eemc" ) {};
				virtual ~Eemc() = default;

				int32_t execute();

				const std::vector<std::string>& hardRequirements() { return mHardRequirements; };
				const std::vector<std::string>& softRequirements() { return mSoftRequirements; };

			private:
				std::vector<std::string> mHardRequirements{"iomaker"};
				std::vector<std::string> mSoftRequirements{};

		};

	} // namespace Framework
} // namespace NPP
