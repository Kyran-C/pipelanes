#pragma once

#include <array>
#include <utility>

// parse any negative indices into their actual index
constexpr
auto parse_lane_index( int index, std::size_t lane_count )
  -> std::size_t
{
	if( index < 0 )
		return lane_count + index;
	else
		return index;
}

// is the value contained within the pack?
constexpr
auto in_pack( auto value, auto... pack )
{
	return ( ( value == pack ) || ... );
}

// returns an array of any index from 0 to Index_Count which don't match the provided indices list
template< std::size_t Index_Count, int... Indices >
constexpr
auto index_complement()
{
	using namespace std;

	static constexpr
	auto complement_count =
	[&]
	< size_t... Out_Index >
	( index_sequence< Out_Index... > )
	{
		return ( ( in_pack( Out_Index, parse_lane_index( Indices, Index_Count )... ) ? 0 : 1 ) + ... );
	}( make_index_sequence< Index_Count >() );

	array< int, complement_count > result{};

	for( size_t index{}, n{}; index < complement_count; ++n )
	{
		if( in_pack( n, parse_lane_index( Indices, Index_Count )... ) )
			continue;

		result[ index++ ] = n;
	}

	return result;
}

// unpacks the (static) array into the given callable's template arguments
template< auto Array >
auto array_unpack( auto&& func )
{
	using namespace std;

	static constexpr
	auto count = Array.size();

	return
		[ & ]
		< size_t... N >( index_sequence< N... > )
		{
			return decltype( func )( func ).template operator()< get< N >( Array )... >();
		}( make_index_sequence< count >() );
}
