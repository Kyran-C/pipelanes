#pragma once

#include <ranges>

#include "pipelanes/algebraic_types/monad.h"

// A transform view which calls bind on a monadic value.
template< typename Func >
auto bind_view( Func&& func )
{
    return std::views::transform(
        [=]< typename Value_T >
        ( Value_T&& value )
		  -> decltype( auto )
        {
            return dispatch_bind( std::forward< Value_T >( value ), func );
        }
    );
}
