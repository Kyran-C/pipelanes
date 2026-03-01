#pragma once

#include <ranges>
#include <type_traits>

template< typename F >
struct Range_Adaptor_Closure : std::ranges::range_adaptor_closure< Range_Adaptor_Closure< F > >
{
	F f;

	Range_Adaptor_Closure( F&& f )
		: f{ std::forward< F >( f ) }
	{}

	template< std::ranges::range Range_T >
	requires std::is_invocable_v< F&&, Range_T& >
	constexpr
	auto operator()( Range_T& range ) &&
	  -> auto
	{
		return std::move( f )( range );
	}

	// template< std::ranges::range Range_T >
	// requires std::is_invocable_v< F const&, Range_T& >
	// constexpr
	// auto operator()( Range_T& range ) const &
	//   -> auto
	// {
	// 	return f( range );
	// }

	template< std::ranges::range Range_T >
	requires std::is_invocable_v< F&, Range_T& >
	constexpr
	auto operator()( Range_T& range ) &
	  -> auto
	{
		return f( range );
	}

	template< std::ranges::range Range_T >
	requires std::is_invocable_v< F&&, Range_T&& >
	constexpr
	auto operator()( Range_T&& range ) &&
	  -> auto
	{
		return std::move( f )( std::forward< Range_T >( range ) );
	}

	// template< std::ranges::range Range_T >
	// requires std::is_invocable_v< F const&, Range_T&& >
	// constexpr
	// auto operator()( Range_T&& range ) const &
	//   -> auto
	// {
	// 	return f( std::forward< Range_T >( range ) );
	// }

	template< std::ranges::range Range_T >
	requires std::is_invocable_v< F&, Range_T&& >
	constexpr
	auto operator()( Range_T&& range ) &
	  -> auto
	{
		return f( std::forward< Range_T >( range ) );
	}
};

template< typename F >
Range_Adaptor_Closure( F&& f ) -> Range_Adaptor_Closure< F >;


