#pragma once

#include <optional>

// shamelessly "borrowed" from:
// https://www.cppstories.com/2021/sphero-cpp-return/
// ...then modified to include a message

namespace NPP {
namespace Util {

	template <typename T>
	class Result {
		public:
			constexpr Result(T const& t) noexcept
				: mOptional { t } {}

			explicit constexpr Result( ) noexcept = default;

			[[nodiscard]] constexpr bool valid( ) const noexcept {
				return mOptional.has_value( );
			}

			[[nodiscard]] constexpr bool invalid( ) const noexcept {
				return !valid( );
			}

			[[nodiscard]] constexpr auto get( ) const -> T {
				return mOptional.value( );
			}

			[[nodiscard]] constexpr auto get_or( ) const noexcept -> T {
				return mOptional.value_or(T( ));
			}

			[[nodiscard]] const std::string& msg() const noexcept { return mMsg; }

			void setMsg( const std::string& m ) noexcept { mMsg = m; }

		private:
			std::optional<T> mOptional;
			std::string mMsg{};
	};

} // namespace Util
} // namespace NPP
