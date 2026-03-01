#include <cassert>
#include <ranges>
#include <map>
#include <functional>
#include <string_view>
#include <vector>

#include "pipelanes/overload_set.h"
#include "pipelanes/tuple_utils.h"
#include "pipelanes/from.h"

#include "pipelanes/adaptors/transform.h"
#include "pipelanes/adaptors/lensed_filter.h"
#include "pipelanes/adaptors/lens_view.h"
#include "pipelanes/adaptors/select.h"
#include "pipelanes/adaptors/flatten.h"
#include "pipelanes/adaptors/gather.h"
#include "pipelanes/adaptors/bind.h"
#include "pipelanes/adaptors/fmap.h"
#include "pipelanes/adaptors/print_elements.h"

#include "pipelanes/operators/bind.h"
#include "pipelanes/operators/fmap.h"
#include "pipelanes/operators/compose.h"
#include "pipelanes/operators/range_extractor.h"
#include "pipelanes/operators/custom_combinator.h"

#include "test_helpers.h"

static constexpr
bool break_on_failure{ false };

// test framework?
// no, we have test framework at home

auto populate_tests()
{
	using namespace std::views;
	using namespace std::ranges;
	using std::vector;
	using std::tuple;

	std::map< std::string_view, std::function< bool() > > tests;

	tests[ "Transform: transform_from" ] =
	[]()
	{
		auto result = 
			make_vec()
			| transform_from< 0, 2 >( sum )
			| to< vector >();

		return result == vector{ 4, 10, 16 };
	};

	tests[ "Transform: transform_indexed" ] =
	[]()
	{
		auto result =
			iota( 0, 10 )
			| transform_indexed( []( size_t i, int val ){ return int( i + val ); } )
			| to< vector >();

		return result == vector{ 0, 2, 4, 6, 8, 10, 12, 14, 16, 18 };
	};

	tests[ "Transform: transform_to_front" ] =
	[]()
	{
		auto result =
			iota( 0, 10 )
			| transform_to_front( plus_ten )
			| transform_to_front( from< 1 >, mul_ten )
			| to< vector >();

		return result == vector< tuple< int, int, int > >{
			{  0, 10, 0 },
			{ 10, 11, 1 },
			{ 20, 12, 2 },
			{ 30, 13, 3 },
			{ 40, 14, 4 },
			{ 50, 15, 5 },
			{ 60, 16, 6 },
			{ 70, 17, 7 },
			{ 80, 18, 8 },
			{ 90, 19, 9 }
		};
	};

	tests[ "Transform: transform_to_back" ] =
	[]()
	{
		auto result =
			iota( 0, 10 )
			| transform_to_back( plus_ten )
			| transform_to_back( from< 0 >, mul_ten )
			| to< vector >();

		return result == vector< tuple< int, int, int > >{
			{ 0, 10,  0 },
			{ 1, 11, 10 },
			{ 2, 12, 20 },
			{ 3, 13, 30 },
			{ 4, 14, 40 },
			{ 5, 15, 50 },
			{ 6, 16, 60 },
			{ 7, 17, 70 },
			{ 8, 18, 80 },
			{ 9, 19, 90 }
		};
	};

	tests[ "Transform: transform_to" ] =
	[]()
	{
		auto result =
			make_vec()
			| transform_to< 1 >( []( auto tup ){ auto [a, b, c] = tup; return a + b + c; } )
			| transform_to< 0 >( from< 2 >, mul_ten )
			| to< vector >();

		return result == vector< tuple< int, int, int > >{
			{ 30, 6,  3 },
			{ 60, 15, 6 },
			{ 90, 24, 9 }
		};
	};

	tests[ "Transform: transform_nth" ] =
	[]()
	{
		auto result =
			make_vec()
			| transform_nth< 0, 2 >( mul_ten )
			| transform_nth< 0, 2 >( plus_one, minus_one )
			| transform_nth< -2 >( plus_ten )
			| to< vector >();

		return result == vector< tuple< int, int, int > >{
			{ 11, 12, 29 },
			{ 41, 15, 59 },
			{ 71, 18, 89 }
		};
	};

	tests[ "Transform: transform_each" ] =
	[]()
	{
		auto result =
			make_vec()
			| transform_each( mul_ten )
			| to< vector >();

		return result == vector< tuple< int, int, int > >{
			{ 10, 20, 30 },
			{ 40, 50, 60 },
			{ 70, 80, 90 }
		};
	};

	tests[ "Filter: filter_lens" ] =
	[]()
	{
		auto result =
			make_vec()
			| filter_lens< 0, 2 >( []( auto a, auto b ){ return is_odd( a ) && is_odd( b ); } )
			| to< vector >();

		return result == vector< tuple< int, int, int > >{
			{ 1, 2, 3 },
			{ 7, 8, 9 }
		};
	};

	tests[ "Filter: filter_lens_all" ] =
	[]()
	{
		auto result =
			make_vec()
			| filter_lens_all< 0, 1 >( is_odd, is_even )
			| filter_lens_all< 0, 2 >( is_odd )
			| filter_lens_all( is_nonzero )
			| to< vector >();

		return result == vector< tuple< int, int, int > >{
			{ 1, 2, 3 },
			{ 7, 8, 9 }
		};
	};

	tests[ "Filter: filter_lens_any" ] =
	[]()
	{
		auto result =
			make_vec()
			| filter_lens_any< 0, 0 >( is_odd, is_even )
			| filter_lens_any( is_odd )
			| filter_lens_any< 1 >( is_odd )
			| to< vector >();

		return result == vector< tuple< int, int, int > >{
			{ 4, 5, 6 }
		};
	};

	tests[ "Select: select_lanes" ] =
	[]()
	{
		auto result =
			make_vec()
			| select_lanes< 0, 1, 2, 0 >()
			| select_lanes< 1, 3, 2 >()
			| to< vector >();

		return result == vector< tuple< int, int, int > >{
			{ 2, 1, 3 },
			{ 5, 4, 6 },
			{ 8, 7, 9 }
		};
	};

	tests[ "Gather: gather_to_front" ] =
	[]()
	{
		auto result =
			make_vec()
			| gather_to_front< 0, -1 >()
			| gather_to_front< 0, 1 >()
			| to< vector >();

		return result == vector< tuple< tuple< tuple< int, int >, int > > >{
			{ { { 1, 3 }, 2 } },
			{ { { 4, 6 }, 5 } },
			{ { { 7, 9 }, 8 } }
		};
	};

	tests[ "Gather: gather_to_back" ] =
	[]()
	{
		auto result =
			make_vec()
			| gather_to_back< 0 >()
			| gather_to_back< 0, 1, -2 >()
			| to< vector >();

		return result == vector< tuple< tuple< int >, tuple< int, int, int > > >{
			{ { 1 }, { 2, 3, 3 } },
			{ { 4 }, { 5, 6, 6 } },
			{ { 7 }, { 8, 9, 9 } }
		};
	};

	tests[ "Flatten: flatten" ] =
	[]()
	{
		auto result =
			make_vec()
			| gather_to_back< 0 >()
			| gather_to_front< -1, -2 >()
			| flatten()
			| gather_to_back< -1 >()
			| flatten< 0 >()
			| to< vector >();

		return result == vector< tuple< int, int, tuple< int > > >{
			{ 1, 3, { 2 } },
			{ 4, 6, { 5 } },
			{ 7, 9, { 8 } }
		};
	};

	tests[ "Lens: lens" ] =
	[]()
	{
		auto result =
			all( make_vec() )
			| lens< 0, 1 >
				( transform_each( mul_ten )
				| transform_each( plus_one )
				)
			| lens< 2, 0 >		// test that the lanes are passed in the order specified, and placed back into the right lanes
				( bind_view( sort_pair_elements )
				)
			| to< vector >()
			;

		return result == vector< tuple< int, int, int > >{
			{ 11, 21, 3 },
			{ 41, 51, 6 },
			{ 71, 81, 9 }
		};
	};


	tests[ "Lens: lens overflow" ] =
	[]()
	{
		auto result =
			all( make_vec() )
			| lens< 0, 1 >
				( transform_to_back( from< 0, 1 >, sum )
				)
			| to< vector >()
			;

		return result == vector< tuple< int, int, int, int > >{
			{ 3, 1, 2,  3 },
			{ 6, 4, 5,  9 },
			{ 9, 7, 8, 15 }
		};
	};


	tests[ "Lens: lens underflow" ] =
	[]()
	{
		auto result =
			all( make_vec() )
			| lens< 0, 1 >
				( bind_view( sum )
				)
			| to< vector >()
			;

		return result == vector< tuple< int, int > >{
			{ 3,  3 },
			{ 6,  9 },
			{ 9, 15 }
		};
	};


	tests[ "Lens: unlens" ] =
	[]()
	{
		auto result =
			all( make_vec() )
			| unlens< 2 >
				( transform_each( mul_ten )
				| transform_each( plus_one )
				)
			| to< vector >()
			;

		return result == vector< tuple< int, int, int > >{
			{ 11, 21, 3 },
			{ 41, 51, 6 },
			{ 71, 81, 9 }
		};
	};

	tests[ "Lens: nested lens" ] =
	[]()
	{
		auto result =
			all( make_vec() )
			| lens< 1, 0 >
				( transform_to_back( from< 0, 1 >, sum )
				| lens< 0, 1 >
					( bind_view( multiply )
					)		// narrowing from 2 lanes to 1
				| lens< -2 >
					( transform_to_front( from< 0 >, minus_one )
					)		// expanding from 1 lane to 2
				| lens< 0, 1 >
					( transform_each( mul_ten )
					)
				)
			| to< vector >()
			;

		return result == vector< tuple< int, int, int, int > >{
		{  3,  20,  20,  3, },
		{  6, 200,  80,  9, },
		{  9, 560, 140, 15, }
		};
	};


	tests[ "Lens: complete lens" ] =
	[]()
	{
		auto result =
			all( make_vec() )
			| lens< 0, 1, 2 >
				( transform_each( mul_ten )
				)
			| to< vector >()
			;

		return result == vector< tuple< int, int, int > >{
			{ 10, 20, 30 },
			{ 40, 50, 60 },
			{ 70, 80, 90 }
		};
	};

	tests[ "Custom Combinator: repeated fmap" ] =
	[]()
	{
		// no ranges here, just basic function composition
		// suppose we want to invoke a function 4 times on an optional value...
		// we define `fmap_times` to repeatedly invoke fmap on an optional

		auto plus_25 = []( int a ){ return a + 25; };

		auto result =
		    unit( std::optional< int >( 15 ) ) // create a function that just returns back the value
		    <fmap_times< 4 >> plus_25 // adds 100
			;

		return result() == std::optional< int >{ 115 };
	};

	tests[ "Custom Combinator: heterogenous operations" ] =
	[]()
	{
		using vec_opt_str = std::vector< std::optional< std::string_view > >;
		using vec_opt_size = std::vector< std::optional< size_t > >;

		// we want to transform this range of optionals by invoking fmap repeatedly on each one
		auto words = vec_opt_str{ { "Hello" }, { "World!" }, std::nullopt };

		auto result =
			all( words )
			| transform(
					id
					<fmap_times< 7 >>
						Overload_Set // we invoke this overload set 7 times, producing a different result type each time
						{
							[]( std::string_view str ){ return str.length(); },
							[]( size_t num ){ return num % 2 == 0u; },
							[]( bool p ) -> size_t{ return p ? 69 : 420; }
						}
				)
			| to< std::vector >()
			;

		return result == vec_opt_size{ { 420 }, { 69 }, std::nullopt };
	};


	return tests;
}

constexpr
auto println = []( auto... args )
{
	( std::cout << ... << args ) << std::endl;
};


int main( int, char** )
{
	auto tests = populate_tests();

	size_t passed_count{}, failed_count{};

	for( auto& [name, run] : tests )
	{
		auto result = run();
		println( "[ ",result ? "  OK  " : "FAILED", " ] - ", name );

		if( result )
			++passed_count;
		else
			++failed_count;

		if constexpr( break_on_failure )
			assert( result );
	}

	println( "Passed: ", passed_count, "\nFailed: ", failed_count );
}
