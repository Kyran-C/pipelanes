#pragma once

#include <tuple>
#include <utility>

template< typename T >
concept Tuple_Like =
requires( T t )
{
    std::tuple_size< T >::value;
    std::get< 0 >( t );
};

template< typename T >
concept Optional_Like =
requires( T t )
{
    { t.has_value() } -> std::same_as< bool >;
    t.value();
};
