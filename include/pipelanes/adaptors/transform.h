#pragma once

#include <ranges>
#include <tuple>
#include <utility>

#include "pipelanes/adaptors/bind.h"
#include "pipelanes/from.h"
#include "pipelanes/lane_indices.h"


// A transform view which also passes the element's index as an argument.
auto transform_indexed( auto&& func )
{
    return std::views::transform(
        [=, i = 0u]( auto a ) mutable
        {
            return func( i++, a ); 
        }
    );
}

// A transform view which only passes the specified lanes as arguments to the function.
template< int... Lanes, typename Func_T >
auto transform_from( Func_T&& func )
{
	static_assert( sizeof...( Lanes ) > 0u, "No lanes specified." );

    return std::views::transform(
        [=, func = std::forward< Func_T >( func ) ]
        < typename Arg_T >( Arg_T&& arg )
        {
            using Tuple_T = std::remove_cvref_t< decltype( arg ) >;
			
			static constexpr
			auto tuple_size = std::tuple_size_v< Tuple_T >;

			static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );
			
			return func( std::get< parse_lane_index( Lanes, tuple_size ) >( std::forward< Arg_T >( arg ) )... );
        }
    );
}


// A transform view which returns its result and input as a tuple.
template< typename Func_T >
auto transform_to_front( Func_T&& func )
{
    return std::views::transform(
        [=, func = std::forward< Func_T >( func ) ]
        < typename Arg_T >( Arg_T&& arg )
        {
            auto&& result = func( arg );
            return std::make_tuple( decltype( result )( result ), std::forward< Arg_T >( arg ) );
        }
    );
}

// A transform view which prepends its result to the input tuple.
template< int... Lanes, typename Func_T >
auto transform_to_front( From< Lanes... >, Func_T&& func )
{
	static_assert( sizeof...( Lanes ) > 0u, "No lanes specified." );

    return std::views::transform(
        [=, func = std::forward< Func_T >( func ) ]
        < typename Tuple_T >( Tuple_T&& tuple )
        {
            return tuple_bind(
                [&]< typename... Args_T >( Args_T&&... args )
                {
					static constexpr
					auto tuple_size = sizeof...( args );

					static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );
					
                    auto&& result = func( std::get< parse_lane_index( Lanes, tuple_size ) >( tuple )... );
                    return std::make_tuple( decltype( result )( result ), std::forward< Args_T >( args )... );
                }
            )( std::forward< Tuple_T >( tuple ) );
        }
    );
}


// A transform view which returns its input and result as a tuple.
template< typename Func_T >
auto transform_to_back( Func_T&& func )
{
    return std::views::transform(
        [=, func = std::forward< Func_T >( func ) ]
        < typename Arg_T >( Arg_T&& arg )
        {
            auto&& result = func( arg );
            return std::make_tuple( std::forward< Arg_T >( arg ), decltype( result )( result ) );
        }
    );
}

// A transform view which appends its result to the input tuple.
template< int... Lanes, typename Func_T >
auto transform_to_back( From< Lanes... >, Func_T&& func )
{
	static_assert( sizeof...( Lanes ) > 0u, "No lanes specified." );

    return std::views::transform(
        [=, func = std::forward< Func_T >( func ) ]
        < typename Arg_T >( Arg_T&& arg )
        {
            return tuple_bind(
                [&]< typename... Args_T >( Args_T&&... args )
                {
					static constexpr
					auto tuple_size = sizeof...( args );

					static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );
					
                    auto&& result = func( std::get< parse_lane_index( Lanes, tuple_size ) >( arg )... );
                    return std::make_tuple( std::forward< Args_T >( args )..., decltype( result )( result ) );
                }
            )( std::forward< Arg_T >( arg ) );
        }
    );
}


// A transform view which inserts its result back into the input tuple at the specified index.
// The function must accept a tuple.
template< int Dest_Lane = 0, typename Func_T >
auto transform_to( Func_T&& func )
{
    return std::views::transform(
        [=, func = std::forward< Func_T >( func ) ]
        < typename Tuple_T >( Tuple_T&& tuple )
        {
			using Tuple = std::remove_cvref_t< decltype( tuple ) >;
			
			static constexpr
			auto tuple_size = std::tuple_size_v< Tuple >;

            auto get_value = [&]< size_t N, typename Elem_T >( Elem_T&& elem )
            {
                if constexpr( N == parse_lane_index( Dest_Lane, tuple_size ) )
                    return func( std::forward< Tuple_T >( tuple ) );
                else
                    return std::forward< Elem_T >( elem );
            };

            return tuple_bind(
                [&]< typename... Args_T >( Args_T&&... args )
                {
                    return [&]< size_t... N >( std::index_sequence< N... > )
                    {
                        return std::make_tuple( get_value.template operator()< N >( std::forward< Args_T >( args ) )... );
                    }( std::make_index_sequence< sizeof...( args ) >{} );
                }
            )( std::forward< Tuple_T >( tuple ) );
        }
    );
}

