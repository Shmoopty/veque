/*
 *
 * veque::veque test suite.
 *
 * Exercises the single-pass (input iterator) code paths of the range
 * constructor, assign(), and insert().  These overloads were previously
 * uncovered by the suite.
 *
 *  Copyright (C) 2019 Drew Dormann
 *
 */

#include <sstream>
#include <iterator>
#include <string>
#include "catch.hpp"
#include "test_types.h"


TEST_CASE( "input-iterator range constructor", "[veque::veque][input_iterator]" )
{
    SECTION( "non-empty ints" )
    {
        std::istringstream ss( "1 2 3 4 5" );
        std::istream_iterator<int> b( ss ), e;
        veque::veque<int> v( b, e );
        REQUIRE( v == veque::veque<int>{ 1, 2, 3, 4, 5 } );
    }
    SECTION( "empty range" )
    {
        std::istringstream ss( "" );
        std::istream_iterator<int> b( ss ), e;
        veque::veque<int> v( b, e );
        REQUIRE( v.empty() );
    }
    SECTION( "single element" )
    {
        std::istringstream ss( "42" );
        std::istream_iterator<int> b( ss ), e;
        veque::veque<int> v( b, e );
        REQUIRE( v == veque::veque<int>{ 42 } );
    }
    SECTION( "strings" )
    {
        std::istringstream ss( "alpha beta gamma" );
        std::istream_iterator<std::string> b( ss ), e;
        veque::veque<std::string> v( b, e );
        REQUIRE( v == veque::veque<std::string>{ "alpha", "beta", "gamma" } );
    }
}

TEST_CASE( "input-iterator assign", "[veque::veque][input_iterator]" )
{
    SECTION( "grow" )
    {
        veque::veque<int> v{ 9, 9 };
        std::istringstream ss( "1 2 3 4" );
        std::istream_iterator<int> b( ss ), e;
        v.assign( b, e );
        REQUIRE( v == veque::veque<int>{ 1, 2, 3, 4 } );
    }
    SECTION( "shrink" )
    {
        veque::veque<int> v{ 9, 9, 9, 9, 9 };
        std::istringstream ss( "1 2" );
        std::istream_iterator<int> b( ss ), e;
        v.assign( b, e );
        REQUIRE( v == veque::veque<int>{ 1, 2 } );
    }
    SECTION( "to empty" )
    {
        veque::veque<int> v{ 9, 9, 9 };
        std::istringstream ss( "" );
        std::istream_iterator<int> b( ss ), e;
        v.assign( b, e );
        REQUIRE( v.empty() );
    }
}

TEST_CASE( "input-iterator insert", "[veque::veque][input_iterator]" )
{
    SECTION( "middle" )
    {
        veque::veque<int> v{ 1, 2, 3 };
        std::istringstream ss( "10 20" );
        std::istream_iterator<int> b( ss ), e;
        auto it = v.insert( v.begin() + 1, b, e );
        REQUIRE( v == veque::veque<int>{ 1, 10, 20, 2, 3 } );
        REQUIRE( std::distance( v.begin(), it ) == 1 );
        REQUIRE( *it == 10 );
    }
    SECTION( "front" )
    {
        veque::veque<int> v{ 1, 2, 3 };
        std::istringstream ss( "7 8 9" );
        std::istream_iterator<int> b( ss ), e;
        auto it = v.insert( v.begin(), b, e );
        REQUIRE( v == veque::veque<int>{ 7, 8, 9, 1, 2, 3 } );
        REQUIRE( it == v.begin() );
    }
    SECTION( "back" )
    {
        veque::veque<int> v{ 1, 2, 3 };
        std::istringstream ss( "7 8" );
        std::istream_iterator<int> b( ss ), e;
        auto it = v.insert( v.end(), b, e );
        REQUIRE( v == veque::veque<int>{ 1, 2, 3, 7, 8 } );
        REQUIRE( *it == 7 );
    }
    SECTION( "empty input range is a no-op returning the position" )
    {
        veque::veque<int> v{ 1, 2, 3 };
        std::istringstream ss( "" );
        std::istream_iterator<int> b( ss ), e;
        auto it = v.insert( v.begin() + 1, b, e );
        REQUIRE( v == veque::veque<int>{ 1, 2, 3 } );
        REQUIRE( std::distance( v.begin(), it ) == 1 );
    }
}
