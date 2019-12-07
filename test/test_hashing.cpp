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

TEMPLATE_PRODUCT_TEST_CASE( "hashing", "[veque::veque][template]", (StdVeque, GrumpyVeque, PropogatingGrumpyVeque, AllocCountingVeque), (int, std::string, double ) )
{
    std::unordered_set<TestType> uset;
    
    uset.emplace( TestType{val<typename TestType::value_type,1>} );
    uset.emplace( TestType{val<typename TestType::value_type,2>} );
    uset.emplace( TestType{val<typename TestType::value_type,3>} );

    CHECK( uset.size() == 3 );
    CHECK( uset.count(TestType{val<typename TestType::value_type,0>}) == 0 );
    CHECK( uset.count(TestType{val<typename TestType::value_type,1>}) == 1 );
    CHECK( uset.count(TestType{val<typename TestType::value_type,2>}) == 1 );
    CHECK( uset.count(TestType{val<typename TestType::value_type,3>}) == 1 );
    CHECK( uset.count(TestType{val<typename TestType::value_type,4>}) == 0 );
    CHECK( uset.count(TestType{val<typename TestType::value_type,5>}) == 0 );

    uset.emplace( TestType{val<typename TestType::value_type,3>} );

    CHECK( uset.size() == 3 );
    CHECK( uset.count(TestType{val<typename TestType::value_type,0>}) == 0 );
    CHECK( uset.count(TestType{val<typename TestType::value_type,1>}) == 1 );
    CHECK( uset.count(TestType{val<typename TestType::value_type,2>}) == 1 );
    CHECK( uset.count(TestType{val<typename TestType::value_type,3>}) == 1 );
    CHECK( uset.count(TestType{val<typename TestType::value_type,4>}) == 0 );
    CHECK( uset.count({val<typename TestType::value_type,5>}) == 0 );
}
