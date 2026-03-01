#pragma once

#include <format>
#include <ranges>
#include <utility>

#include "pipelanes/tuple_utils.h"

// A transform view which formats a tuple to a string.
auto format_tuple( auto&& format_str )
{
	return std::views::transform(
		[=]< typename Tuple >( Tuple&& tup )
		{
			return tuple_bind(
				[=]( auto... args )
				{
					return std::vformat( format_str, std::make_format_args( decltype( args )( args )... ) );
				}
			)( std::forward< Tuple >( tup ) );
		}
	);
}

