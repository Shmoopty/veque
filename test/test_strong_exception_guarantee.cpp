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

TEMPLATE_PRODUCT_TEST_CASE( "veque::veques can be modified at either end with strong exception guarantee", "[veque::veque][template]", (StdVeque, GrumpyVeque, PropogatingGrumpyVeque, AllocCountingVeque), (bool, int, std::string, LargeTrivialObject, NonTrivialObject, ThrowingMoveConstructObject, ThrowingMoveAssignObject, ThrowingMoveObject ) )
{
    TestType v( 5 );

    REQUIRE( v.size() == 5 );
    REQUIRE( v.capacity() >= 5 );

    SECTION( "push_rvalue_back" )
    {
        auto size = v.size();
        for ( auto i = 0; i != 100; ++ i )
        {
            auto v_before = v;
            try
            {
                v.push_back( {} );
                REQUIRE( ++size == v.size() );
            }
            catch(...)
            {
                REQUIRE( size == v.size() );
                REQUIRE( v == v_before );
            }
        }
    }
    SECTION( "push_lvalue_back" )
    {
        auto size = v.size();
        for ( auto i = 0; i != 100; ++ i )
        {
            auto v_before = v;
            try
            {
                typename TestType::value_type val{};
                v.push_back( val );
                REQUIRE( ++size == v.size() );
            }
            catch(...)
            {
                REQUIRE( size == v.size() );
                REQUIRE( v == v_before );
            }
        }
    }
    SECTION( "emplace_back" )
    {
        auto size = v.size();
        for ( auto i = 0; i != 100; ++ i )
        {
            auto v_before = v;
            try
            {
                v.emplace_back();
                REQUIRE( ++size == v.size() );
            }
            catch(...)
            {
                REQUIRE( size == v.size() );
                REQUIRE( v == v_before );
            }
        }
    }
    SECTION( "push_lvalue_front" )
    {
        auto size = v.size();
        for ( auto i = 0; i != 100; ++ i )
        {
            auto v_before = v;
            try
            {
                typename TestType::value_type val{};
                v.push_front( val );
                REQUIRE( ++size == v.size() );
            }
            catch(...)
            {
                REQUIRE( size == v.size() );
                REQUIRE( v == v_before );
            }
        }
    }
    SECTION( "push_rvalue_front" )
    {
        auto size = v.size();
        for ( auto i = 0; i != 100; ++ i )
        {
            auto v_before = v;
            try
            {
                v.push_front( {} );
                REQUIRE( ++size == v.size() );
            }
            catch(...)
            {
                REQUIRE( size == v.size() );
                REQUIRE( v == v_before );
            }
        }
    }
    SECTION( "emplace_front" )
    {
        auto size = v.size();
        for ( auto i = 0; i != 100; ++ i )
        {
            auto v_before = v;
            try
            {
                v.emplace_front();
                REQUIRE( ++size == v.size() );
            }
            catch(...)
            {
                REQUIRE( size == v.size() );
                REQUIRE( v == v_before );
            }
        }
    }
}

