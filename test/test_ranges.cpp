/* 
 * 
 * veque::veque test suite.
 * 
 * Additionally, valgrind claims there is no bad behavior throughout this usage.
 *
 *  Copyright (C) 2019-2026 Drew Dormann
 * 
 */

#include <cstdlib> 
#include <forward_list> 
#include <list> 
#include <ranges> 
#include <string> 
#include <vector> 
#include "catch.hpp"
#include "test_types.h"

TEMPLATE_PRODUCT_TEST_CASE( "veque::veque range construction and insertion", "[veque::veque][template]", (StdVeque, GrumpyVeque, PropogatingGrumpyVeque, AllocCountingVeque), (
        (int,veque::fast_resize_traits), (int,veque::std_vector_traits), (int,veque::no_reserve_traits), (int,front_vector_traits), 
        (std::string,veque::fast_resize_traits), (std::string,veque::std_vector_traits), (std::string,veque::no_reserve_traits), (std::string,front_vector_traits), 
        (double,veque::fast_resize_traits), (double,veque::std_vector_traits), (double,veque::no_reserve_traits), (double,front_vector_traits), 
        (std::vector<int>,veque::fast_resize_traits), (std::vector<int>,veque::std_vector_traits), (std::vector<int>,veque::no_reserve_traits), (std::vector<int>,front_vector_traits)
        ) )
{
    using value_type = typename TestType::value_type;

    const std::vector<value_type> source{ val<value_type,0>, val<value_type,1>, val<value_type,2> };

    // take_while_view's end() is a sentinel of another type, so the range is
    // not a common_range and the iterator pair constructor cannot accept it.
    const auto sentinel_terminated = [&source]
    {
        return source | std::views::take_while( []( const value_type & ) { return true; } );
    };

#ifdef __cpp_lib_containers_ranges
    SECTION( "from_range_t construction" )
    {
        TestType v( std::from_range, source );

        REQUIRE( v.size() == 3 );
        REQUIRE( v[0] == val<value_type,0> );
        REQUIRE( v[1] == val<value_type,1> );
        REQUIRE( v[2] == val<value_type,2> );
    }

    SECTION( "from_range_t construction from a sentinel-terminated range" )
    {
        TestType v( std::from_range, sentinel_terminated() );

        REQUIRE( v.size() == 3 );
        REQUIRE( v[0] == val<value_type,0> );
        REQUIRE( v[2] == val<value_type,2> );
    }
#endif

    SECTION( "append_range" )
    {
        TestType v;
        v.push_back( val<value_type,3> );
        v.append_range( source );

        REQUIRE( v.size() == 4 );
        REQUIRE( v[0] == val<value_type,3> );
        REQUIRE( v[1] == val<value_type,0> );
        REQUIRE( v[3] == val<value_type,2> );
    }

    SECTION( "prepend_range" )
    {
        TestType v;
        v.push_back( val<value_type,3> );
        v.prepend_range( source );

        REQUIRE( v.size() == 4 );
        REQUIRE( v[0] == val<value_type,0> );
        REQUIRE( v[2] == val<value_type,2> );
        REQUIRE( v[3] == val<value_type,3> );
    }

    SECTION( "prepend_range from a sentinel-terminated range keeps order" )
    {
        TestType v;
        v.push_back( val<value_type,3> );
        v.prepend_range( sentinel_terminated() );

        REQUIRE( v.size() == 4 );
        REQUIRE( v[0] == val<value_type,0> );
        REQUIRE( v[1] == val<value_type,1> );
        REQUIRE( v[2] == val<value_type,2> );
        REQUIRE( v[3] == val<value_type,3> );
    }

    SECTION( "insert_range" )
    {
        TestType v;
        v.push_back( val<value_type,3> );
        v.push_back( val<value_type,4> );
        v.insert_range( v.begin() + 1, source );

        REQUIRE( v.size() == 5 );
        REQUIRE( v[0] == val<value_type,3> );
        REQUIRE( v[1] == val<value_type,0> );
        REQUIRE( v[3] == val<value_type,2> );
        REQUIRE( v[4] == val<value_type,4> );
    }

    SECTION( "assign_range" )
    {
        TestType v;
        v.push_back( val<value_type,3> );
        v.push_back( val<value_type,4> );
        v.push_back( val<value_type,5> );
        v.assign_range( source );

        REQUIRE( v.size() == 3 );
        REQUIRE( v[0] == val<value_type,0> );
        REQUIRE( v[2] == val<value_type,2> );
    }

    SECTION( "assign_range growing, from a forward-only source" )
    {
        const std::list<value_type> longer{ val<value_type,0>, val<value_type,1>, val<value_type,2>, val<value_type,3>, val<value_type,4> };

        TestType v;
        v.reserve( 32 );
        v.push_back( val<value_type,5> );
        v.assign_range( longer );

        REQUIRE( v.size() == 5 );
        REQUIRE( v[0] == val<value_type,0> );
        REQUIRE( v[4] == val<value_type,4> );
    }

    SECTION( "assign from forward-only iterators" )
    {
        const std::forward_list<value_type> singly{ val<value_type,0>, val<value_type,1>, val<value_type,2> };

        TestType v;
        v.reserve( 32 );
        v.push_back( val<value_type,5> );
        v.assign( singly.begin(), singly.end() );

        REQUIRE( v.size() == 3 );
        REQUIRE( v[0] == val<value_type,0> );
        REQUIRE( v[2] == val<value_type,2> );
    }
}

