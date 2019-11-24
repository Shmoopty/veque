/* 
 * 
 * veque test suite.
 * 
 * Additionally, valgrind claims there is no bad behavior throughout this usage.
 *
 *  Copyright (C) 2019 Drew Dormann
 * 
 */

#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include "include/veque.hpp"
#include <vector>
#include <string> 
#include <unordered_set> 
#include <string> 
#include <functional> 


// A trivial object should benefit from supporting memcpy/memmove
// and no destruction
struct LargeTrivialObject
{
    bool operator== (const LargeTrivialObject & other) const
    {
        return std::equal( std::begin(data), std::end(data), other.data );
    }
    int data[1024];
};

// A non-trivial object requires construction/destruction and potentially
// benefits from move operations
struct NonTrivialObject
{
    std::string data = std::string( 1024, 'W' );
    bool operator== (const NonTrivialObject & other) const
    {
        return data == other.data;
    }
};

// An object with move operations that aren't noexcept should use 
// copy operations to allow strong exception guarantee
struct ThrowingMoveConstructObject
{
    ThrowingMoveConstructObject() = default;
    ThrowingMoveConstructObject( const ThrowingMoveConstructObject& ) = default;
    ThrowingMoveConstructObject( ThrowingMoveConstructObject&& ) noexcept(false)
    {
        throw std::runtime_error("Failed move construction");
    }

    ThrowingMoveConstructObject& operator=( const ThrowingMoveConstructObject& ) = default;
    ThrowingMoveConstructObject& operator=( ThrowingMoveConstructObject&& ) = default;
    bool operator== (const ThrowingMoveConstructObject & other) const
    {
        return data == other.data;
    }
    std::string data = std::string( 1024, 'X' );
};

struct ThrowingMoveAssignObject
{
    ThrowingMoveAssignObject() = default;
    ThrowingMoveAssignObject( const ThrowingMoveAssignObject& ) = default;
    ThrowingMoveAssignObject( ThrowingMoveAssignObject&& ) = default;

    ThrowingMoveAssignObject& operator=( const ThrowingMoveAssignObject& ) = default;
    ThrowingMoveAssignObject& operator=( ThrowingMoveAssignObject&& ) noexcept(false)
    {
        throw std::runtime_error("Failed move assignment");
    }
    bool operator== (const ThrowingMoveAssignObject & other) const
    {
        return data == other.data;
    }
    std::string data = std::string( 1024, 'Y' );
};

struct ThrowingMoveObject
{
    ThrowingMoveObject() = default;
    ThrowingMoveObject( const ThrowingMoveObject& ) = default;
    ThrowingMoveObject( ThrowingMoveObject&& ) noexcept(false)
    {
        throw std::runtime_error("Failed move construction");
    }

    ThrowingMoveObject& operator=( const ThrowingMoveObject& ) = default;
    ThrowingMoveObject& operator=( ThrowingMoveObject&& ) noexcept(false)
    {
        throw std::runtime_error("Failed move assignment");
    }
    bool operator== (const ThrowingMoveObject & other) const
    {
        return data == other.data;
    }
    std::string data = std::string( 1024, 'Z' );
};

