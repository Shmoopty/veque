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

TEMPLATE_PRODUCT_TEST_CASE( "veque::veques can be modified at either end with strong exception guarantee", "[veque::veque][template]", (StdVeque, GrumpyVeque, PropogatingGrumpyVeque, AllocCountingVeque),  (
        (int,veque::fast_resize_traits), (int,veque::std_vector_traits), (int,veque::no_reserve_traits), (int,front_vector_traits), 
        (std::string,veque::fast_resize_traits), (std::string,veque::std_vector_traits), (std::string,veque::no_reserve_traits), (std::string,front_vector_traits), 
        (double,veque::fast_resize_traits), (double,veque::std_vector_traits), (double,veque::no_reserve_traits), (double,front_vector_traits), 
        (LargeTrivialObject,veque::fast_resize_traits), (LargeTrivialObject,veque::std_vector_traits), (LargeTrivialObject,veque::no_reserve_traits), (LargeTrivialObject,front_vector_traits), 
        (NonTrivialObject,veque::fast_resize_traits), (NonTrivialObject,veque::std_vector_traits), (NonTrivialObject,veque::no_reserve_traits), (NonTrivialObject,front_vector_traits), 
        (ThrowingMoveConstructObject,veque::fast_resize_traits), (ThrowingMoveConstructObject,veque::std_vector_traits), (ThrowingMoveConstructObject,veque::no_reserve_traits), (ThrowingMoveConstructObject,front_vector_traits), 
        (ThrowingMoveAssignObject,veque::fast_resize_traits), (ThrowingMoveAssignObject,veque::std_vector_traits), (ThrowingMoveAssignObject,veque::no_reserve_traits), (ThrowingMoveAssignObject,front_vector_traits), 
        (ThrowingMoveObject,veque::fast_resize_traits), (ThrowingMoveObject,veque::std_vector_traits), (ThrowingMoveObject,veque::no_reserve_traits), (ThrowingMoveObject,front_vector_traits)
        ) )
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

