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

TEMPLATE_PRODUCT_TEST_CASE( "veque::veque element ordering and access", "[veque::veque][template]", (StdVeque, GrumpyVeque, PropogatingGrumpyVeque, AllocCountingVeque), (int, std::string, double, std::vector<int> ) )
{
    TestType veq1;
    
    CHECK( veq1.empty() );
    CHECK( veq1.size() == 0 );

    veq1.push_back( val<typename TestType::value_type,1> );
    veq1.push_back( val<typename TestType::value_type,2> );
    veq1.emplace_back( val<typename TestType::value_type,3> );

    CHECK( !veq1.empty() );
    CHECK( veq1.size() == 3 );
    CHECK( veq1 == TestType{ val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );
    CHECK( veq1.front() == val<typename TestType::value_type,1> );
    CHECK( veq1.back() == val<typename TestType::value_type,3> );

    veq1.push_front( val<typename TestType::value_type,4> );
    veq1.emplace_front( val<typename TestType::value_type,5> );

    CHECK( !veq1.empty() );
    CHECK( veq1.size() == 5 );
    CHECK( veq1 == TestType{ val<typename TestType::value_type,5>, val<typename TestType::value_type,4>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );
    CHECK( veq1.front() == val<typename TestType::value_type,5> );
    CHECK( veq1.back() == val<typename TestType::value_type,3> );
    
    veq1.pop_back();

    CHECK( !veq1.empty() );
    CHECK( veq1.size() == 4 );
    CHECK( veq1 == TestType{ val<typename TestType::value_type,5>, val<typename TestType::value_type,4>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2> } );
    CHECK( veq1.front() == val<typename TestType::value_type,5> );
    CHECK( veq1.back() == val<typename TestType::value_type,2> );
    CHECK( veq1[0] == val<typename TestType::value_type,5> );
    CHECK( veq1.at(0) == val<typename TestType::value_type,5> );
    CHECK( veq1[1] == val<typename TestType::value_type,4> );
    CHECK( veq1.at(1) == val<typename TestType::value_type,4> );
    CHECK( veq1[2] == val<typename TestType::value_type,1> );
    CHECK( veq1.at(2) == val<typename TestType::value_type,1> );
    CHECK( veq1[3] == val<typename TestType::value_type,2> );
    CHECK( veq1.at(3) == val<typename TestType::value_type,2> );
    CHECK_THROWS( veq1.at(4) );

    auto it = veq1.begin();
    CHECK( it != veq1.end() );
    CHECK( *it == val<typename TestType::value_type,5> );
    ++it;
    CHECK( it != veq1.end() );
    CHECK( *it == val<typename TestType::value_type,4> );
    ++it;
    CHECK( it != veq1.end() );
    CHECK( *it == val<typename TestType::value_type,1> );
    ++it;
    CHECK( it != veq1.end() );
    CHECK( *it == val<typename TestType::value_type,2> );
    ++it;
    CHECK( it == veq1.end() );
    
    auto c_it = veq1.cbegin();
    CHECK( c_it != veq1.cend() );
    CHECK( *c_it == val<typename TestType::value_type,5> );
    ++c_it;
    CHECK( c_it != veq1.cend() );
    CHECK( *c_it == val<typename TestType::value_type,4> );
    ++c_it;
    CHECK( c_it != veq1.cend() );
    CHECK( *c_it == val<typename TestType::value_type,1> );
    ++c_it;
    CHECK( c_it != veq1.cend() );
    CHECK( *c_it == val<typename TestType::value_type,2> );
    ++c_it;
    CHECK( c_it == veq1.cend() );


    auto r_it = veq1.rbegin();
    CHECK( r_it != veq1.rend() );
    CHECK( *r_it == val<typename TestType::value_type,2> );
    ++r_it;
    CHECK( r_it != veq1.rend() );
    CHECK( *r_it == val<typename TestType::value_type,1> );
    ++r_it;
    CHECK( r_it != veq1.rend() );
    CHECK( *r_it == val<typename TestType::value_type,4> );
    ++r_it;
    CHECK( r_it != veq1.rend() );
    CHECK( *r_it == val<typename TestType::value_type,5> );
    ++r_it;
    CHECK( r_it == veq1.rend() );
    
    auto cr_it = veq1.crbegin();
    CHECK( cr_it != veq1.crend() );
    CHECK( *cr_it == val<typename TestType::value_type,2> );
    ++cr_it;
    CHECK( cr_it != veq1.crend() );
    CHECK( *cr_it == val<typename TestType::value_type,1> );
    ++cr_it;
    CHECK( cr_it != veq1.crend() );
    CHECK( *cr_it == val<typename TestType::value_type,4> );
    ++cr_it;
    CHECK( cr_it != veq1.crend() );
    CHECK( *cr_it == val<typename TestType::value_type,5> );
    ++cr_it;
    CHECK( cr_it == veq1.crend() );
    
    veq1.pop_front();

    CHECK( !veq1.empty() );
    CHECK( veq1.size() == 3 );
    CHECK( veq1 == TestType{ val<typename TestType::value_type,4>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2> } );
    CHECK( veq1.front() == val<typename TestType::value_type,4> );
    CHECK( veq1.back() == val<typename TestType::value_type,2> );
   
    auto veq2 = veq1;

    CHECK( veq1 == veq2 );
    CHECK( !(veq1 != veq2) );

    veq2.emplace_front( val<typename TestType::value_type,0> );
    
    CHECK( veq2 < veq1 );
    CHECK( veq2 <= veq1 );
    CHECK( veq2 != veq1 );
    CHECK( veq1 > veq2 );
    CHECK( veq1 >= veq2 );
    CHECK( veq1 != veq2 );
    CHECK( !(veq1 < veq2) );
    CHECK( !(veq1 <= veq2) );
    CHECK( !(veq2 > veq1) );
    CHECK( !(veq2 >= veq1) );

    veq2.swap( veq1 );

    CHECK( veq1 < veq2 );
    CHECK( veq1 <= veq2 );
    CHECK( veq1 != veq2 );
    CHECK( veq2 > veq1 );
    CHECK( veq2 >= veq1 );
    CHECK( veq2 != veq1 );
    CHECK( !(veq2 < veq1) );
    CHECK( !(veq2 <= veq1) );
    CHECK( !(veq1 > veq2) );
    CHECK( !(veq1 >= veq2) );

    veq1 = veq2;

    CHECK( veq1 == veq2 );
    CHECK( !(veq1 != veq2) );

    veq2 = TestType{ val<typename TestType::value_type,4>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2> };

    CHECK( veq1 == veq2 );
    CHECK( !(veq1 != veq2) );
    
    if constexpr( is_using_counting_allocator<TestType> )
    {
        REQUIRE( veq1.get_allocator().counter == 6 );
    }
}
