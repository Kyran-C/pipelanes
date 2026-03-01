#pragma once

#include <ranges>

// Range extraction operator
void operator >>=( std::ranges::range auto&& range, auto&& func )
{
    for( auto&& elem : range )
        func( elem );
}