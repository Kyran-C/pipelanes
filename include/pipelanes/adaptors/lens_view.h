#pragma once

#include <ranges>
#include <type_traits>

#include "pipelanes/adaptors/adaptor_closure.h"
#include "pipelanes/adaptors/lens_view/proxy_range.h"
#include "pipelanes/lane_indices.h"



// A range adaptor which selects certain elements at specified indices, passes them through a side pipeline, 
//      and then joins the results with the original data.
template< std::ranges::range Upstream_T, typename Pipeline_T, int... Lanes >
class Lens_View : public std::ranges::view_interface< Lens_View< Upstream_T, Pipeline_T, Lanes... > >
{
    using Proxy_Range_T = Proxy_Range< Upstream_T, Lanes... >;
    using Upstream_Tuple_T = std::remove_cvref_t< decltype( *std::declval< Upstream_T >().begin() ) >;
	static_assert( std::is_invocable_v< Pipeline_T, Proxy_Range_T& > );
	using Composed_Pipeline_T = std::invoke_result_t< Pipeline_T, Proxy_Range_T& >;
    using Pipeline_Result_T = tuplify_t< std::decay_t< decltype( *std::declval< Composed_Pipeline_T >().begin() ) > >;

    static constexpr
    size_t tuple_size = std::tuple_size_v< std::remove_cvref_t< Upstream_Tuple_T > >;

    static constexpr
    size_t lens_count = sizeof...( Lanes );

    // number of elements in the lensed pipeline's result
    static constexpr
    size_t lens_out_count = std::tuple_size_v< Pipeline_Result_T >;

    static constexpr
    size_t output_count = tuple_size - lens_count + lens_out_count;

    static constexpr
    auto is_lensed = []( size_t N )
      -> bool
    { 
        return ( ( N == parse_lane_index( Lanes, tuple_size ) ) || ... ); 
    };

	static constexpr 
	size_t leftover_count = []< size_t... N >( std::index_sequence< N... > )
	{
		return ( ( is_lensed( N ) ? 0 : 1 ) + ... );
	}( std::make_index_sequence< tuple_size >{} );

    // if the lensed pipeline doesn't alter the number of elements, they can be put back in their original place
    static constexpr
    bool in_place_lens = lens_count == lens_out_count;

    static constexpr
    auto is_output_lensed = []( size_t N )
      -> bool
    {
        if constexpr( in_place_lens )
            return is_lensed( N );
        else
            return N >= leftover_count;
    };

	// for the given lensed output at lane n, get its index in the lensed pipeline
	static constexpr
	auto nth_lensed_output = []( auto nth )
	{
		size_t lane{};
		size_t count{};

		( void )
		( ( ( nth == Lanes ) ? ( lane = count, true ) : ( ++count, false )  ) || ... );

		return lane;
	};

    static constexpr
    std::array output_indices = []
    {
        std::array< size_t, output_count > result{};

        if constexpr( in_place_lens )
        {
            size_t lens_index{}, leftover_index{};
            for( size_t i{}; i < output_count; ++i )
            {
                result[ i ] = is_lensed( i ) ? nth_lensed_output( lens_index++ ) : leftover_index++;
            }
        }
        else
        {
            // index from the source tuple
            for( size_t i{}; i < leftover_count; ++i )
            {
                result[ i ] = i;
            }

            // index from the pipeline result tuple
            for( size_t source{}, i{ leftover_count }; i < output_count; ++i, ++source )
            {
                result[ i ] = source;
            }
        }
        
        return result;
    }();

    static constexpr
    std::array leftover_indices = []
    {
        std::array< size_t, leftover_count > result{};

        for( size_t index{}, i{}; i < leftover_count; ++i )
        {
            while( is_lensed( index ) )
                ++index;

            result[ i ] = index++;
        }

        return result;
    }();

    using Leftover_Tuple_T = decltype(
        []< size_t... I >( std::index_sequence< I... > )
          -> std::tuple< std::decay_t< decltype( std::get< leftover_indices[ I ] >( std::declval< Upstream_Tuple_T >() ) ) >... >
        {
            return std::forward_as_tuple( std::get< leftover_indices[ I ] >( std::declval< Upstream_Tuple_T >() )... );
        }( std::make_index_sequence< leftover_count >{} )
    );

    using Downstream_Tuple_T = decltype( std::tuple_cat( std::declval< Leftover_Tuple_T >(), std::declval< Pipeline_Result_T >() ) );
    

public:

