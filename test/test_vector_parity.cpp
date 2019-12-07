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

TEMPLATE_PRODUCT_TEST_CASE( "std::vector interface parity", "[veque::veque][template]", (StdVeque, GrumpyVeque, PropogatingGrumpyVeque, AllocCountingVeque), (int, std::string, double, std::vector<int> ) )
{
    using VectorType = std::vector<typename TestType::value_type, typename TestType::allocator_type>;
            
    TestType veq;
    VectorType vec;

    srand(time(NULL));
    
    REQUIRE( veq.max_size() >= 20'000 );
    
    auto tests = std::vector<std::function<void()>>{
        [&]
        {
            INFO( "data" );
            CHECK( veq == TestType( veq.data(), veq.data() + veq.size() ) );
            CHECK( vec == VectorType( vec.data(), vec.data() + vec.size() ) );
        },
        [&]
        {
            INFO( "const data" );
            const auto & veq2 = veq;
            const auto & vec2 = vec;
            CHECK( veq2 == TestType( veq2.data(), veq2.data() + veq2.size() ) );
            CHECK( vec2 == VectorType( vec2.data(), vec2.data() + vec2.size() ) );
        },
        [&]
        {
            INFO( "il assign" );
            veq = { val<typename TestType::value_type,4>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2> };
            vec = { val<typename TestType::value_type,4>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2> };
        },
        [&]
        {
            INFO( "rvalue assign" );
            veq = TestType{ val<typename TestType::value_type,4>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2> };
            vec = VectorType{ val<typename TestType::value_type,4>, val<typename TestType::value_type,1>, val<typename TestType::value_type,2> };
        },
        [&]
        {
            INFO( "resize" );
            auto new_size = rand() % 10'000;
            veq.resize(new_size);
            vec.resize(new_size);
        },
        [&]
        {
            INFO( "resize 2" );
            auto new_size = rand() % 10'000;
            veq.resize(new_size, val<typename TestType::value_type,2>);
            vec.resize(new_size, val<typename TestType::value_type,2>);
        },
        [&]
        {
            INFO( "at" );
            if ( veq.size() )
            {
                auto index = rand() % veq.size();
                CHECK( veq.at(index) == vec.at(index) );
            }
        },
        [&]
        {
            INFO( "[]" );
            if ( veq.ssize() )
            {
                auto index = rand() % veq.ssize();
                CHECK( veq[index] == vec[index] );
            }
        },
        [&]
        {
            INFO( "front" );
            if ( veq.size() )
            {
                CHECK( veq.front() == vec.front() );
            }
        },
        [&]
        {
            INFO( "back" );
            if ( veq.size() )
            {
                CHECK( veq.back() == vec.back() );
            }
        },
        [&]
        {
            INFO( "push_back0" );
            auto item = val<typename TestType::value_type,0>;
            veq.push_back( item );
            vec.push_back( item );
        },
        [&]
        {
            INFO( "push_back1" );
            auto item = val<typename TestType::value_type,1>;
            veq.push_back( typename TestType::value_type{item} );
            vec.push_back( typename TestType::value_type{item} );
        },
        [&]
        {
            INFO( "emplace_back" );
            auto item = val<typename TestType::value_type,4>;
            veq.emplace_back( typename TestType::value_type{item} );
            vec.emplace_back( typename TestType::value_type{item} );
        },
        [&]
        {
            INFO( "insert2" );
            auto item = val<typename TestType::value_type,2>;
            auto index = veq.size() ? rand() % veq.size() : 0;
            veq.insert( veq.begin() + index, item );
            vec.insert( vec.begin() + index, item );
        },
        [&]
        {
            INFO( "insert3" );
            auto item = val<typename TestType::value_type,3>;
            auto index = veq.size() ? rand() % veq.size() : 0;
            veq.insert( veq.begin() + index, typename TestType::value_type{item} );
            vec.insert( vec.begin() + index, typename TestType::value_type{item});
        },
        [&]
        {
            INFO( "emplace" );
            auto index = veq.size() ? rand() % veq.size() : 0;
            veq.emplace( veq.begin() + index );
            vec.emplace( vec.begin() + index );
        },
        [&]
        {
            INFO( "begin" );
            if ( veq.size() )
            {
                CHECK( *veq.begin() == *vec.begin() );
            }
        },
        [&]
        {
            INFO( "rbegin" );
            if ( veq.size() )
            {
                CHECK( *veq.rbegin() == *vec.rbegin() );
            }
        },
        [&]
        {
            INFO( "clear" );
            veq.clear();
            vec.clear();
        },
        [&]
        {
            INFO( "reserve" );
            auto new_size = rand() % 10'000;
            veq.reserve(new_size);
            vec.reserve(new_size);
        },
        [&]
        {
            INFO( "reserve" );
            auto new_size = rand() % 10'000;
            veq.reserve(new_size);
            vec.reserve(new_size);
        },
        [&]
        {
            INFO( "swap 1" );
            if ( vec.size() > 2 )
            {
                auto veq2 = TestType( veq.begin() + 1, veq.end() - 1 );
                auto vec2 = VectorType( vec.begin() + 1, vec.end() - 1 );
                if ( veq.get_allocator() == veq2.get_allocator() && vec.get_allocator() == vec2.get_allocator() )
                {
                    // UB, otherwise.
                    veq.swap( veq2 );
                    vec.swap( vec2 );
                }
            }
        },
        [&]
        {
            INFO( "swap 2" );
            if ( vec.size() > 2 )
            {
                auto veq2 = TestType( veq.begin() + 1, veq.end() - 1 );
                auto vec2 = VectorType( vec.begin() + 1, vec.end() - 1 );
                if ( veq.get_allocator() == veq2.get_allocator() && vec.get_allocator() == vec2.get_allocator() )
                {
                    // UB, otherwise.
                    using std::swap;
                    swap( veq, veq2 );
                    swap( vec, vec2 );
                }
            }
        },
        [&]
        {
            INFO( "swap 3" );
            if ( vec.size() > 2 )
            {
                auto veq2 = TestType( veq.begin() + 1, veq.end() - 1 );
                auto veq3 = TestType( std::move(veq2), veq.get_allocator() );
                auto vec2 = VectorType( vec.begin() + 1, vec.end() - 1 );
                auto vec3 = VectorType( std::move(vec2), vec.get_allocator() );

                // UB, otherwise.
                if ( veq.get_allocator() == veq2.get_allocator() && vec.get_allocator() == vec2.get_allocator() )
                {
                    using std::swap;
                    swap( veq, veq2 );
                    swap( vec, vec2 );
                }
            }
        }
    };

    for ( auto test_counter = 0; test_counter != 20'000; ++test_counter )
    {
        tests[ rand() % tests.size() ]();
        REQUIRE( std::equal( veq.begin(), veq.end(), vec.begin(), vec.end() ) );
    }
}
