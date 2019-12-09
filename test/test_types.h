#ifndef VEQUE_TEST_TYPES_H
#define VEQUE_TEST_TYPES_H

/* 
 * 
 * veque::veque test suite.
 * 
 * Additionally, valgrind claims there is no bad behavior throughout this usage.
 *
 *  Copyright (C) 2019 Drew Dormann
 * 
 */

#include <vector>
#include <deque>
#include "veque.hpp"

// An allocator that is stateful, unlikely to be equal to another, and aware of being mismatched
template<typename T>
struct StatefulAllocator
{
    using value_type = T;
    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap = std::false_type;
    using is_always_equal = std::false_type;

    static constexpr auto barrier_size = 64;
    std::uint32_t barrier = 0xDEADBEEF + rand();

    StatefulAllocator() = default;
    constexpr StatefulAllocator(const StatefulAllocator &) = default;
    constexpr StatefulAllocator(StatefulAllocator &&) = default;
    template< class U >
    StatefulAllocator( const StatefulAllocator<U>& other ) noexcept
        : barrier{ other.barrier }
    {
    }

    constexpr StatefulAllocator& operator=(const StatefulAllocator &) = default;
    constexpr StatefulAllocator& operator=(StatefulAllocator &&) = default;

    T* allocate( std::size_t n )
    {
        std::byte * ptr = reinterpret_cast<std::byte*>( std::malloc( n * sizeof(T) + 2 * barrier_size ) );
        *reinterpret_cast<std::uint64_t*>(ptr) = barrier;
        *reinterpret_cast<std::uint64_t*>(ptr + n * sizeof(T) + barrier_size) = barrier;
        return reinterpret_cast<T*>(ptr + barrier_size);
    }

    void deallocate( T* p, std::size_t n )
    {
        if ( p )
        {
            std::byte * ptr = reinterpret_cast<std::byte*>(p) - barrier_size;
            if ( *reinterpret_cast<std::uint64_t*>(ptr) != barrier )
            {
                throw std::runtime_error{"StatefulAllocator mismatch"};
            }
            if ( *reinterpret_cast<std::uint64_t*>(ptr + n * sizeof(T) + barrier_size) != barrier )
            {
                throw std::runtime_error{"StatefulAllocator mismatch"};
            }
            std::free( ptr );
        }
    }
};

// An allocator that expects to propagate to other containers
template<typename T>
struct PropagatingStatefulAllocator : StatefulAllocator<T>
{
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    PropagatingStatefulAllocator() = default;
    constexpr PropagatingStatefulAllocator(const PropagatingStatefulAllocator<T> &) = default;
    constexpr PropagatingStatefulAllocator(PropagatingStatefulAllocator<T> &&) = default;
    template< class U >
    PropagatingStatefulAllocator( const PropagatingStatefulAllocator<U>& other ) noexcept : StatefulAllocator<T>{other} {}
    constexpr PropagatingStatefulAllocator& operator=(const PropagatingStatefulAllocator<T> &o) = default;
    constexpr PropagatingStatefulAllocator& operator=(PropagatingStatefulAllocator<T> &&o) = default;
};

// An allocator with construct/destroy members that must always be called.
template<typename T>
struct CountingAllocator
{
    using value_type = T;
    using is_always_equal = std::true_type;
    static inline size_t counter = 0;

    template< class U, class... Args >
    void construct( U* p, Args&&... args )
    {
        ++counter;
        ::new((void *)p) U(std::forward<Args>(args)...);
    }
    
    template< class U >
    void destroy( U* p )
    {
        --counter;
        p->~U();
    }

    T* allocate( std::size_t n )
    {
        return reinterpret_cast<T*>( std::malloc( n * sizeof(T) ) );
    }

    void deallocate( T* p, std::size_t )
    {
        std::free(p);
    }
    
    CountingAllocator() = default;
    constexpr CountingAllocator(const CountingAllocator<T> &) = default;
    constexpr CountingAllocator(CountingAllocator<T> &&) = default;
    template< class U >
    CountingAllocator( const CountingAllocator<U>& ) noexcept {}
    constexpr CountingAllocator& operator=(const CountingAllocator<T> &o) = default;
    constexpr CountingAllocator& operator=(CountingAllocator<T> &&o) = default;
};

template< class T1, class T2 >
constexpr bool operator==( const StatefulAllocator<T1>& lhs, const StatefulAllocator<T2>& rhs ) noexcept
{
    return lhs.barrier == rhs.barrier;
}

template< class T1, class T2 >
constexpr bool operator!=( const StatefulAllocator<T1>& lhs, const StatefulAllocator<T2>& rhs ) noexcept
{
    return lhs.barrier != rhs.barrier;
}

template< class T1, class T2 >
constexpr bool operator==( const CountingAllocator<T1>&, const CountingAllocator<T2>& ) noexcept
{
    return true;
}

template< class T1, class T2 >
constexpr bool operator!=( const CountingAllocator<T1>&, const CountingAllocator<T2>& ) noexcept
{
    return false;
}

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

template<typename T>
using StdVeque = veque::veque<T>;

template<typename T>
using GrumpyVeque = veque::veque<T,veque::fast_resize_traits,StatefulAllocator<T>>;

template<typename T>
using PropogatingGrumpyVeque = veque::veque<T,veque::std_vector_traits,PropagatingStatefulAllocator<T>>;

template<typename T>
using AllocCountingVeque = veque::veque<T,veque::no_reserve_traits,CountingAllocator<T>>;

template<typename Container>
constexpr bool is_using_counting_allocator = std::is_same_v< typename Container::allocator_type, CountingAllocator<typename Container::value_type> >;


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


#endif // VEQUE_TEST_TYPES_H
