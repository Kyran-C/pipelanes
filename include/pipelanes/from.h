#pragma once


// A static value carrier, to enable easier and clearer calling of template functions (with non-type params).
template< int... N >
struct From{};

// Selects the lanes which should be forwarded to an operation.
// (negative indices count backwards from the last lane)
template< int... N >
static constexpr
From< N... > from{};

