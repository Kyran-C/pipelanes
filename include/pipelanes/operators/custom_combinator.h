#pragma once

#include <utility>

// Closure which captures a combinator and its left operand.
template< typename First_F, typename Combinator_T >
struct Combinator_Closure
{
    First_F _first;
    Combinator_T _combinator;
};

// Operator for user-defined combinators. 
// This is paired with the > operator to enable user-defined infix operators.
// The syntax is as follows: ` left_op <combinator> right_op`
template< typename First_F, typename Combinator_T >
auto operator <( First_F&& first, Combinator_T&& combinator )
{
    return Combinator_Closure< First_F, Combinator_T >{ first, combinator };
}

// The closing tag of a user-defined combinator.
template< typename First_F, typename Combinator_T , typename Second_F >
auto operator >( Combinator_Closure< First_F, Combinator_T >&& closure, Second_F&& second )
{
    return std::move( closure )._combinator( std::forward< First_F >( closure._first ), std::forward< Second_F >( second ) );
}
