#pragma once

#include <optional>
#include <utility>

#include "algebraic_traits.h"
#include "pipelanes/tuple_utils.h"
#include "pipelanes/type_fail.h"

template< typename T >
concept Has_Bind =
requires( T&& t )
{
    mbind< T >( t, []( auto&& arg ){ return std::declval< T >(); } );
};

template< typename Value_T >
auto dispatch_bind( Value_T&& value, auto&& func )
  -> decltype( auto )
{
	using Value = std::remove_cvref_t< Value_T >;

    if constexpr( Has_Bind< Value > )
        return mbind( std::forward< Value_T >( value ), func );
    else if constexpr( Tuple_Like< Value > )
        return tuple_bind( func )( std::forward< Value_T >( value ) );
    else if constexpr( Optional_Like< Value > )
        return value.has_value() ? func( std::forward< Value_T >( value ).value() ) : std::nullopt;
	else
		static_assert( type_fail< Value >, "No bind implementation for type." );
}
