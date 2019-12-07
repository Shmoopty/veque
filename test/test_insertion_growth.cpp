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

TEMPLATE_PRODUCT_TEST_CASE( "large insertion growth", "[veque::veque][template]", (StdVeque, GrumpyVeque, PropogatingGrumpyVeque, AllocCountingVeque), (bool, int, std::string, double, LargeTrivialObject, NonTrivialObject ) )
{
    typename TestType::size_type size = 5;
    TestType v( size );

    REQUIRE( v.size() == size );
    REQUIRE( v.capacity() >= size );

    SECTION( "insert begin" )
    {
        for ( int i = 0; i < 2'000; ++i )
        {
            typename TestType::value_type val{};
            v.insert( v.begin(), val );
            ++size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        if constexpr( is_using_counting_allocator<TestType> )
        {
            REQUIRE( v.get_allocator().counter == 2'005 );
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
        if constexpr( is_using_counting_allocator<TestType> )
        {
            REQUIRE( v.get_allocator().counter == 0 );
        }
    }
    SECTION( "insert end" )
    {
        for ( int i = 0; i < 2'000; ++i )
        {
            typename TestType::value_type val{};
            v.insert( v.end(), val );
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
    SECTION( "insert middle" )
    {
        for ( int i = 0; i < 2'000; ++i )
        {
            typename TestType::value_type val{};
            v.insert( v.begin() + v.size() / 2, val );
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
    SECTION( "insert near begin" )
    {
        for ( int i = 0; i < 2'000; ++i )
        {
            typename TestType::value_type val{};
            v.insert( v.begin() + v.size() / 3, val );
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
    SECTION( "insert near end" )
    {
        for ( int i = 0; i < 2'000; ++i )
        {
            typename TestType::value_type val{};
            v.insert( v.begin() + 2 * v.size() / 3, val );
            ++size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        if constexpr( is_using_counting_allocator<TestType> )
        {
            REQUIRE( v.get_allocator().counter == 2'005 );
        }
        while ( v.size() )
        {
            v.pop_front();
            --size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        REQUIRE( 0 == size );
        REQUIRE( v.empty() );
        if constexpr( is_using_counting_allocator<TestType> )
        {
            REQUIRE( v.get_allocator().counter == 0 );
        }
    }
    SECTION( "insert randomly" )
    {
        // Valgrind doesn't like std::random_device.
        //std::random_device rd;
        //std::mt19937 gen(rd());
        srand(time(NULL));
        for ( int i = 0; i < 2'000; ++i )
        {
            typename TestType::value_type val{};
            //auto index = std::uniform_int_distribution<>(0, v.size())(gen);    
            auto index = rand() % (v.size()+1);
            //v.insert( v.begin() + dis(gen), val );
            v.insert( v.begin() + index, val );
            ++size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        if constexpr( is_using_counting_allocator<TestType> )
        {
            REQUIRE( v.get_allocator().counter == 2'005 );
        }
        while ( v.size() )
        {
            v.pop_front();
            --size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
        }
        REQUIRE( 0 == size );
        REQUIRE( v.empty() );
    }
}
