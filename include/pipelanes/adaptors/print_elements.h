#pragma once

#include <ranges>
#include <utility>

#include "pipelanes/tuple_utils.h"

// A transform view which prints each element, using the specified format string.
template< typename Format_String = decltype( "{: >6} " ) > 
auto print_tuple_elems( Format_String&& format_string = "{: >6} " )
{
    return std::views::transform(
        [ format_string = std::forward< Format_String >( format_string ) ]
        < typename Tuple >
		( Tuple&& tuple )
        {
            print_tuple( tuple, format_string );
            return std::forward< Tuple >( tuple );
        }
    );
}
