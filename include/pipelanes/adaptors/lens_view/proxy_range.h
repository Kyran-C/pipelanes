#pragma once

#include <array>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

#include "pipelanes/tuple_utils.h"
#include "pipelanes/lane_indices.h"
#include "pipelanes/type_fail.h"

// The proxy range sits between the upstream pipeline and the lensed pipeline, 
//      and caches the leftover upstream values so they can be later joined with the rest of the results.
template< std::ranges::range Upstream_T, int... Lanes >
struct Proxy_Range : std::ranges::view_interface< Proxy_Range< Upstream_T, Lanes... > >
{
    using Upstream_Tuple_T = tuplify_t< std::remove_cvref_t< decltype( *std::declval< Upstream_T >().begin() ) > >;
    
    static constexpr
    size_t tuple_size = std::tuple_size_v< std::remove_cvref_t< Upstream_Tuple_T > >;

    static constexpr
    size_t lens_count = sizeof...( Lanes );

    static constexpr
    auto is_lensed = []( size_t N )
	{
		return ( ( N == parse_lane_index( Lanes, tuple_size ) ) || ... );
	};

	static constexpr 
	size_t leftover_count = []< size_t... N >( std::index_sequence< N... > )
	{
		return ( ( is_lensed( N ) ? 0 : 1 ) + ... );
	}( std::make_index_sequence< tuple_size >{} );

    static constexpr
    std::array leftover_indices = []
    {
        std::array< size_t, leftover_count > result{};

        for( size_t index{}, i{}; i < leftover_count; ++i )
        {
            while( Proxy_Range::is_lensed( index ) )
                ++index;

            result[ i ] = index++;
        }

        return result;
    }();

	// static_assert( type_fail< decltype( leftover_indices ) >, "Type debugging!" );
	// static_assert( leftover_count == 49 );
	// static_assert( type_fail< Upstream_Tuple_T >, "Type debugging!" );

	static constexpr
	auto __make_leftovers = 
        []< size_t... I >( std::index_sequence< I... >, Upstream_Tuple_T& tuple )
          -> std::tuple< std::decay_t< decltype( std::get< leftover_indices[ I ] >( std::declval< Upstream_Tuple_T >() ) ) >... >
		  // -> decltype( auto )
        {
            // return std::make_tuple( std::get< leftover_indices[ I ] >( std::declval< Upstream_Tuple_T >() )... );
            return { std::get< leftover_indices[ I ] >( tuple )... };
        };
    using Leftover_Tuple_T = decltype(
		__make_leftovers( std::make_index_sequence< leftover_count >{}, std::declval< Upstream_Tuple_T& >() )
    );

    template< bool is_const >
    struct Sentinel;

    template< bool is_const >
    struct Iterator;
    
    static_assert( std::input_iterator< Iterator< true > > );
    static_assert( std::sentinel_for< Sentinel< true >, Iterator< true > > );
    static_assert( std::input_iterator< Iterator< false > > );
    static_assert( std::sentinel_for< Sentinel< false >, Iterator< false > > );

    template< size_t... >
    Proxy_Range( Upstream_T&& upstream, std::optional< Leftover_Tuple_T >& leftovers )
        : _upstream{ std::forward< Upstream_T >( upstream ) }
        , _leftovers{ leftovers }
    {        
    }

    //Proxy_Range( const Proxy_Range& ) = delete;
    Proxy_Range( const Proxy_Range& ) = default;
    
    auto operator=( const Proxy_Range& )
      -> Proxy_Range&
       = default;
    //   = delete;
    
    Proxy_Range( Proxy_Range&& ) = default;

    auto operator=( Proxy_Range&& )
      -> Proxy_Range&
       = default;

    auto begin() const
    {
        return Iterator< true >( this, _upstream.begin() );
    }

    auto begin()
    {
        return Iterator< false >( this, _upstream.begin() );
    }

    auto end()
      -> Sentinel< false >
    {
        return { _upstream.end() };
    }

    auto end() const
      -> Sentinel< true >
    {
        return { _upstream.end() };
    }

    Upstream_T _upstream;
    std::optional< Leftover_Tuple_T >& _leftovers;
};

template< typename Upstream_T, int... Lanes >
Proxy_Range( Upstream_T&& ) -> Proxy_Range< Upstream_T, Lanes... >;

template< std::ranges::range Upstream_T, int... Lanes >
template< bool is_const >
struct Proxy_Range< Upstream_T, Lanes... >
        ::Sentinel
{
    using Upstream_Sentinel_T = decltype( std::declval< Upstream_T >().end() );
    Upstream_Sentinel_T _upstream_sentinel;
};

// Proxy Range Iterator
template< std::ranges::range Upstream_T, int... Lanes >
template< bool is_const >
struct Proxy_Range< Upstream_T, Lanes... >
        ::Iterator
{
    using iterator_concept = std::forward_iterator_tag;
    using value_type = decltype( tuple_elems< parse_lane_index( Lanes, tuple_size )... >( std::declval< std::remove_cvref_t< Upstream_Tuple_T > >() ) );
    using difference_type = std::ptrdiff_t;

    auto operator*() const
      -> value_type
    {
        auto tuple = *_upstream_iterator;

        [&]< size_t... N >( std::index_sequence< N... > )
        {
            _proxy->_leftovers = tuple_elems< leftover_indices[ N ]... >( tuple );
        }( std::make_index_sequence< leftover_count >{} );

        return
        [&]< size_t... N >( std::index_sequence< N... > )
        {
            return tuple_elems< parse_lane_index( Lanes, tuple_size )... >( tuple );
        }( std::make_index_sequence< lens_count >{} );
    }

    auto operator++()
      -> Iterator&
    {
        ++_upstream_iterator;
        return *this;
    }

    auto operator++(int)
      -> Iterator
    {
        Iterator copy( *this );
        ++_upstream_iterator;
        return copy;
    }

    auto operator==( const Sentinel< is_const >& sentinel ) const
      -> bool
    {
        return _upstream_iterator == sentinel._upstream_sentinel;
    }

    const Proxy_Range< Upstream_T, Lanes... >* _proxy;
    using Upstream_Iterator_T = decltype( std::declval< Upstream_T >().begin() );
    Upstream_Iterator_T _upstream_iterator;
};
