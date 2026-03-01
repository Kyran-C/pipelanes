#pragma once

#include <array>
#include <ranges>
#include <tuple>
#include <utility>

#include "pipelanes/lane_indices.h"

// Groups the selected lanes together as a nested tuple, and places it at the front of the results.
// gather_to_front< 1, 3 >( A, B, C, D ) -> ( ( B, D ), A, C )
template< int... Lanes >
auto gather_to_front()
{
    return std::views::transform(
        []< typename Tuple >( Tuple&& tuple )
        {
            using Tuple_T = std::remove_cvref_t< Tuple >;
            
            static constexpr
            size_t tuple_size{ std::tuple_size_v< Tuple_T > };
            
			static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );

            static constexpr
            auto is_selected = []( size_t N ){ return ( ( N == parse_lane_index( Lanes, tuple_size ) ) || ... ); };

            static constexpr 
            size_t gather_count = sizeof...( Lanes );
            
            static constexpr 
            size_t leftover_count = []< size_t... N >( std::index_sequence< N... > )
			{
				return ( ( is_selected( N ) ? 0 : 1 ) + ... );
			}( std::make_index_sequence< tuple_size >{} );

            static constexpr
            std::array leftover_indices = []
            {
                std::array< size_t, leftover_count > result{};

                for( size_t index{}, i{}; i < leftover_count; ++i )
                {
                    while( is_selected( index ) )
                        ++index;

                    result[ i ] = index++;
                }

                return result;
            }();

            return [&]< size_t... Leftover_N >( std::index_sequence< Leftover_N... > )
            {
                return std::make_tuple( std::make_tuple( std::get< parse_lane_index( Lanes, tuple_size ) >( std::forward< Tuple >( tuple ) )... ), std::get< leftover_indices[ Leftover_N ] >( std::forward< Tuple >( tuple ) )... );

            }( std::make_index_sequence< leftover_count >() );
        }
    );
}

// Groups the selected lanes together as a nested tuple, and places it at the back of the results.
// gather_to_back< 1, 3 >( A, B, C, D ) -> ( A, C, ( B, D ) )
template< int... Lanes >
auto gather_to_back()
{
    return std::views::transform(
        []< typename Tuple >( Tuple&& tuple )
        {
            using Tuple_T = std::remove_cvref_t< Tuple >;
            
            static constexpr
            size_t tuple_size{ std::tuple_size_v< Tuple_T > };
            
			static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );
            
            static constexpr
            auto is_selected = []( size_t N ){ return ( ( N == parse_lane_index( Lanes, tuple_size ) ) || ... ); };

            static constexpr 
            size_t gather_count = sizeof...( Lanes );

            static constexpr 
            size_t leftover_count = []< size_t... N >( std::index_sequence< N... > )
			{
				return ( ( is_selected( N ) ? 0 : 1 ) + ... );
			}( std::make_index_sequence< tuple_size >{} );

            static constexpr
            std::array leftover_indices = []
            {
                std::array< size_t, leftover_count > result{};

                for( size_t index{}, i{}; i < leftover_count; ++i )
                {
                    while( is_selected( index ) )
                        ++index;

                    result[ i ] = index++;
                }

                return result;
            }();

            return [&]< size_t... Leftover_N >( std::index_sequence< Leftover_N... > )
            {
                return std::make_tuple( std::get< leftover_indices[ Leftover_N ]  >( std::forward< Tuple >( tuple ) )..., std::make_tuple( std::get< parse_lane_index( Lanes, tuple_size ) >( std::forward< Tuple >( tuple ) )... ) );

            }( std::make_index_sequence< leftover_count >() );
        }
    );
}
