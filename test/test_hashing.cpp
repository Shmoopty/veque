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
#include <tuple> 
#include "catch.hpp"
#include "test_types.h"

using MyTypes = std::tuple<
        StdVeque<int,veque::fast_resize_traits>, StdVeque<int,veque::std_vector_traits>, StdVeque<int,veque::no_reserve_traits>, StdVeque<int,front_vector_traits>, 
        StdVeque<std::string,veque::fast_resize_traits>, StdVeque<std::string,veque::std_vector_traits>, StdVeque<std::string,veque::no_reserve_traits>, StdVeque<std::string,front_vector_traits>, 
        StdVeque<double,veque::fast_resize_traits>, StdVeque<double,veque::std_vector_traits>, StdVeque<double,veque::no_reserve_traits>, StdVeque<double,front_vector_traits>,
        GrumpyVeque<int,veque::fast_resize_traits>, GrumpyVeque<int,veque::std_vector_traits>, GrumpyVeque<int,veque::no_reserve_traits>, GrumpyVeque<int,front_vector_traits>, 
        GrumpyVeque<std::string,veque::fast_resize_traits>, GrumpyVeque<std::string,veque::std_vector_traits>, GrumpyVeque<std::string,veque::no_reserve_traits>, GrumpyVeque<std::string,front_vector_traits>, 
        GrumpyVeque<double,veque::fast_resize_traits>, GrumpyVeque<double,veque::std_vector_traits>, GrumpyVeque<double,veque::no_reserve_traits>, GrumpyVeque<double,front_vector_traits>,
        PropogatingGrumpyVeque<int,veque::fast_resize_traits>, PropogatingGrumpyVeque<int,veque::std_vector_traits>, PropogatingGrumpyVeque<int,veque::no_reserve_traits>, PropogatingGrumpyVeque<int,front_vector_traits>, 
        PropogatingGrumpyVeque<std::string,veque::fast_resize_traits>, PropogatingGrumpyVeque<std::string,veque::std_vector_traits>, PropogatingGrumpyVeque<std::string,veque::no_reserve_traits>, PropogatingGrumpyVeque<std::string,front_vector_traits>, 
        PropogatingGrumpyVeque<double,veque::fast_resize_traits>, PropogatingGrumpyVeque<double,veque::std_vector_traits>, PropogatingGrumpyVeque<double,veque::no_reserve_traits>, PropogatingGrumpyVeque<double,front_vector_traits>,
        AllocCountingVeque<int,veque::fast_resize_traits>, AllocCountingVeque<int,veque::std_vector_traits>, AllocCountingVeque<int,veque::no_reserve_traits>, AllocCountingVeque<int,front_vector_traits>, 
        AllocCountingVeque<std::string,veque::fast_resize_traits>, AllocCountingVeque<std::string,veque::std_vector_traits>, AllocCountingVeque<std::string,veque::no_reserve_traits>, AllocCountingVeque<std::string,front_vector_traits>, 
        AllocCountingVeque<double,veque::fast_resize_traits>, AllocCountingVeque<double,veque::std_vector_traits>, AllocCountingVeque<double,veque::no_reserve_traits>, AllocCountingVeque<double,front_vector_traits>
        >;

TEMPLATE_LIST_TEST_CASE( "hashing", "[veque::veque][template]", MyTypes )
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