    struct Sentinel{};
    

    struct Iterator
    {
        using iterator_concept = std::forward_iterator_tag;
        using value_type = Downstream_Tuple_T;
        using difference_type = std::ptrdiff_t;
        
        using Pipeline_Iterator_T = decltype( std::declval< Pipeline_T >()( std::declval< Proxy_Range_T& >() ).begin() );
        using Pipeline_Sentinel_T = decltype( std::declval< Pipeline_T >()( std::declval< Proxy_Range_T& >() ).end() );

        Iterator() = default;

        Iterator( const Lens_View* self, Pipeline_Iterator_T&& pipeline_iterator, Pipeline_Sentinel_T&& pipeline_sentinel )
            : _self{ self }
            , _pipeline_iterator{ pipeline_iterator }
            , _pipeline_sentinel{ pipeline_sentinel }
        {}

        Iterator( const Iterator& rhs ) = default;
        Iterator( Iterator&& rhs ) = default;

        auto operator=( const Iterator& rhs )
          -> Iterator&
           = default;

        auto operator=( Iterator&& rhs )
          -> Iterator&
           = default;

        auto operator*() const
          -> value_type
        {
            assert( _self );

            return
            [&]< size_t... Ns >( auto&& lensed, std::index_sequence< Ns... > )
            {
                assert( _self->_proxy_range._leftovers.has_value() );
                auto leftovers = _self->_proxy_range._leftovers.value();

                auto get_elem = [&]< size_t N >()
                {
                    if constexpr( is_output_lensed( N ) )
                    {
                        return std::get< output_indices[ N ] >( decltype( lensed )( lensed ) );
                    }
                    else
                    {
                        return std::get< output_indices[ N ] >( std::move( leftovers ) );
                    }
                };

                return Downstream_Tuple_T( get_elem.template operator()< Ns >()... );
            }( Pipeline_Result_T( *_pipeline_iterator ), std::make_index_sequence< output_count >{} );
        }

        auto operator++()
          -> Iterator&
        {
            ++_pipeline_iterator;
            return *this;
        }

        auto operator++(int)
            -> Iterator
        {
            Iterator copy( *this );
            ++_pipeline_iterator;
            return copy;
        }

        auto operator==( const Sentinel& ) const
          -> bool
        {
            return _pipeline_iterator == _pipeline_sentinel;
        }

        auto operator==( const Iterator& rhs ) const
          -> bool
        {
            return _pipeline_iterator == rhs._pipeline_iterator;
        }

    private:
        const Lens_View* _self{ nullptr };

        Pipeline_Iterator_T _pipeline_iterator;
        Pipeline_Sentinel_T _pipeline_sentinel;
    };


    static_assert( std::input_iterator< Iterator > );
    static_assert( std::sentinel_for< Sentinel, Iterator > );

    Lens_View( Upstream_T&& upstream, Pipeline_T pipeline )
        : _leftovers{ std::nullopt }
        , _proxy_range{ std::forward< Upstream_T >( upstream ), _leftovers }
		, _partial_pipeline{ std::move( pipeline ) }
        , _pipeline{ _partial_pipeline( _proxy_range ) }
    {}

	Lens_View( Lens_View< Upstream_T, Pipeline_T, Lanes... > const& other )
		: _leftovers{ other._leftovers }
		, _proxy_range{ other._proxy_range._upstream, _leftovers }
		, _partial_pipeline{ other._partial_pipeline }
		, _pipeline{ _partial_pipeline( _proxy_range ) }
	{}

	Lens_View( Lens_View< Upstream_T, Pipeline_T, Lanes... >&& other )
		: _leftovers{ std::move( other._leftovers ) }
		, _proxy_range{ std::move( other._proxy_range )._upstream, _leftovers }
		, _partial_pipeline{ std::move( other._partial_pipeline ) }
		, _pipeline{ _partial_pipeline( _proxy_range ) }
	{
	}

	~Lens_View() noexcept
	{}

	auto operator=( Lens_View< Upstream_T, Pipeline_T, Lanes... > const& other )
	  -> Lens_View< Upstream_T, Pipeline_T, Lanes... >&
	{
		_leftovers = other._leftovers;
		_proxy_range = { other._proxy_range._upstream, _leftovers };
		_partial_pipeline = other._partial_pipeline;
		_pipeline = { _partial_pipeline( _proxy_range ) };
	}

