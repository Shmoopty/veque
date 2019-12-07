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

TEMPLATE_PRODUCT_TEST_CASE( "std::deque interface parity", "[veque::veque][template]", (StdVeque, GrumpyVeque, PropogatingGrumpyVeque, AllocCountingVeque), (int, std::string, double, std::vector<int> ) )
{
    TestType veq;
    std::deque<typename TestType::value_type, typename TestType::allocator_type> deq;

    srand(time(NULL));
    
    auto tests = std::vector<std::function<void()>>{
        [&]
        {
            INFO( "resize" );
            auto new_size = rand() % 10'000;
            veq.resize(new_size);
            deq.resize(new_size);
        },
        [&]
        {
            INFO( "at" );
            if ( veq.size() )
            {
                auto index = rand() % veq.size();
                CHECK( veq.at(index) == deq.at(index) );
            }
        },
        [&]
        {
            INFO( "const at" );
            if ( veq.size() )
            {
                const auto veq2 = veq;
                const auto deq2 = deq;
                auto index = rand() % veq.size();
                CHECK( veq2.at(index) == deq2.at(index) );
            }
        },
        [&]
        {
            INFO( "[]" );
            if ( veq.size() )
            {
                auto index = rand() % veq.size();
                CHECK( veq[index] == deq[index] );
            }
        },
        [&]
        {
            INFO( "[]" );
            if ( veq.size() )
            {
                const auto & veq2 = veq;
                const auto & deq2 = deq;
                auto index = rand() % veq.size();
                CHECK( veq2[index] == deq2[index] );
            }
        },
        [&]
        {
            INFO( "front" );
            if ( veq.size() )
            {
                CHECK( veq.front() == deq.front() );
            }
        },
        [&]
        {
            INFO( "const front" );
            if ( veq.size() )
            {
                const auto & veq2 = veq;
                const auto & deq2 = deq;
                CHECK( veq2.front() == deq2.front() );
            }
        },
        [&]
        {
            INFO( "back" );
            if ( veq.size() )
            {
                CHECK( veq.back() == deq.back() );
            }
        },
        [&]
        {
            INFO( "const back" );
            if ( veq.size() )
            {
                const auto & veq2 = veq;
                const auto & deq2 = deq;
                CHECK( veq2.back() == deq2.back() );
            }
        },
        [&]
        {
            INFO( "push_back0" );
            auto item = val<typename TestType::value_type,0>;
            veq.push_back( item );
            deq.push_back( item );
        },
        [&]
        {
            INFO( "push_back1" );
            auto item = val<typename TestType::value_type,1>;
            veq.push_back( typename TestType::value_type{item} );
            deq.push_back( typename TestType::value_type{item} );
        },
        [&]
        {
            INFO( "emplace_back" );
            auto item = val<typename TestType::value_type,4>;
            veq.emplace_back( typename TestType::value_type{item} );
            deq.emplace_back( typename TestType::value_type{item} );
        },
        [&]
        {
            INFO( "push_front5" );
            auto item = val<typename TestType::value_type,5>;
            veq.push_front( item );
            deq.push_front( item );
        },
        [&]
        {
            INFO( "push_front4" );
            auto item = val<typename TestType::value_type,4>;
            veq.push_front( typename TestType::value_type{item} );
            deq.push_front( typename TestType::value_type{item} );
        },
        [&]
        {
            INFO( "emplace_front" );
            auto item = val<typename TestType::value_type,4>;
            veq.emplace_front( typename TestType::value_type{item} );
            deq.emplace_front( typename TestType::value_type{item} );
        },
        [&]
        {
            INFO( "insert2" );
            auto item = val<typename TestType::value_type,2>;
            auto index = veq.size() ? rand() % veq.size() : 0;
            veq.insert( veq.begin() + index, item );
            deq.insert( deq.begin() + index, item );
        },
        [&]
        {
            INFO( "insert3" );
            auto item = val<typename TestType::value_type,3>;
            auto index = veq.size() ? rand() % veq.size() : 0;
            veq.insert( veq.begin() + index, typename TestType::value_type{item} );
            deq.insert( deq.begin() + index, typename TestType::value_type{item});
        },
        [&]
        {
            INFO( "emplace" );
            auto index = veq.size() ? rand() % veq.size() : 0;
            veq.emplace( veq.begin() + index );
            deq.emplace( deq.begin() + index );
        },
        [&]
        {
            INFO( "begin" );
            if ( veq.size() )
            {
                CHECK( *veq.begin() == *deq.begin() );
            }
        },
        [&]
        {
            INFO( "rbegin" );
            if ( veq.size() )
            {
                CHECK( *veq.rbegin() == *deq.rbegin() );
            }
        },
        [&]
        {
            INFO( "end" );
            if ( veq.size() )
            {
                CHECK( *(veq.end()-1) == *--deq.end() );
            }
        },
        [&]
        {
            INFO( "rend" );
            if ( veq.size() )
            {
                CHECK( *(veq.rend()-1) == *--deq.rend() );
            }
        },
        [&]
        {
            INFO( "const begin" );
            if ( veq.size() )
            {
                const auto veq2 = veq;
                const auto deq2 = deq;
                CHECK( *veq2.begin() == *deq2.begin() );
            }
        },
        [&]
        {
            INFO( "const rbegin" );
            if ( veq.size() )
            {
                const auto veq2 = veq;
                const auto deq2 = deq;
                CHECK( *veq2.rbegin() == *deq2.rbegin() );
            }
        },
        [&]
        {
            INFO( "const end" );
            if ( veq.size() )
            {
                const auto veq2 = veq;
                const auto deq2 = deq;
                CHECK( *(veq2.end()-1) == *--deq2.end() );
            }
        },
        [&]
        {
            INFO( "const rend" );
            if ( veq.size() )
            {
                const auto veq2 = veq;
                const auto deq2 = deq;
                CHECK( *(veq2.rend()-1) == *--deq2.rend() );
            }
        },
        [&]
        {
            INFO( "clear" );
            veq.clear();
            deq.clear();
        },
        [&]
        {
            INFO( "swap" );
            if ( deq.size() > 2 )
            {
                auto veq2 = TestType( veq.begin() + 1, veq.end() - 1 );
                auto vec2 = std::deque<typename TestType::value_type, typename TestType::allocator_type>( deq.begin() + 1, deq.end() - 1 );
                if ( veq.get_allocator() == veq2.get_allocator() && deq.get_allocator() == deq.get_allocator() )
                {
                    // UB, otherwise.
                    veq.swap( veq2 );
                    deq.swap( vec2 );
                }
            }
        }
    };

    for ( auto test_counter = 0; test_counter != 20'000; ++test_counter )
    {
        tests[ rand() % tests.size() ]();
        REQUIRE( std::equal( veq.begin(), veq.end(), deq.begin(), deq.end() ) );
    }
}