TEMPLATE_TEST_CASE( "veques can be sized and resized", "[veque][template]", bool, int, std::string, LargeTrivialObject, NonTrivialObject, ThrowingMoveConstructObject, ThrowingMoveAssignObject, ThrowingMoveObject )
{
    veque<TestType> v( 5 );

    REQUIRE( v.size() == 5 );
    REQUIRE( v.capacity() >= 5 );

    SECTION( "resizing bigger changes size and capacity" )
    {
        v.resize_back( 10 );

        REQUIRE( v.size() == 10 );
        REQUIRE( v.capacity() >= 10 );
        REQUIRE( v.capacity_back() >= 10 );

        v.resize_front( 15 );

        REQUIRE( v.size() == 15 );
        REQUIRE( v.capacity() >= 15 );
        REQUIRE( v.capacity_front() >= 15 );
    }
    SECTION( "resizing smaller changes size but not capacity" )
    {
        v.resize( 0 );

        REQUIRE( v.size() == 0 );
        REQUIRE( v.capacity() >= 5 );

        v.resize( 5 );

        SECTION( "We can use the 'swap trick' to reset the capacity" )
        {
            veque<TestType>( v ).swap( v );

            REQUIRE( v.capacity() == 5 );
        }
        SECTION( "Or we can use shrink_to_fit()" )
        {
            REQUIRE( v.size() == 5 );

            v.shrink_to_fit();

            CHECK( v.size() == 5 );
            CHECK( v.capacity() == 5 );
        }
    }
    SECTION( "reserving smaller does not change size or capacity" )
    {
        v.reserve( 0 );

        CHECK( v.size() == 5 );
        CHECK( v.capacity() >= 5 );
    }
    SECTION( "clearing" )
    {
        v.clear();

        CHECK( v.size() == 0 );
        CHECK( v.empty() );
    }
    SECTION( "reserve_front" )
    {
        v.reserve_front( 20 );
        CHECK( v.capacity_front() >= 20 );
    }
    SECTION( "reserve_back" )
    {
        v.reserve_back( 20 );
        CHECK( v.capacity_back() >= 20 );
    }
    SECTION( "reserve" )
    {
        v.reserve( 20 );
        CHECK( v.capacity_front() >= 20 );
        CHECK( v.capacity_back() >= 20 );
    }
    SECTION( "reserve less" )
    {
        auto old_capacity_front = v.capacity_front();
        auto old_capacity_back = v.capacity_back();

        v.reserve( 0 );
        
        CHECK( v.capacity_front() == old_capacity_front );
        CHECK( v.capacity_back() == old_capacity_back );
    }
}

