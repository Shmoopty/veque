/*
 *
 * veque::veque test suite.
 *
 * Exercises operations between veques with differing ResizeTraits, the
 * mismatched-stateful-allocator branches of the extended-move constructor,
 * copy/move assignment and swap, and a few error / capacity edge paths.
 *
 *  Copyright (C) 2019 Drew Dormann
 *
 */

#include <algorithm>
#include <stdexcept>
#include "catch.hpp"
#include "test_types.h"


TEST_CASE( "operations between differing ResizeTraits", "[veque::veque][traits]" )
{
    using Fast = veque::veque<int, veque::fast_resize_traits>;
    using Vec  = veque::veque<int, veque::std_vector_traits>;
    using NoR  = veque::veque<int, veque::no_reserve_traits>;

    const Fast a{ 1, 2, 3, 4, 5 };

    SECTION( "copy construct across traits" )
    {
        Vec b( a );
        REQUIRE( b == a );
        REQUIRE( b == Vec{ 1, 2, 3, 4, 5 } );
    }
    SECTION( "move construct across traits" )
    {
        Fast src{ 1, 2, 3, 4, 5 };
        NoR b( std::move(src) );
        REQUIRE( b == a );
    }
    SECTION( "copy assign across traits" )
    {
        Vec b{ 9, 9 };
        b = a;
        REQUIRE( b == a );
    }
    SECTION( "move assign across traits" )
    {
        Fast src{ 1, 2, 3, 4, 5 };
        Vec b{ 9, 9, 9, 9, 9, 9, 9 };
        b = std::move(src);
        REQUIRE( b == a );
    }
    SECTION( "swap across traits" )
    {
        Fast x{ 1, 2, 3 };
        Vec y{ 4, 5, 6, 7 };
        x.swap( y );
        REQUIRE( x == Fast{ 4, 5, 6, 7 } );
        REQUIRE( y == Vec{ 1, 2, 3 } );
    }
    SECTION( "equality and ordering across traits" )
    {
        Vec equal{ 1, 2, 3, 4, 5 };
        Vec less{ 1, 2, 3 };
        REQUIRE( a == equal );
        REQUIRE_FALSE( a != equal );
        REQUIRE( less < a );
        REQUIRE( a > less );
        REQUIRE( less <= a );
        REQUIRE( a >= equal );
    }
}

TEST_CASE( "mismatched stateful allocator paths", "[veque::veque][allocator]" )
{
    using V = veque::veque<int, veque::fast_resize_traits, StatefulAllocator<int>>;
    StatefulAllocator<int> a1;
    a1.barrier = 0x1111;
    StatefulAllocator<int> a2;
    a2.barrier = 0x2222;
    REQUIRE( a1 != a2 );

    SECTION( "extended-move ctor with unequal allocator reallocates" )
    {
        V x( { 1, 2, 3, 4, 5 }, a1 );
        V y( std::move(x), a2 );
        REQUIRE( y == veque::veque<int>{ 1, 2, 3, 4, 5 } );
        REQUIRE( y.get_allocator() == a2 );
    }
    SECTION( "move-assign, unequal non-propagating, reuses existing storage" )
    {
        V x( { 1, 2, 3 }, a1 );
        V y( { 7, 7, 7, 7, 7, 7, 7, 7 }, a2 ); // ample capacity for 3
        y = std::move(x);
        REQUIRE( y == veque::veque<int>{ 1, 2, 3 } );
        REQUIRE( y.get_allocator() == a2 );
    }
    SECTION( "move-assign, unequal non-propagating, needs reallocation" )
    {
        V x( { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, a1 );
        V y( { 7 }, a2 );
        y = std::move(x);
        REQUIRE( y == veque::veque<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } );
        REQUIRE( y.get_allocator() == a2 );
    }
    SECTION( "copy-assign, unequal non-propagating" )
    {
        V x( { 1, 2, 3, 4, 5 }, a1 );
        V y( { 9, 9 }, a2 );
        y = x;
        REQUIRE( y == veque::veque<int>{ 1, 2, 3, 4, 5 } );
        REQUIRE( y.get_allocator() == a2 );
    }
    SECTION( "swap, unequal non-propagating, allocates compatible storage" )
    {
        V x( { 1, 2, 3 }, a1 );
        V y( { 4, 5, 6, 7 }, a2 );
        x.swap( y );
        REQUIRE( x == veque::veque<int>{ 4, 5, 6, 7 } );
        REQUIRE( y == veque::veque<int>{ 1, 2, 3 } );
        REQUIRE( x.get_allocator() == a1 );
        REQUIRE( y.get_allocator() == a2 );
    }
}

TEST_CASE( "propagating stateful allocator across traits", "[veque::veque][allocator][traits]" )
{
    using Fast = veque::veque<int, veque::fast_resize_traits, PropagatingStatefulAllocator<int>>;
    using Vec  = veque::veque<int, veque::std_vector_traits, PropagatingStatefulAllocator<int>>;
    PropagatingStatefulAllocator<int> a1;
    a1.barrier = 0x3333;
    PropagatingStatefulAllocator<int> a2;
    a2.barrier = 0x4444;

    SECTION( "move assign propagates allocator" )
    {
        Fast src( { 1, 2, 3, 4 }, a1 );
        Vec dst( { 9 }, a2 );
        dst = std::move(src);
        REQUIRE( dst == veque::veque<int>{ 1, 2, 3, 4 } );
        REQUIRE( dst.get_allocator() == a1 );
    }
    SECTION( "swap propagates allocators" )
    {
        Fast x( { 1, 2, 3 }, a1 );
        Fast y( { 4, 5 }, a2 );
        x.swap( y );
        REQUIRE( x == veque::veque<int>{ 4, 5 } );
        REQUIRE( y == veque::veque<int>{ 1, 2, 3 } );
        REQUIRE( x.get_allocator() == a2 );
        REQUIRE( y.get_allocator() == a1 );
    }
}

TEST_CASE( "capacity and error edge paths", "[veque::veque][capacity]" )
{
    SECTION( "shrink_to_fit releases spare capacity" )
    {
        veque::veque<int> v;
        v.reserve( 50, 50 );
        v.assign( 3, 7 );
        REQUIRE( v.capacity_full() >= 3 );
        v.shrink_to_fit();
        REQUIRE( v.capacity_full() == 3 );
        REQUIRE( v == veque::veque<int>{ 7, 7, 7 } );
    }
    SECTION( "reserve beyond max_size throws length_error" )
    {
        veque::veque<int> v{ 1, 2, 3 };
        REQUIRE_THROWS_AS( v.reserve( v.max_size(), v.max_size() ), std::length_error );
    }
    SECTION( "at() out of range throws" )
    {
        veque::veque<int> v{ 1, 2, 3 };
        REQUIRE_THROWS_AS( v.at( 3 ), std::out_of_range );
        REQUIRE_NOTHROW( v.at( 2 ) );
        const auto & cv = v;
        REQUIRE_THROWS_AS( cv.at( 99 ), std::out_of_range );
    }
}
