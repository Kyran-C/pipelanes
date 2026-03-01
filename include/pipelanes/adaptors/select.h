#pragma once

#include <array>
#include <ranges>
#include <tuple>
#include <utility>

#include "pipelanes/lane_indices.h"

// Allows you to swizzle/select different elements in arbitrary order.
// If the same lane is specified multiple times, duplicates will be copied;
//      otherwise all values are forwarded.
template< int... Lanes >
auto select_lanes()
{
    return std::views::transform(
        []< typename Tuple >( Tuple&& tuple )
        {
			using Tuple_T = std::remove_cvref_t< Tuple >;

			static constexpr
			auto tuple_size = std::tuple_size_v< Tuple_T >;

			static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );

			// an array of bools, indicating whether the element at the given index 
			// is a duplicate and needs to be copied
			static constexpr
			auto needs_copy = []< size_t... Indices >( std::index_sequence< Indices... > )
			{
				std::array values{ parse_lane_index( Lanes, tuple_size )... };

				auto contains = []( size_t index, auto& list  )
				{
					for( size_t i{}; i < index; ++i )
					{
						if( list[ i ] == list[ index ] )
							return true;
					}
					return false;
				};
				return std::array{ contains( Indices, values )... };
			}( std::make_index_sequence< sizeof...( Lanes ) >{} );

            auto get = [&]< size_t Nth, size_t Index >()
              -> decltype( auto )
            {
                if constexpr( needs_copy[ Nth ] )
                {
                    return std::get< Index >( tuple );
                }
                else
                {
                    return std::get< Index >( std::forward< Tuple >( tuple ) );
                }
            };

            return [&]< size_t... N >( std::index_sequence< N... > )
            {
                return std::make_tuple(
                    get.template operator()< N, parse_lane_index( Lanes, tuple_size ) >()...
                );
            }( std::make_index_sequence< sizeof...( Lanes ) >() );
        }
    );
}
