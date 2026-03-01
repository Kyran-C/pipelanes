#pragma once

// Combine multiple callables into one aggregate.
template< typename... Func_T >
struct Overload_Set : Func_T...
{
    using Func_T::operator()...;
};


template< typename... Func_T >
Overload_Set( Func_T&&... func ) -> Overload_Set< Func_T... >;