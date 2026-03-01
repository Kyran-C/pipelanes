#pragma once

#include <utility>

// Function composition operator.
template< typename First, typename Second >
auto operator*( First&& first, Second&& second )
{
    return
    [ first = std::forward< First >( first )
    , second = std::forward< Second >( second ) ]
    ( auto&&... args )
    {
        return second( first( decltype(args)(args)... ) );
    };
}