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

TEMPLATE_PRODUCT_TEST_CASE( "reassignment", "[veque::veque][template]", (StdVeque, GrumpyVeque, PropogatingGrumpyVeque, AllocCountingVeque), (int, std::string, double, std::vector<int> ) )
{
    for ( int i = 0; i < 10'000; ++i )
    {
        auto v1 = TestType( rand() % 100, val<typename TestType::value_type,4> );
        auto v2 = TestType( rand() % 100, val<typename TestType::value_type,5> );
        auto v3 = TestType( rand() % 100, val<typename TestType::value_type,5> );
        v1 = v2;
        REQUIRE( v1 == v2 );
        v3 = std::move(v2);
        REQUIRE( v1 == v3 );
        auto v4 = TestType( v1, typename TestType::allocator_type{} );
        REQUIRE( v1 == v4 );
    }

    // Valgrind doesn't like std::random_device.
    //std::random_device rd;
    //std::mt19937 gen(rd());
    srand(time(NULL));
    for ( int i = 0; i < 10'000; ++i )
    {
        auto v1 = TestType( typename TestType::allocator_type{} );
        auto v2 = TestType( rand() % 100, val<typename TestType::value_type,5> );
        auto v3 = TestType( rand() % 100, val<typename TestType::value_type,5> );
        v1.assign( v2.begin(), v2.end() );
        REQUIRE( v1 == v2 );
        v2.assign( v3.begin(), v3.end() );
        REQUIRE( v2 == v3 );
    }
}
