#pragma once

#include <utility>

#include "pipelanes/algebraic_types/functor.h"

// Operator for fmap.
template< typename First_F, typename Second_F >
auto operator >( First_F&& first, Second_F&& second )
{
    return 
    [ first = std::forward< First_F >( first )
    , second = std::forward< Second_F >( second ) ]
    < typename... Args_T >
    ( Args_T... args )
    {
        return dispatch_fmap( first( std::forward< Args_T >( args )... ), second );
    };
}
