#pragma once

#include <iostream>
#include <format>
#include <ranges>
#include <tuple>
#include <utility>

#include "algebraic_types/algebraic_traits.h"
#include "lane_indices.h"

// Selects the specified elements from the given tuple.
template< size_t... Elem_N, typename Tuple_T >
constexpr
auto tuple_elems( Tuple_T&& tuple )
{
	using Tuple = std::remove_cvref_t< Tuple_T >;
	using Result_T = std::tuple< std::tuple_element_t< Elem_N, Tuple >... >;
	return Result_T{ ( std::tuple_element_t< Elem_N, Tuple > )std::get< Elem_N >( std::forward< Tuple_T >( tuple ) )... };
}

// Bind for tuples.
// Unwraps the tuple and passes its values as arguments to the function.
auto tuple_bind( auto&& func )
{
	return
	[ func = decltype( func )( func ) ]
	< typename Tuple >
	requires( Tuple_Like< std::remove_cvref_t< Tuple > > )
	( Tuple&& tuple )
	{
		static constexpr
		size_t tuple_size = std::tuple_size_v< std::remove_cvref_t< Tuple > >;

		return [&]< size_t... N >( std::index_sequence< N... > )
		{
			return func( std::get< N >( decltype( tuple )( tuple ) )... );
		}( std::make_index_sequence< tuple_size >{} );
	};
}

// Fmap for tuples.
// Unwraps the tuple and passes its values as arguments to the function, then wraps the result in a tuple.
auto tuple_fmap( auto&& func )
{
	return
	[ func = decltype( func )( func ) ]
	< typename Tuple >
	requires( Tuple_Like< std::remove_cvref_t< Tuple > > )
	( Tuple&& tuple )
	{
		static constexpr
		size_t tuple_size = std::tuple_size_v< std::remove_cvref_t< Tuple > >;

		return [&]< size_t... N >( std::index_sequence< N... > )
		{
			return std::tuple{ func( std::get< N >( decltype( tuple )( tuple ) )... ) };
		}( std::make_index_sequence< tuple_size >{} );
	};
}

// Pretty prints a tuple, using the given format string (if specified).
inline
auto print_tuple =
[]< typename... Elem_Ts, typename Format_String = decltype( "{: >6} " ) >
( std::tuple< Elem_Ts... > const& tuple, Format_String&& format_string = "{: >6} " )
{
	auto format = [&]< typename Arg_T >( Arg_T&& arg, auto& recurse )
	{
		if constexpr( Tuple_Like< std::remove_cvref_t< Arg_T > > )
		{
			std::cout << "( ";
			tuple_bind(
				[&]( auto... args )
				{
					( recurse( args, recurse ), ... );
				}
			)( std::forward< Arg_T >( arg ) );
			std::cout << " )";
		}
		else
		{
			std::cout << std::vformat( format_string, std::make_format_args( arg ) );
		}
	};

	format( tuple, format );
	std::cout << "\n";
};

namespace detail
{

struct Nested_Index
{
	size_t base;
	size_t nested;
	bool is_flattened;
};

template< size_t... Indices >
using Seq = std::index_sequence< Indices... >;

template< size_t N, size_t M >
constexpr
auto operator +( std::array< Nested_Index, N > lhs, std::array< Nested_Index, M > rhs )
  -> std::array< Nested_Index, N + M >
{

	return [&]< size_t... Ns, size_t... Ms >( Seq< Ns... >, Seq< Ms... > )
	  -> std::array< Nested_Index, N + M >
	{
		return
			{ ( std::get< Ns >( lhs ) )...
			, ( std::get< Ms >( rhs ) )...
			};
	}( std::make_index_sequence< N >(), std::make_index_sequence< M >() );
}

template< bool nested, typename Elem_T >
consteval
auto indices_for_element( size_t base_index )
{
	if constexpr( nested && Tuple_Like< Elem_T > )
	{
		//static constexpr
		//size_t tuple_size = std::tuple_size_v< std::remove_cvref_t< Elem_T > >;

		//auto make_indices = 
		return
		[=]< size_t... N >( std::index_sequence< N... > )
		{
			return std::array{ Nested_Index{ base_index, N, true }... };
		}( std::make_index_sequence< std::tuple_size_v< std::remove_cvref_t< Elem_T > > >() );

		//return make_indices;
	}
	else
	{
		return std::array{ Nested_Index{ base_index, 0u, false } };
	}
}

template< Nested_Index Index >
auto get_nested( auto&& tuple )
{
	if constexpr( Index.is_flattened )
	{
		return std::get< Index.nested >( std::get< Index.base >( std::forward< decltype( tuple ) >( tuple ) ) );
	}
	else
	{
		return std::get< Index.base >( std::forward< decltype( tuple ) >( tuple ) );
	}
}
	
}

// Flatten any nested tuples at the specified indices.
// If no indices are provided, all nested tuples are flattened.
template< int... Ns >
auto flatten_tuple_elements( auto&& tuple )
{
	using Tuple_T = std::remove_cvref_t< std::decay_t< decltype( tuple ) > >;

	static constexpr
	size_t tuple_size{ std::tuple_size_v< Tuple_T > };
	
	static_assert( ( ( parse_lane_index( Ns, tuple_size ) < tuple_size ) && ... ), "Lane index exceeds lane count." );

	static constexpr
	bool flatten_all = sizeof...( Ns ) == 0u;

	static constexpr
	auto is_selected = [&]( size_t N )
	{
		return flatten_all
			|| ( ( N == parse_lane_index( Ns, tuple_size ) ) || ... );
	};

	static constexpr
	auto indices = []< size_t... N >( std::index_sequence< N... > )
	{
		return
		( ( detail::indices_for_element< is_selected( N ), std::tuple_element_t< N, Tuple_T > >( N ) ) + ... );
	}( std::make_index_sequence< tuple_size >() );

	return [&]< size_t... N >( std::index_sequence< N... > )
	{
		return std::make_tuple( detail::get_nested< indices[ N ] >( tuple )... );
	}( std::make_index_sequence< indices.size() >() );
}


template< typename T >
struct Tuplify
{
	using type = std::tuple< T >;
};

template< typename... Args_T >
struct Tuplify< std::tuple< Args_T... > >
{
	using type = std::tuple< Args_T... >;
};

template< typename... Ts >
using tuplify_t = typename Tuplify< Ts... >::type;