	auto operator=( Lens_View< Upstream_T, Pipeline_T, Lanes... >&& other )
	  -> Lens_View< Upstream_T, Pipeline_T, Lanes... >&
	{
		_leftovers = std::move( other._leftovers );
		_proxy_range = std::move( other._proxy_range );
		_partial_pipeline = std::move( other._partial_pipeline );
		_pipeline = { _partial_pipeline( _proxy_range ) };
	}

    // auto begin() const
    //   -> Iterator
    // {
    //     return Iterator( this, _pipeline.begin(), _pipeline.end() );
    // }
    
    // auto end() const
    //   -> Sentinel
    // {
    //     return Sentinel{};
    // }

    auto begin()
		 noexcept
      -> Iterator
    {
        return Iterator( this, _pipeline.begin(), _pipeline.end() );
    }
    
    auto end()
		 noexcept
      -> Sentinel
    {
        return Sentinel{};
    }

private:
    using Upstream_Iterator_T = decltype( std::declval< Upstream_T >().begin() );

    using Pipeline = decltype( std::declval< Pipeline_T >()( std::declval< Proxy_Range_T& >() ) );
	static_assert( std::ranges::range< Pipeline > );

    std::optional< Leftover_Tuple_T > _leftovers{}; 
    Proxy_Range_T _proxy_range;
	Pipeline_T _partial_pipeline;
    Pipeline  _pipeline;
};

template< typename Upstream_T, typename Pipeline_T, int... Lanes >
Lens_View( Upstream_T&& upstream, Pipeline_T&& pipeline ) -> Lens_View< Upstream_T, Pipeline_T, Lanes... >;


// A range adaptor which selects certain elements at the specified indices, passes them through a side pipeline, 
//      and then joins the results with the original data.
template< int... Lanes, typename Pipeline_T >
auto lens( Pipeline_T&& pipeline )
{
	return Range_Adaptor_Closure(
		[ pipeline = std::forward< Pipeline_T >( pipeline ) ]
		< typename Upstream_T >
		( this auto&& self, Upstream_T&& upstream )
		{
			// TODO remove so lvalue refs are permitted
			static_assert( std::is_same_v< decltype( pipeline ), std::remove_cvref_t< decltype( pipeline ) > > );

			using Self_T = decltype( self );

			static constexpr
			bool is_rvalue = std::is_rvalue_reference_v< Self_T >;
			using Pipeline_Type = std::remove_cv_t< std::conditional_t< is_rvalue
													, Pipeline_T
													, decltype( pipeline ) > >;

			static_assert( std::is_same_v< Pipeline_Type, decltype( pipeline ) > );
			static_assert( !std::is_const_v< std::remove_reference_t< Self_T > > );

			static_assert( std::is_same_v< Pipeline_Type, std::remove_cvref_t< Pipeline_Type > > );

			using Lens_View_Type = Lens_View< Upstream_T, Pipeline_T, Lanes... >;

			return Lens_View_Type( std::forward< Upstream_T >( upstream ), std::forward< Pipeline_Type >( pipeline ) );
		}
	);
}


// A range adaptor which selects every element except those at the specified indices, passes them through a side pipeline, 
//      and then joins the results with the original data.
template< int... Lanes, typename Pipeline_T >
auto unlens( Pipeline_T&& pipeline )
{
	return Range_Adaptor_Closure(
		[ pipeline = std::forward< Pipeline_T >( pipeline ) ]
		< typename Upstream_T >
		( this auto&& self, Upstream_T&& upstream )
		{
			using Self_T = decltype( self );

			static constexpr
			bool is_rvalue = std::is_rvalue_reference_v< Self_T >;

			using Pipeline_Type = std::conditional_t< is_rvalue
													, Pipeline_T
													, decltype( pipeline ) >;

			using Upstream_Value_T = std::decay_t< decltype( *upstream.begin() ) >;

			static constexpr
			auto upstream_size = std::tuple_size_v< Upstream_Value_T >;

			return array_unpack< index_complement< upstream_size, Lanes... >() >(
				[ & ]
				< size_t... Included_Lanes >
				{
					using Lens_View_Type = Lens_View< Upstream_T, Pipeline_T, Included_Lanes... >;

					return Lens_View_Type( std::forward< Upstream_T >( upstream ), std::forward< Pipeline_Type >( pipeline ) );
				}
			);
		}
	);
}

