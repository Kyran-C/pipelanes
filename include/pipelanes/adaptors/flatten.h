#pragma once

#include <ranges>
#include <utility>

#include "pipelanes/tuple_utils.h"
#include "pipelanes/lane_indices.h"

// A transform view that flattens nested lanes.
// If no lane indices are provided, all lanes will be flattened
template< int... Lanes >
auto flatten()
{
	return std::views::transform(
		[&]< typename Tuple >( Tuple&& tuple )
		{
            using Tuple_T = std::remove_cvref_t< Tuple >;
			
			static constexpr
			auto tuple_size = std::tuple_size_v< Tuple_T >;
            
			static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );
			
			return flatten_tuple_elements< parse_lane_index( Lanes, tuple_size )... >( std::forward< decltype( tuple ) >( tuple ) );
		}
	);
}