namespace
{
    template<typename T>
    struct AllocationCountingAllocator
    {
        using value_type = T;
        using is_always_equal = std::true_type;
        static inline std::size_t allocations = 0;

        AllocationCountingAllocator() = default;
        template< class U >
        AllocationCountingAllocator( const AllocationCountingAllocator<U>& ) noexcept {}

        T* allocate( std::size_t n )
        {
            ++allocations;
            return reinterpret_cast<T*>( std::malloc( n * sizeof(T) ) );
        }

        void deallocate( T* p, std::size_t )
        {
            std::free( p );
        }
    };

    template< class T1, class T2 >
    constexpr bool operator==( const AllocationCountingAllocator<T1>&, const AllocationCountingAllocator<T2>& ) noexcept
    {
        return true;
    }

    using CountedVeque = veque::veque<int, veque::fast_resize_traits, AllocationCountingAllocator<int>>;

    using IotaIterator = std::ranges::iterator_t<decltype(std::views::iota(1,2))>;
}

// A view whose iterator yields prvalues satisfies std::random_access_iterator
// while reporting input_iterator_tag.  Choosing the single-pass path from that
// tag reallocates on every growth step instead of measuring the range once.
static_assert( std::random_access_iterator<IotaIterator> );
static_assert( std::is_same_v<std::iterator_traits<IotaIterator>::iterator_category, std::input_iterator_tag> );

#ifdef __cpp_lib_ranges_to_container
TEST_CASE( "veque::veque collects a measurable range in one allocation", "[veque::veque]" )
{
    SECTION( "common and sized" )
    {
        AllocationCountingAllocator<int>::allocations = 0;
        auto v = std::ranges::to<CountedVeque>( std::views::iota( 1, 100 ) );

        REQUIRE( v.size() == 99 );
        REQUIRE( AllocationCountingAllocator<int>::allocations == 1 );
    }

    SECTION( "sentinel-terminated" )
    {
        AllocationCountingAllocator<int>::allocations = 0;
        auto v = std::ranges::to<CountedVeque>( std::views::iota( 1 ) | std::views::take( 99 ) );

        REQUIRE( v.size() == 99 );
        REQUIRE( AllocationCountingAllocator<int>::allocations == 1 );
    }

    SECTION( "not sized" )
    {
        AllocationCountingAllocator<int>::allocations = 0;
        auto v = std::ranges::to<CountedVeque>( std::views::iota( 1, 199 )
                                              | std::views::filter( []( int i ) { return i % 2 == 1; } ) );

        REQUIRE( v.size() == 99 );
        REQUIRE( AllocationCountingAllocator<int>::allocations == 1 );
    }
}
#endif