// A transform view which calls a function with a subset of the input elements,
//      and inserts its result back into the input tuple at the specified index.
// `From<>` specifies the input indices to pass.
// Any unmodified elements are forwarded to the output tuple.
template< int Dest_Lane = 0, int... Lanes, typename Func_T >
auto transform_to( From< Lanes... >, Func_T&& func )
{
	static_assert( sizeof...( Lanes ) > 0u, "No lanes specified." );

    return std::views::transform(
        [=, func = std::forward< Func_T >( func ) ]
        < typename Tuple >( Tuple&& tuple )
        {
            return tuple_bind(
                [&]< typename... Args_T >( Args_T&&... args )
                {
					static constexpr
					auto tuple_size = sizeof...( args );

					static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );
					
                    return [&]< size_t... Nth >( std::index_sequence< Nth... > )
                    {
                        auto call_func = [&]< size_t N >()
                          -> decltype( auto )
                        {
                            if constexpr( N == parse_lane_index( Dest_Lane, tuple_size ) )
                                return func( std::get< parse_lane_index( Lanes, tuple_size ) >( tuple )... );
                            else
                                return std::get< N >( std::forward< Tuple >( tuple ) );
                        };

                        return std::make_tuple( call_func.template operator()< Nth >()... );
                    }( std::make_index_sequence< tuple_size >{} );
                }
            )( std::forward< Tuple >( tuple ) );
        }
    );
}

// A transform view which applies a set of functions to the specified indices.
// Any unmodified elements are forwarded to the same position in the output tuple.
// If the number of functions doesn't match the number of specified lanes, the functions are reused.
template< int... Lanes, typename... Func_T >
auto transform_nth( Func_T&&... func )
{
    static_assert( sizeof...( Lanes ) > 0u, "No lanes specified." );
    static_assert( sizeof...( Func_T ) > 0u, "No function to transform with." );

    return std::views::transform(
        [ &, func_table = std::make_tuple( std::forward< Func_T >( func )... ) ]
        < typename Tuple >( Tuple&& tuple )
        {
            using Tuple_T = std::remove_cvref_t< decltype( tuple ) >;
			
			static constexpr
			auto tuple_size = std::tuple_size_v< Tuple_T >;

			static_assert( ( ( parse_lane_index( Lanes, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );
			
			static constexpr
			auto highest_index = []
			{
				size_t max{ 0u };
				(void)( ( max = std::max( max, parse_lane_index( Lanes, tuple_size ) ) ),  ... );
				return max;
			}();

			// an array that maps output lane index to function storage index
			static constexpr
			auto func_table_indices = [&]< size_t... Index >( std::index_sequence< Index... > )
			{
				std::array< size_t, highest_index + 1u > indices{};
				(void)( ( indices[ parse_lane_index( Lanes, tuple_size ) ] = Index ), ... );
				return indices;
			}( std::make_index_sequence< sizeof...( Lanes ) >{} );

			// is the given index one of the lanes being transformed?
			static constexpr
			auto is_transformed = [&]( size_t index )
			{
				return ( ( parse_lane_index( Lanes, tuple_size ) == index ) || ... );
			};

            auto call_func = [&]< size_t Out_Lane, typename Arg_T >( Arg_T&& arg )
              -> decltype( auto )
            {
                if constexpr( is_transformed( Out_Lane ) )
                {
                    static constexpr
                    size_t index = func_table_indices[ Out_Lane ] % sizeof...( func );

                    return std::get< index >( func_table )( std::forward< Arg_T >( arg ) );
                }
                else
                {
                    return std::forward< Arg_T >( arg );
                }
            };

            return tuple_bind(
                [&]< typename... Args_T >( Args_T&&... args )
                {
                    return [&]< size_t... N >( std::index_sequence< N... > )
                    {
                        return std::make_tuple( call_func.template operator()< N >( std::forward< Args_T >( args ) )... );
                    }( std::make_index_sequence< tuple_size >{} );
                }
            )( std::forward< Tuple >( tuple ) );
        }
    );
}

// A transform view that applies the given function to every element of the input tuple.
template< typename Func >
auto transform_each( Func&& func )
  -> decltype( auto )
{
    return bind_view(
        [=]( auto&&... args )
        {
            return std::make_tuple( func( decltype( args )( args ) )... );
        }
    );
} 
