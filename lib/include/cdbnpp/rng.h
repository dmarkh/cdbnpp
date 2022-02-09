#pragma once

#include <ctime>
#include <memory>

#include <xoshiro-cpp/xoshiro-cpp.h>

#include "cdbnpp/singleton.h"

namespace CDBNPP {

	class Rng;

	using RngS = Singleton<Rng, CreateMeyers>;

	class Rng {

		public:

			Rng() : mRng( std::make_shared<XoshiroCpp::Xoshiro256StarStar>( std::time(nullptr) ) ) {}
			~Rng() = default;

			// double between 0..1
			double random() { return (*mRng)() / (double)( mRng->max() - mRng->min() ); }

			template<class T>
				T random_inclusive( T min, T max ) {
					// Ex: RngS::Instance.random_inclusive<size_t>( 1, 100 ); => [ 1 ... 100 ]
					return min + (*mRng)() / ( ( mRng->max() - mRng->min() ) / ( max - min + 1 ) );
				}

		private:
			std::shared_ptr<XoshiroCpp::Xoshiro256StarStar> mRng{nullptr};

	};


} // namespace CDBNPP
