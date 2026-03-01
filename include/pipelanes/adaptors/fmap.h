#pragma once

#include <ranges>

#include "pipelanes/algebraic_types/functor.h"

// A transform view which calls fmap on a functor value.
template< typename Func >
auto fmap_view( Func&& func )
{
    return std::views::transform(
        [=]< typename Value_T >
        ( Value_T&& value )
		  -> decltype( auto )
        {
            return dispatch_fmap( std::forward< Value_T >( value ), func );
        }
    );
}
