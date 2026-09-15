/* 
 * 
 * veque::veque test suite.
 * 
 * Additionally, valgrind claims there is no bad behavior throughout this usage.
 *
 *  Copyright (C) 2019-2026 Drew Dormann
 * 
 */

#include <ranges> 
#include <string> 
#include "catch.hpp"
#include "test_types.h"

// Each byte-wise fast path -- the memset, the memcpy, and both memmoves -- has
// an element-wise fallback for constant evaluation.  Each of these routes
// through a different one, and a regression stops the build rather than
// failing a test.
namespace
{
    constexpr std::size_t grown()
    {
        veque::veque<int> v;
        for ( int i = 0; i < 40; ++i )
        {
            v.push_back( i );
        }
        for ( int i = 0; i < 10; ++i )
        {
            v.push_front( -1 - i );
        }
        return v.size();
    }
    static_assert( grown() == 50 );

    // insert and erase shift elements, which is the memmove path at run time.
    constexpr int shifted()
    {
        veque::veque<int> v{ 1, 2, 7, 8 };
        v.insert( v.begin() + 2, 5 );
        v.erase( v.begin() );
        return v[0] * 100 + v[1] * 10 + v[2];
    }
    static_assert( shifted() == 257 );

    // assign(count, value) is the memset/fill path.
    constexpr std::size_t assigned()
    {
        veque::veque<int> v;
        v.reserve( 32 );
        v.assign( std::size_t{8}, 3 );
        v.clear();
        v.append_range( std::views::iota( 1, 6 ) );
        return v.size();
    }
    static_assert( assigned() == 5 );

    // A non-trivial element type never takes a byte-wise path at all.
    constexpr bool non_trivial()
    {
        veque::veque<std::string> v;
        v.emplace_back( std::string( 100, 'A' ) );
        v.emplace_front( std::string( 200, 'B' ) );
        return v.size() == 2 && v[0].size() > v[1].size();
    }
    static_assert( non_trivial() );
}

TEST_CASE( "veque::veque is usable in constant expressions", "[veque::veque]" )
{
    // The static_asserts above are the test.  Running the same code here keeps
    // the byte-wise branches covered, since at run time they are the ones taken.
    REQUIRE( grown() == 50 );
    REQUIRE( shifted() == 257 );
    REQUIRE( assigned() == 5 );
    REQUIRE( non_trivial() );
}
