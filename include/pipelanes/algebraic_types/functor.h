#pragma once

#include <optional>
#include <utility>

#include "algebraic_traits.h"
#include "pipelanes/tuple_utils.h"
#include "pipelanes/type_fail.h"

template< typename T >
concept Has_Fmap =
requires( T&& t )
{
    fmap< T >( t, []( auto&& arg ){ return arg; } );
};

template< typename Value_T >
auto dispatch_fmap( Value_T&& value, auto&& func )
  -> decltype( auto )
{
	using Value = std::remove_cvref_t< Value_T >;

    if constexpr( Has_Fmap< Value > )
        return fmap( std::forward< Value_T >( value ), func );
    else if constexpr( Tuple_Like< Value > )
        return tuple_fmap( func )( std::forward< Value_T >( value ) );
    else if constexpr( Optional_Like< Value > )
    {
        using Optional_T = decltype( std::make_optional( func( std::forward< Value_T >( value ).value() ) ) );
        return value.has_value() ? std::make_optional( func( std::forward< Value_T >( value ).value() ) ) : Optional_T{ std::nullopt };
    }
	else
		static_assert( type_fail< Value >, "No fmap implementation for type." );
}
