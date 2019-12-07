/* 
 * 
 * veque::veque test suite.
 * 
 * Additionally, valgrind claims there is no bad behavior throughout this usage.
 *
 *  Copyright (C) 2019 Drew Dormann
 * 
 */

#include <cstdint> 
#include <string> 
#include <unordered_set> 
#include <string> 
#include <functional> 
#include "catch.hpp"
#include "test_types.h"

TEMPLATE_PRODUCT_TEST_CASE( "large end growth", "[veque::veque][template]", (StdVeque, GrumpyVeque, PropogatingGrumpyVeque, AllocCountingVeque), (bool, int, std::string, LargeTrivialObject, NonTrivialObject, ThrowingMoveConstructObject, ThrowingMoveAssignObject, ThrowingMoveObject ) )
{
    typename TestType::size_type size = 5;
    TestType v( size );

    REQUIRE( v.size() == size );
    REQUIRE( v.capacity() >= size );
    
    SECTION( "push_back" )
    {
        typename TestType::value_type val{};
        for ( int i = 0; i < 2'000; ++i )
        {
            v.push_back( val );
            ++size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        while ( v.size() )
        {
            v.pop_back();
            --size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        REQUIRE( 0 == size );
        REQUIRE( v.empty() );
    }
    
    SECTION( "emplace_back" )
    {
        for ( int i = 0; i < 2'000; ++i )
        {
            v.emplace_back();
            ++size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        while ( v.size() )
        {
            v.pop_back();
            --size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        REQUIRE( 0 == size );
        REQUIRE( v.empty() );
    }
    SECTION( "push_front" )
    {
        typename TestType::value_type val{};
        for ( int i = 0; i < 2'000; ++i )
        {
            v.push_front( val );
            ++size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        while ( v.size() )
        {
            v.pop_back();
            --size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        REQUIRE( 0 == size );
        REQUIRE( v.empty() );
    }
    SECTION( "emplace_front" )
    {
        for ( int i = 0; i < 2'000; ++i )
        {
            v.emplace_front();
            ++size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        while ( v.size() )
        {
            v.pop_back();
            --size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        REQUIRE( 0 == size );
        REQUIRE( v.empty() );
    }
}
