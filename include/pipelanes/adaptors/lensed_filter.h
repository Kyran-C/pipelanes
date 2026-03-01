#pragma once

#include <ranges>
#include <tuple>

#include "pipelanes/lane_indices.h"

// A filter view that applies an N-ary predicate to the specified lanes.
// If no lane indices are specified, all lanes are passed to the predicate.
template< int... Lanes, typename Func >
auto filter_lens( Func&& func )
{
    return std::views::filter(
        [=]( auto&& elems )
        {
            using Tuple_T = std::remove_cvref_t< decltype( elems ) >;
			
			static constexpr
			auto tuple_size = std::tuple_size_v< Tuple_T >;

			static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );

            static constexpr
            size_t index_count = sizeof...( Lanes );


            // if no indices are supplied, test with every lane
            static constexpr
            auto elem_indices = []
            {
                if constexpr( index_count > 0u )
                    return std::index_sequence< parse_lane_index( Lanes, tuple_size )... >{};
                else
                    return std::make_index_sequence< std::tuple_size_v< Tuple_T > >{};
            }();


            return [&]< size_t... Nth_Lane >( std::index_sequence< Nth_Lane... > )
            {
                return func( std::get< Nth_Lane >( decltype( elems )( elems ) ) ... );
            }( elem_indices );
        }
    );
}

// A filter view that applies a predicate to individual lanes. The value is passed if any lane passes its predicate.
// If no indices are passed, the predicate is applied to every lane.
// If multiple predicates are passed, they're applied sequentially to the specified lanes.
//      (wrapping back to the start if the number of indices exceeds the number of predicates)
template< int... Lanes, typename... Func >
auto filter_lens_any( Func&&... func )
{
    static_assert( sizeof...( Func ) > 0u );

    return std::views::filter(
        [ funcs = std::tuple{ std::forward< Func >( func )... }
        ]( auto&& elems )
        {
            using Tuple_T = std::remove_cvref_t< decltype( elems ) >;

			static constexpr
			auto tuple_size = std::tuple_size_v< Tuple_T >;

			static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );

            static constexpr
            size_t func_count = sizeof...( Func );

            static constexpr
            size_t index_count = sizeof...( Lanes );

            // if no indices are supplied, test every lane
            static constexpr
            size_t elem_count = []
            {
                if constexpr( index_count > 0u )
                    return index_count;
                else
                    return std::tuple_size_v< Tuple_T >;
            }();

            static constexpr
            auto elem_indices = []
            {
                if constexpr( index_count > 0u )
                    return std::index_sequence< parse_lane_index( Lanes, tuple_size )... >{};
                else
                    return std::make_index_sequence< elem_count >{};
            }();

            static constexpr
            auto func_indices = std::make_index_sequence< elem_count >{};

            auto filter_lane = [&]< size_t Nth_Func, size_t Nth_Elem >()
            {
                auto& predicate = std::get< Nth_Func % func_count >( funcs );

                return predicate( std::get< Nth_Elem >( decltype( elems )( elems ) ) );
            };

            return [&]< size_t... Nth_Func, size_t... Nth_Lane >( std::index_sequence< Nth_Func... >, std::index_sequence< Nth_Lane... > )
            {
                return ( filter_lane.template operator()< Nth_Func, Nth_Lane >() || ... );
            }( func_indices, elem_indices );
        }
    );
}


// A filter view that applies a predicate to individual lanes. The value is passed if all lanes pass their predicates.
// If no indices are passed, the predicate is applied to every lane.
// If multiple predicates are passed, they're applied sequentially to the specified lanes.
//      (wrapping back to the start if the number of indices exceeds the number of predicates)
template< int... Lanes, typename... Func >
auto filter_lens_all( Func&&... func )
{
    static_assert( sizeof...( Func ) > 0u );

    return std::views::filter(
        [ funcs = std::tuple{ std::forward< Func >( func )... }
        ]( auto&& elems )
        {
            using Tuple_T = std::remove_cvref_t< decltype( elems ) >;

			static constexpr
			auto tuple_size = std::tuple_size_v< Tuple_T >;

			static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );

            static constexpr
            size_t func_count = sizeof...( Func );

            static constexpr
            size_t index_count = sizeof...( Lanes );

            // if no indices are supplied, test every lane
            static constexpr
            size_t elem_count = []
            {
                if constexpr( index_count > 0u )
                    return index_count;
                else
                    return std::tuple_size_v< Tuple_T >;
            }();

            static constexpr
            auto elem_indices = []
            {
                if constexpr( index_count > 0u )
                    return std::index_sequence< parse_lane_index( Lanes, tuple_size )... >{};
                else
                    return std::make_index_sequence< elem_count >{};
            }();

            static constexpr
            auto func_indices = std::make_index_sequence< elem_count >{};

            auto filter_lane = [&]< size_t Nth_Func, size_t Nth_Elem >()
            {
                auto& predicate = std::get< Nth_Func % func_count >( funcs );

                return predicate( std::get< Nth_Elem >( decltype( elems )( elems ) ) );
            };

            return [&]< size_t... Nth_Func, size_t... Nth_Lane >( std::index_sequence< Nth_Func... >, std::index_sequence< Nth_Lane... > )
            {
                return ( filter_lane.template operator()< Nth_Func, Nth_Lane >() && ... );
            }( func_indices, elem_indices );
        }
    );
}
