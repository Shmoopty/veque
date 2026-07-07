/*
 *
 * veque::veque test suite.
 *
 * Exercises assign(size_type, const T&) across every capacity state
 * (empty, shrink, same-size, grow-in-place, grow-with-reallocation),
 * plus a variety of empty / zero-length operations.  The in-place grow
 * branch of assign(count, value) was previously uncovered.
 *
 *  Copyright (C) 2019 Drew Dormann
 *
 */

#include <string>
#include <vector>
#include "catch.hpp"
#include "test_types.h"


TEMPLATE_PRODUCT_TEST_CASE( "assign(count,value) across capacity states", "[veque::veque][assign][template]",
    ( StdVeque, AllocCountingVeque ), (
        ( int, veque::fast_resize_traits ), ( int, veque::std_vector_traits ), ( int, veque::no_reserve_traits ), ( int, front_vector_traits ),
        ( std::string, veque::fast_resize_traits ), ( std::string, veque::no_reserve_traits ),
        ( std::vector<int>, veque::fast_resize_traits ), ( std::vector<int>, veque::std_vector_traits )
    ) )
{
    using V = TestType;
    using T = typename V::value_type;
    const T A = val<T,4>;
    const T B = val<T,5>;

    for ( std::size_t reserve_front : { std::size_t(0), std::size_t(4), std::size_t(16) } )
    for ( std::size_t reserve_back  : { std::size_t(0), std::size_t(4), std::size_t(16) } )
    for ( std::size_t start  : { std::size_t(0), std::size_t(3), std::size_t(10) } )
    for ( std::size_t target : { std::size_t(0), std::size_t(1), std::size_t(3), std::size_t(7), std::size_t(10), std::size_t(25), std::size_t(40) } )
    {
        V v;
        v.reserve( reserve_front, reserve_back );
        v.assign( start, A );
        REQUIRE( v.size() == start );
        for ( const auto & x : v ) REQUIRE( x == A );

        v.assign( target, B );
        REQUIRE( v.size() == target );
        for ( const auto & x : v ) REQUIRE( x == B );
    }
}

TEST_CASE( "empty and zero-length operations", "[veque::veque][empty]" )
{
    using V = veque::veque<int>;
    const V empty;

    SECTION( "copy-construct from empty" )
    {
        V b( empty );
        REQUIRE( b.empty() );
    }
    SECTION( "move-construct from empty" )
    {
        V src;
        V b( std::move(src) );
        REQUIRE( b.empty() );
    }
    SECTION( "copy-assign empty over non-empty" )
    {
        V b{ 1, 2, 3 };
        b = empty;
        REQUIRE( b.empty() );
    }
    SECTION( "assign(0, value)" )
    {
        V b{ 1, 2, 3 };
        b.assign( 0, 5 );
        REQUIRE( b.empty() );
    }
    SECTION( "insert count 0 is a no-op returning the position" )
    {
        V b{ 1, 2, 3 };
        auto it = b.insert( b.begin() + 1, 0, 9 );
        REQUIRE( b == V{ 1, 2, 3 } );
        REQUIRE( std::distance( b.begin(), it ) == 1 );
    }
    SECTION( "insert empty forward range" )
    {
        V b{ 1, 2, 3 };
        std::vector<int> src;
        auto it = b.insert( b.begin() + 1, src.begin(), src.end() );
        REQUIRE( b == V{ 1, 2, 3 } );
        REQUIRE( std::distance( b.begin(), it ) == 1 );
    }
    SECTION( "clear then reuse" )
    {
        V b{ 1, 2, 3 };
        b.clear();
        REQUIRE( b.empty() );
        b.push_back( 7 );
        b.push_front( 6 );
        REQUIRE( b == V{ 6, 7 } );
    }
    SECTION( "assign empty range over non-empty" )
    {
        V b{ 1, 2, 3 };
        std::vector<int> src;
        b.assign( src.begin(), src.end() );
        REQUIRE( b.empty() );
    }
}
