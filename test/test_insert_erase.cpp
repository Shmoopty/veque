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

TEMPLATE_PRODUCT_TEST_CASE( "insert/erase", "[veque::veque][template]", (StdVeque, GrumpyVeque, PropogatingGrumpyVeque, AllocCountingVeque), (int, std::string, double, std::vector<int> ) )
{
    TestType veq{ val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>  };
    veq.reserve(20);
    
    CHECK( !veq.empty() );
    CHECK( veq.size() == 3 );
    
    SECTION( "l-value insertion" )
    {
        veq.insert( veq.begin(), val<typename TestType::value_type,0> );

        REQUIRE( veq.size() == 4 );
        CHECK( veq[0] == val<typename TestType::value_type,0> );
        CHECK( veq[1] == val<typename TestType::value_type,1> );
        CHECK( veq[2] == val<typename TestType::value_type,2> );
        CHECK( veq[3] == val<typename TestType::value_type,3> );
        CHECK( veq == TestType{ val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );

        veq.insert( veq.end(), val<typename TestType::value_type,0> );

        CHECK( veq.size() == 5 );
        CHECK( veq == TestType{ val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,0> } );
    }
    SECTION( "l-value resizing insertion" )
    {
        veq.shrink_to_fit();
        veq.insert( veq.begin(), val<typename TestType::value_type,0> );

        REQUIRE( veq.size() == 4 );
        CHECK( veq[0] == val<typename TestType::value_type,0> );
        CHECK( veq[1] == val<typename TestType::value_type,1> );
        CHECK( veq[2] == val<typename TestType::value_type,2> );
        CHECK( veq[3] == val<typename TestType::value_type,3> );
        CHECK( veq == TestType{ val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );

        veq.shrink_to_fit();
        veq.insert( veq.end(), val<typename TestType::value_type,0> );

        REQUIRE( veq.size() == 5 );
        CHECK( veq[0] == val<typename TestType::value_type,0> );
        CHECK( veq[1] == val<typename TestType::value_type,1> );
        CHECK( veq[2] == val<typename TestType::value_type,2> );
        CHECK( veq[3] == val<typename TestType::value_type,3> );
        CHECK( veq[4] == val<typename TestType::value_type,0> );
        CHECK( veq == TestType{ val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,0> } );
    }
    SECTION( "r-value insertion" )
    {
        veq.insert( veq.begin(), typename TestType::value_type(val<typename TestType::value_type,0>) );

        CHECK( veq.size() == 4 );
        CHECK( veq == TestType{ val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );

        veq.insert( veq.end(), typename TestType::value_type(val<typename TestType::value_type,0>) );

        CHECK( veq.size() == 5 );
        CHECK( veq == TestType{ val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,0> } );
    }
    SECTION( "r-value resizing insertion" )
    {
        veq.shrink_to_fit();
        veq.insert( veq.begin(), typename TestType::value_type(val<typename TestType::value_type,0>) );

        CHECK( veq.size() == 4 );
        CHECK( veq == TestType{ val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );

        veq.shrink_to_fit();
        veq.insert( veq.end(), typename TestType::value_type(val<typename TestType::value_type,0>) );

        CHECK( veq.size() == 5 );
        CHECK( veq == TestType{ val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,0> } );
    }
    SECTION( "pop erasure" )
    {
        // pop erasure
        CHECK( veq.pop_front_element() == val<typename TestType::value_type,1> );
        
        CHECK( veq.size() == 2 );
        CHECK( veq == TestType{ val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );

        CHECK( veq.pop_back_element() == val<typename TestType::value_type,3> );

        CHECK( veq.size() == 1 );
        CHECK( veq == TestType{ val<typename TestType::value_type,2> } );
    }
    SECTION( "val,count insertion" )
    {
        veq.insert( veq.end(), 2, val<typename TestType::value_type,4> );

        CHECK( veq.size() == 5 );
        CHECK( veq == TestType{ val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,4>, val<typename TestType::value_type,4> } );

    }
    SECTION( "val,count resizing insertion" )
    {
        veq.shrink_to_fit();
        veq.insert( veq.end(), 2, val<typename TestType::value_type,4> );

        CHECK( veq.size() == 5 );
        CHECK( veq == TestType{ val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,4>, val<typename TestType::value_type,4> } );

    }
    SECTION( "range insertion" )
    {
        auto veq2 = TestType{ val<typename TestType::value_type,5>, val<typename TestType::value_type,4>, val<typename TestType::value_type,3> };
        veq.insert( veq.begin(), veq2.begin(), veq2.end() );

        CHECK( veq.size() == 6 );
        CHECK( veq == TestType{ val<typename TestType::value_type,5>, val<typename TestType::value_type,4>, val<typename TestType::value_type,3>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );
    }
    SECTION( "range resizing insertion" )
    {
        veq.shrink_to_fit();
        auto veq2 = TestType{ val<typename TestType::value_type,5>, val<typename TestType::value_type,4>, val<typename TestType::value_type,3> };
        veq.insert( veq.begin(), veq2.begin(), veq2.end() );

        CHECK( veq.size() == 6 );
        CHECK( veq == TestType{ val<typename TestType::value_type,5>, val<typename TestType::value_type,4>, val<typename TestType::value_type,3>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );
    }
    SECTION( "resize_back growth 1" )
    {
        veq.resize_back( 4 );

        CHECK( veq.size() == 4 );
        CHECK( veq == TestType{ val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, typename TestType::value_type{} } );
    }
    SECTION( "resize_back growth 2" )
    {
        veq.resize_back( 4, val<typename TestType::value_type,5> );

        CHECK( veq.size() == 4 );
        CHECK( veq == TestType{ val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,5> } );
    }
    SECTION( "resize_back erasure" )
    {
        // resize erasure
        veq.resize_back( 1 );

        CHECK( veq.size() == 1 );
        CHECK( veq == TestType{ val<typename TestType::value_type,1> } );
    }
    SECTION( "resize_front growth 1" )
    {
        veq.resize_front( 4 );

        CHECK( veq.size() == 4 );
        CHECK( veq == TestType{ typename TestType::value_type{}, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );
    }
    SECTION( "resize_front growth 2" )
    {
        veq.resize_front( 4, val<typename TestType::value_type,5> );

        CHECK( veq.size() == 4 );
        CHECK( veq == TestType{ val<typename TestType::value_type,5>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );
    }
    SECTION( "resize_front erasure" )
    {
        // resize erasure
        veq.resize_front( 1 );

        CHECK( veq.size() == 1 );
        CHECK( veq == TestType{ val<typename TestType::value_type,3> } );
    }
    SECTION( "initializer list insertion" )
    {
        // initializer list insertion
        veq.insert( veq.end(), {val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>} );

        CHECK( veq.size() == 6 );
        CHECK( veq == TestType{ val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2> } );
    }
    SECTION( "iterator erasure 1" )
    {
        veq.erase( veq.begin() );

        CHECK( veq.size() == 2 );
        CHECK( veq[0] == val<typename TestType::value_type,2> );
        CHECK( veq[1] == val<typename TestType::value_type,3> );
        CHECK( veq == TestType{ val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );
    }
    SECTION( "iterator erasure 2" )
    {
        veq.erase( veq.begin() + 1 );

        CHECK( veq.size() == 2 );
        CHECK( veq[0] == val<typename TestType::value_type,1> );
        CHECK( veq[1] == val<typename TestType::value_type,3> );
        CHECK( veq == TestType{ val<typename TestType::value_type,1>, val<typename TestType::value_type,3> } );
    }
    SECTION( "iterator erasure 3" )
    {
        veq.erase( veq.end() - 1 );

        CHECK( veq.size() == 2 );
        CHECK( veq[0] == val<typename TestType::value_type,1> );
        CHECK( veq[1] == val<typename TestType::value_type,2> );
        CHECK( veq == TestType{ val<typename TestType::value_type,1>, val<typename TestType::value_type,2> } );
    }
    SECTION( "range erasure begin" )
    {
        veq.assign( { val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,4>, val<typename TestType::value_type,5>} );
        veq.erase( veq.begin(), veq.begin() + 3 );
        CHECK( veq.size() == 3 );
        CHECK( veq == TestType{ val<typename TestType::value_type,3>, val<typename TestType::value_type,4>, val<typename TestType::value_type,5> } );
    }
    SECTION( "range erasure end" )
    {
        veq.assign( { val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,4>, val<typename TestType::value_type,5>} );
        veq.erase( veq.begin() + 3, veq.end() );
        CHECK( veq.size() == 3 );
        CHECK( veq == TestType{ val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2> } );
    }
    SECTION( "range erasure mid near front" )
    {
        veq.assign( { val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,4>, val<typename TestType::value_type,5>} );
        veq.erase( veq.begin() + 1, veq.begin() + 4 );
        CHECK( veq.size() == 3 );
        CHECK( veq == TestType{ val<typename TestType::value_type,0>, val<typename TestType::value_type,4>, val<typename TestType::value_type,5> } );
    }
    SECTION( "range erasure mid near back" )
    {
        veq.assign( { val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3>, val<typename TestType::value_type,4>, val<typename TestType::value_type,5>} );
        veq.erase( veq.begin() + 2, veq.begin() + 5 );
        CHECK( veq.size() == 3 );
        CHECK( veq == TestType{ val<typename TestType::value_type,0>, val<typename TestType::value_type,1>, val<typename TestType::value_type,5> } );
    }
    SECTION( "Range assign" )
    {
        TestType veq3;
        veq3.assign( veq.begin(), veq.end() );
        CHECK( veq3.size() == 3 );
        CHECK( veq3 == TestType{ val<typename TestType::value_type,1>, val<typename TestType::value_type,2>, val<typename TestType::value_type,3> } );
    }
    SECTION( "count,val assign" )
    {
        TestType veq4;
        veq4.assign( 3, val<typename TestType::value_type,2> );
        CHECK( veq4.size() == 3 );
        CHECK( veq4 == TestType{ val<typename TestType::value_type,2>, val<typename TestType::value_type,2>, val<typename TestType::value_type,2> } );
    }
}