TEMPLATE_TEST_CASE( "large end growth", "[veque][template]", bool, int /*, std::string, LargeTrivialObject, NonTrivialObject, ThrowingMoveConstructObject, ThrowingMoveAssignObject, ThrowingMoveObject*/ )
{
    typename veque<TestType>::size_type size = 5;
    veque<TestType> v( size );

    REQUIRE( v.size() == size );
    REQUIRE( v.capacity() >= size );
    
    SECTION( "push_back" )
    {
        TestType val{};
        for ( int i = 0; i < 2'000; ++i )
        {
            v.push_back( val );
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
    
    SECTION( "emplace_back" )
    {
        for ( int i = 0; i < 2'000; ++i )
        {
            v.emplace_back();
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
    SECTION( "push_front" )
    {
        TestType val{};
        for ( int i = 0; i < 2'000; ++i )
        {
            v.push_front( val );
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
    SECTION( "emplace_front" )
    {
        for ( int i = 0; i < 2'000; ++i )
        {
            v.emplace_front();
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
}

TEMPLATE_TEST_CASE( "large insertion growth", "[veque][template]", bool/*, int, std::string, LargeTrivialObject, NonTrivialObject*/ )
{
    typename veque<TestType>::size_type size = 5;
    veque<TestType> v( size );

    REQUIRE( v.size() == size );
    REQUIRE( v.capacity() >= size );

    SECTION( "insert begin" )
    {
        for ( int i = 0; i < 2'000; ++i )
        {
            TestType val{};
            v.insert( v.begin(), val );
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
    SECTION( "insert end" )
    {
        for ( int i = 0; i < 2'000; ++i )
        {
            TestType val{};
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
            TestType val{};
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
            TestType val{};
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
            TestType val{};
            v.insert( v.begin() + 2 * v.size() / 3, val );
            ++size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
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
    SECTION( "insert randomly" )
    {
        // Valgrind doesn't like std::random_device.
        //std::random_device rd;
        //std::mt19937 gen(rd());
        srand(time(NULL));
        for ( int i = 0; i < 2'000; ++i )
        {
            TestType val{};
            //auto index = std::uniform_int_distribution<>(0, v.size())(gen);    
            auto index = rand() % (v.size()+1);
            //v.insert( v.begin() + dis(gen), val );
            v.insert( v.begin() + index, val );
            ++size;
            REQUIRE( v.size() == size );
            REQUIRE( v.capacity() >= size );
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

TEMPLATE_TEST_CASE( "veques can be modified at either end with strong exception guarantee", "[veque][template]", bool, int, std::string, LargeTrivialObject, NonTrivialObject, ThrowingMoveConstructObject, ThrowingMoveAssignObject, ThrowingMoveObject )
{
    veque<TestType> v( 5 );

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
                TestType val;
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
                TestType val;
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

TEST_CASE( "veques emplace", "[veque]" )
{
    veque<std::string> v;
    
    v.emplace_back( "ABC" );
    CHECK( v == veque<std::string>{ std::string("ABC") } );

    v.emplace_front( 5, 'X' );
    CHECK( v == veque<std::string>{ std::string("XXXXX"), std::string("ABC") } );

    v.emplace_back( std::string_view("12345"), 1, 3 );
    CHECK( v == veque<std::string>{ std::string("XXXXX"), std::string("ABC"), std::string("234") } );
}

// Sample data, each in increasing comparison order
template<typename T, size_t index> const T val;
template<> const int val<int,0> = 0;
template<> const int val<int,1> = 1;
template<> const int val<int,2> = 2;
template<> const int val<int,3> = 3;
template<> const int val<int,4> = 4;
template<> const int val<int,5> = 5;
template<> const std::string val<std::string,0> = std::string(100, 'A');
template<> const std::string val<std::string,1> = std::string(200, 'B');
template<> const std::string val<std::string,2> = std::string(300, 'C');
template<> const std::string val<std::string,3> = std::string(400, 'D');
template<> const std::string val<std::string,4> = std::string(500, 'E');
template<> const std::string val<std::string,5> = std::string(600, 'F');
template<> const double val<double,0> = 00.0;
template<> const double val<double,1> = 11.0;
template<> const double val<double,2> = 22.0;
template<> const double val<double,3> = 33.0;
template<> const double val<double,4> = 44.0;
template<> const double val<double,5> = 55.0;
template<> const std::vector<int> val<std::vector<int>,0> = { 0, 1, 2 };
template<> const std::vector<int> val<std::vector<int>,1> = { 1, 2, 3 };
template<> const std::vector<int> val<std::vector<int>,2> = { 2, 3, 4 };
template<> const std::vector<int> val<std::vector<int>,3> = { 3, 4, 5 };
template<> const std::vector<int> val<std::vector<int>,4> = { 4, 5, 6 };
template<> const std::vector<int> val<std::vector<int>,5> = { 6, 7, 8 };


TEMPLATE_TEST_CASE( "std::vector interface parity", "[veque][template]", int, std::string, double, std::vector<int> )
{
    veque<TestType> veq;
    std::vector<TestType> vec;

    srand(time(NULL));
    
    auto tests = std::vector<std::function<void()>>{
        [&]
        {
            INFO( "resize" );
            auto new_size = rand() % 10'000;
            veq.resize(new_size);
            vec.resize(new_size);
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
            if ( veq.size() )
            {
                auto index = rand() % veq.size();
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
            auto item = val<TestType,0>;
            veq.push_back( item );
            vec.push_back( item );
        },
        [&]
        {
            INFO( "push_back1" );
            auto item = val<TestType,1>;
            veq.push_back( TestType{item} );
            vec.push_back( TestType{item} );
        },
        [&]
        {
            INFO( "emplace_back" );
            auto item = val<TestType,4>;
            veq.emplace_back( TestType{item} );
            vec.emplace_back( TestType{item} );
        },
        [&]
        {
            INFO( "insert2" );
            auto item = val<TestType,2>;
            auto index = veq.size() ? rand() % veq.size() : 0;
            veq.insert( veq.begin() + index, item );
            vec.insert( vec.begin() + index, item );
        },
        [&]
        {
            INFO( "insert3" );
            auto item = val<TestType,3>;
            auto index = veq.size() ? rand() % veq.size() : 0;
            veq.insert( veq.begin() + index, TestType{item} );
            vec.insert( vec.begin() + index, TestType{item});
        },
        [&]
        {
            INFO( "emplace" );
            auto item = val<TestType,3>;
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
            INFO( "swap" );
            if ( vec.size() > 2 )
            {
                auto veq2 = veque<TestType>( veq.begin() + 1, veq.end() - 1 );
                auto vec2 = std::vector<TestType>( vec.begin() + 1, vec.end() - 1 );
                veq.swap( veq2 );
                vec.swap( vec2 );
            }
        }
    };

    for ( auto test_counter = 0; test_counter != 20'000; ++test_counter )
    {
        tests[ rand() % tests.size() ]();
        REQUIRE( std::equal( veq.begin(), veq.end(), vec.begin(), vec.end() ) );
    }
}

TEMPLATE_TEST_CASE( "std::deque interface parity", "[veque][template]", int, std::string, double, std::vector<int> )
{
    veque<TestType> veq;
    std::deque<TestType> deq;

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
            INFO( "[]" );
            if ( veq.size() )
            {
                auto index = rand() % veq.size();
                CHECK( veq[index] == deq[index] );
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
            INFO( "back" );
            if ( veq.size() )
            {
                CHECK( veq.back() == deq.back() );
            }
        },
        [&]
        {
            INFO( "push_back0" );
            auto item = val<TestType,0>;
            veq.push_back( item );
            deq.push_back( item );
        },
        [&]
        {
            INFO( "push_back1" );
            auto item = val<TestType,1>;
            veq.push_back( TestType{item} );
            deq.push_back( TestType{item} );
        },
        [&]
        {
            INFO( "emplace_back" );
            auto item = val<TestType,4>;
            veq.emplace_back( TestType{item} );
            deq.emplace_back( TestType{item} );
        },
        [&]
        {
            INFO( "push_front5" );
            auto item = val<TestType,5>;
            veq.push_front( item );
            deq.push_front( item );
        },
        [&]
        {
            INFO( "push_front4" );
            auto item = val<TestType,4>;
            veq.push_front( TestType{item} );
            deq.push_front( TestType{item} );
        },
        [&]
        {
            INFO( "emplace_front" );
            auto item = val<TestType,4>;
            veq.emplace_front( TestType{item} );
            deq.emplace_front( TestType{item} );
        },
        [&]
        {
            INFO( "insert2" );
            auto item = val<TestType,2>;
            auto index = veq.size() ? rand() % veq.size() : 0;
            veq.insert( veq.begin() + index, item );
            deq.insert( deq.begin() + index, item );
        },
        [&]
        {
            INFO( "insert3" );
            auto item = val<TestType,3>;
            auto index = veq.size() ? rand() % veq.size() : 0;
            veq.insert( veq.begin() + index, TestType{item} );
            deq.insert( deq.begin() + index, TestType{item});
        },
        [&]
        {
            INFO( "emplace" );
            auto item = val<TestType,3>;
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
            INFO( "clear" );
            veq.clear();
            deq.clear();
        },
        [&]
        {
            INFO( "swap" );
            if ( deq.size() > 2 )
            {
                auto veq2 = veque<TestType>( veq.begin() + 1, veq.end() - 1 );
                auto vec2 = std::deque<TestType>( deq.begin() + 1, deq.end() - 1 );
                veq.swap( veq2 );
                deq.swap( vec2 );
            }
        }
    };

    for ( auto test_counter = 0; test_counter != 20'000; ++test_counter )
    {
        tests[ rand() % tests.size() ]();
        REQUIRE( std::equal( veq.begin(), veq.end(), deq.begin(), deq.end() ) );
    }
}

TEMPLATE_TEST_CASE( "veque element ordering and access", "[veque][template]", int, std::string, double, std::vector<int> )
{
    veque<TestType> veq1;
    
    CHECK( veq1.empty() );
    CHECK( veq1.size() == 0 );

    veq1.push_back( val<TestType,1> );
    veq1.push_back( val<TestType,2> );
    veq1.emplace_back( val<TestType,3> );

    CHECK( !veq1.empty() );
    CHECK( veq1.size() == 3 );
    CHECK( veq1 == veque<TestType>{ val<TestType,1>, val<TestType,2>, val<TestType,3> } );
    CHECK( veq1.front() == val<TestType,1> );
    CHECK( veq1.back() == val<TestType,3> );

    veq1.push_front( val<TestType,4> );
    veq1.emplace_front( val<TestType,5> );

    CHECK( !veq1.empty() );
    CHECK( veq1.size() == 5 );
    CHECK( veq1 == veque<TestType>{ val<TestType,5>, val<TestType,4>, val<TestType,1>, val<TestType,2>, val<TestType,3> } );
    CHECK( veq1.front() == val<TestType,5> );
    CHECK( veq1.back() == val<TestType,3> );
    
    veq1.pop_back();

    CHECK( !veq1.empty() );
    CHECK( veq1.size() == 4 );
    CHECK( veq1 == veque<TestType>{ val<TestType,5>, val<TestType,4>, val<TestType,1>, val<TestType,2> } );
    CHECK( veq1.front() == val<TestType,5> );
    CHECK( veq1.back() == val<TestType,2> );
    CHECK( veq1[0] == val<TestType,5> );
    CHECK( veq1.at(0) == val<TestType,5> );
    CHECK( veq1[1] == val<TestType,4> );
    CHECK( veq1.at(1) == val<TestType,4> );
    CHECK( veq1[2] == val<TestType,1> );
    CHECK( veq1.at(2) == val<TestType,1> );
    CHECK( veq1[3] == val<TestType,2> );
    CHECK( veq1.at(3) == val<TestType,2> );
    CHECK_THROWS( veq1.at(4) );
    
    veq1.pop_front();

    CHECK( !veq1.empty() );
    CHECK( veq1.size() == 3 );
    CHECK( veq1 == veque<TestType>{ val<TestType,4>, val<TestType,1>, val<TestType,2> } );
    CHECK( veq1.front() == val<TestType,4> );
    CHECK( veq1.back() == val<TestType,2> );
   
    auto veq2 = veq1;

    CHECK( veq1 == veq2 );
    CHECK( !(veq1 != veq2) );

    veq2.emplace_front( val<TestType,0> );
    
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

    veq2 = veque<TestType>{ val<TestType,4>, val<TestType,1>, val<TestType,2> };

    CHECK( veq1 == veq2 );
    CHECK( !(veq1 != veq2) );
}

TEMPLATE_TEST_CASE( "insert/erase", "[veque][template]", int, std::string, double, std::vector<int> )
{
    // (Template deduction guide)
    veque veq{ val<TestType,1>, val<TestType,2>, val<TestType,3>  };
    veq.reserve(20);
    
    CHECK( !veq.empty() );
    CHECK( veq.size() == 3 );
    
    SECTION( "l-value insertion" )
    {
        veq.insert( veq.begin(), val<TestType,0> );

        REQUIRE( veq.size() == 4 );
        CHECK( veq[0] == val<TestType,0> );
        CHECK( veq[1] == val<TestType,1> );
        CHECK( veq[2] == val<TestType,2> );
        CHECK( veq[3] == val<TestType,3> );
        CHECK( veq == veque<TestType>{ val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3> } );

        veq.insert( veq.end(), val<TestType,0> );

        CHECK( veq.size() == 5 );
        CHECK( veq == veque<TestType>{ val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3>, val<TestType,0> } );
    }
    SECTION( "l-value resizing insertion" )
    {
        veq.shrink_to_fit();
        veq.insert( veq.begin(), val<TestType,0> );

        REQUIRE( veq.size() == 4 );
        CHECK( veq[0] == val<TestType,0> );
        CHECK( veq[1] == val<TestType,1> );
        CHECK( veq[2] == val<TestType,2> );
        CHECK( veq[3] == val<TestType,3> );
        CHECK( veq == veque<TestType>{ val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3> } );

        veq.shrink_to_fit();
        veq.insert( veq.end(), val<TestType,0> );

        REQUIRE( veq.size() == 5 );
        CHECK( veq[0] == val<TestType,0> );
        CHECK( veq[1] == val<TestType,1> );
        CHECK( veq[2] == val<TestType,2> );
        CHECK( veq[3] == val<TestType,3> );
        CHECK( veq[4] == val<TestType,0> );
        CHECK( veq == veque<TestType>{ val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3>, val<TestType,0> } );
    }
    SECTION( "r-value insertion" )
    {
        veq.insert( veq.begin(), TestType(val<TestType,0>) );

        CHECK( veq.size() == 4 );
        CHECK( veq == veque<TestType>{ val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3> } );

        veq.insert( veq.end(), TestType(val<TestType,0>) );

        CHECK( veq.size() == 5 );
        CHECK( veq == veque<TestType>{ val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3>, val<TestType,0> } );
    }
    SECTION( "r-value resizing insertion" )
    {
        veq.shrink_to_fit();
        veq.insert( veq.begin(), TestType(val<TestType,0>) );

        CHECK( veq.size() == 4 );
        CHECK( veq == veque<TestType>{ val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3> } );

        veq.shrink_to_fit();
        veq.insert( veq.end(), TestType(val<TestType,0>) );

        CHECK( veq.size() == 5 );
        CHECK( veq == veque<TestType>{ val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3>, val<TestType,0> } );
    }
    SECTION( "pop erasure" )
    {
        // pop erasure
        veq.pop_front();
        
        CHECK( veq.size() == 2 );
        CHECK( veq == veque<TestType>{ val<TestType,2>, val<TestType,3> } );

        veq.pop_back();
        CHECK( veq.size() == 1 );
        CHECK( veq == veque<TestType>{ val<TestType,2> } );
    }
    SECTION( "val,count insertion" )
    {
        veq.insert( veq.end(), typename veque<TestType>::size_type(2), val<TestType,4> );

        CHECK( veq.size() == 5 );
        CHECK( veq == veque<TestType>{ val<TestType,1>, val<TestType,2>, val<TestType,3>, val<TestType,4>, val<TestType,4> } );

    }
    SECTION( "val,count resizing insertion" )
    {
        veq.shrink_to_fit();
        veq.insert( veq.end(), typename veque<TestType>::size_type(2), val<TestType,4> );

        CHECK( veq.size() == 5 );
        CHECK( veq == veque<TestType>{ val<TestType,1>, val<TestType,2>, val<TestType,3>, val<TestType,4>, val<TestType,4> } );

    }
    SECTION( "range insertion" )
    {
        auto veq2 = veque<TestType>{ val<TestType,5>, val<TestType,4>, val<TestType,3> };
        veq.insert( veq.begin(), veq2.begin(), veq2.end() );

        CHECK( veq.size() == 6 );
        CHECK( veq == veque<TestType>{ val<TestType,5>, val<TestType,4>, val<TestType,3>, val<TestType,1>, val<TestType,2>, val<TestType,3> } );
    }
    SECTION( "range resizing  insertion" )
    {
        veq.shrink_to_fit();
        auto veq2 = veque<TestType>{ val<TestType,5>, val<TestType,4>, val<TestType,3> };
        veq.insert( veq.begin(), veq2.begin(), veq2.end() );

        CHECK( veq.size() == 6 );
        CHECK( veq == veque<TestType>{ val<TestType,5>, val<TestType,4>, val<TestType,3>, val<TestType,1>, val<TestType,2>, val<TestType,3> } );
    }
    SECTION( "resize_back erasure" )
    {
        // resize erasure
        veq.resize_back( 1 );

        CHECK( veq.size() == 1 );
        CHECK( veq == veque<TestType>{ val<TestType,1> } );
    }
    SECTION( "resize_front erasure" )
    {
        // resize erasure
        veq.resize_front( 1 );

        CHECK( veq.size() == 1 );
        CHECK( veq == veque<TestType>{ val<TestType,3> } );
    }
    SECTION( "resize_front erasure" )
    {
        // initializer list insertion
        veq.insert( veq.end(), {val<TestType,0>, val<TestType,1>, val<TestType,2>} );

        CHECK( veq.size() == 6 );
        CHECK( veq == veque<TestType>{ val<TestType,1>, val<TestType,2>, val<TestType,3>, val<TestType,0>, val<TestType,1>, val<TestType,2> } );
    }
    SECTION( "iterator erasure 1" )
    {
        veq.erase( veq.begin() );

        CHECK( veq.size() == 2 );
        CHECK( veq[0] == val<TestType,2> );
        CHECK( veq[1] == val<TestType,3> );
        CHECK( veq == veque<TestType>{ val<TestType,2>, val<TestType,3> } );
    }
    SECTION( "iterator erasure 2" )
    {
        veq.erase( veq.begin() + 1 );

        CHECK( veq.size() == 2 );
        CHECK( veq[0] == val<TestType,1> );
        CHECK( veq[1] == val<TestType,3> );
        CHECK( veq == veque<TestType>{ val<TestType,1>, val<TestType,3> } );
    }
    SECTION( "iterator erasure 3" )
    {
        veq.erase( veq.end() - 1 );

        CHECK( veq.size() == 2 );
        CHECK( veq[0] == val<TestType,1> );
        CHECK( veq[1] == val<TestType,2> );
        CHECK( veq == veque<TestType>{ val<TestType,1>, val<TestType,2> } );
    }
    SECTION( "range erasure begin" )
    {
        veq.assign( { val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3>, val<TestType,4>, val<TestType,5>} );
        veq.erase( veq.begin(), veq.begin() + 3 );
        CHECK( veq.size() == 3 );
        CHECK( veq == veque<TestType>{ val<TestType,3>, val<TestType,4>, val<TestType,5> } );
    }
    SECTION( "range erasure end" )
    {
        veq.assign( { val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3>, val<TestType,4>, val<TestType,5>} );
        veq.erase( veq.begin() + 3, veq.end() );
        CHECK( veq.size() == 3 );
        CHECK( veq == veque<TestType>{ val<TestType,0>, val<TestType,1>, val<TestType,2> } );
    }
    SECTION( "range erasure mid near front" )
    {
        veq.assign( { val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3>, val<TestType,4>, val<TestType,5>} );
        veq.erase( veq.begin() + 1, veq.begin() + 4 );
        CHECK( veq.size() == 3 );
        CHECK( veq == veque<TestType>{ val<TestType,0>, val<TestType,4>, val<TestType,5> } );
    }
    SECTION( "range erasure mid near back" )
    {
        veq.assign( { val<TestType,0>, val<TestType,1>, val<TestType,2>, val<TestType,3>, val<TestType,4>, val<TestType,5>} );
        veq.erase( veq.begin() + 2, veq.begin() + 5 );
        CHECK( veq.size() == 3 );
        CHECK( veq == veque<TestType>{ val<TestType,0>, val<TestType,1>, val<TestType,5> } );
    }
    SECTION( "Range assign" )
    {
        veque<TestType> veq3;
        veq3.assign( veq.begin(), veq.end() );
        CHECK( veq3.size() == 3 );
        CHECK( veq3 == veque<TestType>{ val<TestType,1>, val<TestType,2>, val<TestType,3> } );
    }
    SECTION( "count,val assign" )
    {
        veque<TestType> veq4;
        veq4.assign( typename veque<TestType>::size_type(3), val<TestType,2> );
        CHECK( veq4.size() == 3 );
        CHECK( veq4 == veque<TestType>{ val<TestType,2>, val<TestType,2>, val<TestType,2> } );
    }
}

TEMPLATE_TEST_CASE( "hashing", "[veque][template]", int, std::string, double )
{
    std::unordered_set<veque<TestType>> uset;
    
    uset.emplace( veque<TestType>{val<TestType,1>} );
    uset.emplace( veque<TestType>{val<TestType,2>} );
    uset.emplace( veque<TestType>{val<TestType,3>} );

    CHECK( uset.size() == 3 );
    CHECK( uset.count(veque<TestType>{val<TestType,0>}) == 0 );
    CHECK( uset.count(veque<TestType>{val<TestType,1>}) == 1 );
    CHECK( uset.count(veque<TestType>{val<TestType,2>}) == 1 );
    CHECK( uset.count(veque<TestType>{val<TestType,3>}) == 1 );
    CHECK( uset.count(veque<TestType>{val<TestType,4>}) == 0 );
    CHECK( uset.count(veque<TestType>{val<TestType,5>}) == 0 );

    uset.emplace( veque<TestType>{val<TestType,3>} );

    CHECK( uset.size() == 3 );
    CHECK( uset.count(veque<TestType>{val<TestType,0>}) == 0 );
    CHECK( uset.count(veque<TestType>{val<TestType,1>}) == 1 );
    CHECK( uset.count(veque<TestType>{val<TestType,2>}) == 1 );
    CHECK( uset.count(veque<TestType>{val<TestType,3>}) == 1 );
    CHECK( uset.count(veque<TestType>{val<TestType,4>}) == 0 );
    CHECK( uset.count(veque<TestType>{val<TestType,5>}) == 0 );
}
