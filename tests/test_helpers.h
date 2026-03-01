#pragma once

#include <vector>
#include <tuple>

static constexpr
auto consume = []( auto&& s )
{
	(void)s;
};


inline
auto make_vec()
  -> std::vector< std::tuple< int, int, int > >
{
    return {
		{ 1, 2, 3 },
		{ 4, 5, 6 },
		{ 7, 8, 9 }
	};
}

static constexpr
auto id = []( auto&& arg )
{
	return decltype( arg )( arg );
};

static constexpr
auto sum = []( auto arg, auto arg2 )
{
	return arg + arg2;
};

static constexpr
auto multiply = []( auto arg, auto arg2 )
{
	return arg * arg2;
};

static constexpr
auto minus_one = []( auto arg )
{
	return arg - 1;
};


static constexpr
auto plus_one = []( auto arg )
{
	return arg + 1;
};


static constexpr
auto plus_ten = []( auto arg )
{
	return arg + 10;
};

static constexpr
auto mul_ten = []( auto arg )
{
	return arg * 10;
};

static constexpr
auto is_even = []( int a )
{
	return a % 2 == 0;
};

static constexpr
auto is_odd = []( int a )
{
	return a % 2 != 0;
};

static constexpr
auto is_nonzero = []( int a )
{
	return a != 0;
};


static constexpr
auto sort_pair_elements = []( auto&& left, auto&& right )
{
	if( left < right )
	{
		return std::make_tuple(
			decltype( left )( left ),
			decltype( right )( right )
		);
	}
	else
	{
		return std::make_tuple(
			decltype( right )( right ),
			decltype( left )( left )
		);
	}
};

// Creates a function which returns the original value.
// Note that values are forwarded, so the resultant function can only be called once (an input_range).
template< typename Arg_T >
auto unit( Arg_T&& arg )
{
    return
    [ arg = std::forward< Arg_T >( arg )]
    ()
    {
        return std::move( arg ); // arg is moved here, so unit can only be called once
    };
}


// performs fmap `count` times on an optional value
template< size_t count >
auto fmap_times =
    []( auto&& left, auto&& right )
    {
        return
        [ =
        , left = decltype( left )( left )
        , right = decltype( right )( right ) ]
        ( auto&&... args )
        {
            auto result = left( decltype( args )( args )... );
            
            auto invoke = [&]< size_t nth >( auto& self )
              -> decltype( auto )
            {
                if constexpr( nth == 1u )
                {
                    return right( std::move( result ).value() );
                }
                else
                {
                    return right( self.template operator()< nth - 1u >( self ) );
                }
            };


            auto get = [&]
            { 
                return std::make_optional( invoke.template operator()< count >( invoke ) );
            };

            if( result.has_value() )
                return get();
            else
                return decltype( get() ){ std::nullopt };
        };
    };



