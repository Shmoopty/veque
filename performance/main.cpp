/*
 *
 * veque performance test
 *
 * Copyright (C) 2019 Drew Dormann
 *
 * SAMPLE OUTPUT (g++-9)

std::deque results:
   502,088 us resizing time
   348,253 us back growth time
    19,299 us front growth time
   932,011 us arbitrary insertion time
   285,542 us iteration time
   539,452 us cache thrashing time
 1,456,559 us reassignment time
 4,083,208 us total time

std::vector results:
   217,627 us resizing time
   745,331 us back growth time
 6,916,795 us front growth time
 1,108,077 us arbitrary insertion time
   180,263 us iteration time
   449,901 us cache thrashing time
 1,393,661 us reassignment time
11,011,659 us total time

veque results:
   176,656 us resizing time
   600,655 us back growth time
    31,510 us front growth time
   538,941 us arbitrary insertion time
   185,953 us iteration time
   262,112 us cache thrashing time
 1,120,177 us reassignment time
 2,916,006 us total time

 */

#include "include/veque.hpp"
#include <algorithm>
#include <vector>
#include <deque>
#include <chrono>
#include <string>
#include <iostream>
#include <ios>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <string_view>
#include <functional>

struct LargeTrivialObject {

    bool operator==(const LargeTrivialObject & other) const {
        return std::equal(std::begin(data), std::end(data), other.data);
    }
    int data[1024];
};

struct NonTrivialObject {
    std::string data = std::string(1024, 'W');

    bool operator==(const NonTrivialObject & other) const {
        return data == other.data;
    }
};

struct ThrowingMoveConstructObject {
    ThrowingMoveConstructObject() = default;
    ThrowingMoveConstructObject(const ThrowingMoveConstructObject&) = default;

    ThrowingMoveConstructObject(ThrowingMoveConstructObject&&) noexcept(false) {
        throw std::runtime_error("Failed move construction");
    }

    ThrowingMoveConstructObject& operator=(const ThrowingMoveConstructObject&) = default;
    ThrowingMoveConstructObject& operator=(ThrowingMoveConstructObject&&) = default;

    bool operator==(const ThrowingMoveConstructObject & other) const {
        return data == other.data;
    }
    std::string data = std::string(1024, 'X');
};

struct ThrowingMoveAssignObject {
    ThrowingMoveAssignObject() = default;
    ThrowingMoveAssignObject(const ThrowingMoveAssignObject&) = default;
    ThrowingMoveAssignObject(ThrowingMoveAssignObject&&) = default;

    ThrowingMoveAssignObject& operator=(const ThrowingMoveAssignObject&) = default;

    ThrowingMoveAssignObject& operator=(ThrowingMoveAssignObject&&) noexcept(false) {
        throw std::runtime_error("Failed move assignment");
    }

    bool operator==(const ThrowingMoveAssignObject & other) const {
        return data == other.data;
    }
    std::string data = std::string(1024, 'Y');
};

struct ThrowingMoveObject
{
    ThrowingMoveObject() = default;
    ThrowingMoveObject(const ThrowingMoveObject&) = default;

    ThrowingMoveObject(ThrowingMoveObject&&) noexcept(false) {
        throw std::runtime_error("Failed move construction");
    }

    ThrowingMoveObject& operator=(const ThrowingMoveObject&) = default;

    ThrowingMoveObject& operator=(ThrowingMoveObject&&) noexcept(false) {
        throw std::runtime_error("Failed move assignment");
    }

    bool operator==(const ThrowingMoveObject & other) const {
        return data == other.data;
    }
    std::string data = std::string(1024, 'Z');
};

template< typename Container >
int reassignment_test(int i)
{
    // Valgrind doesn't like std::random_device.
    //std::random_device rd;
    //std::mt19937 gen(rd());
    srand(time(NULL));
    for ( int x = 0; x < 3'000; ++x )
    {
        auto v1 = Container( rand() % 100 );
        auto v2 = Container( rand() % 100 );
        auto v3 = Container( rand() % 100 );
        v1 = v2;
        v3 = std::move(v2);
        i += *reinterpret_cast<char*>(&v1);
    }

    for ( int x = 0; x < 3'000; ++x )
    {
        auto v1 = Container( rand() % 100 );
        auto v2 = Container( rand() % 100 );
        auto v3 = Container( rand() % 100 );
        v1.assign( v2.begin(), v2.end() );
        i += *reinterpret_cast<char*>(&v1);
    }
    return i;
}

template< typename Container >
int resizing_test(int i)
{
    using ValType = typename Container::value_type;
    
    for ( int y = 0; y != 100; ++y )
    {
        Container v(5);

        v.resize(15);
        ValType x = v[0];
        i += *reinterpret_cast<char*>(&x);
        v.resize(20);
        x = v[0];
        i += *reinterpret_cast<char*>(&x);
        v.resize(25, ValType{});
        x = v[0];
        i += *reinterpret_cast<char*>(&x);
        v.resize(30);
        x = v[0];
        i += *reinterpret_cast<char*>(&x);
        v.resize(35, ValType{});
        x = v[0];
        i += *reinterpret_cast<char*>(&x);
        v.resize(999);
        x = v[0];
        i += *reinterpret_cast<char*>(&x);
        v.resize(0);
        v.resize(999, ValType{});
        x = v[0];
        i += *reinterpret_cast<char*>(&x);
        v.resize(5);
        x = v[0];
        i += *reinterpret_cast<char*>(&x);
    }
    return i;
}

template< typename Container >
int back_growth_test(int i)
{
    using ValType = typename Container::value_type;
    
    for ( int y = 0; y != 20; ++y )
    {
        {
            typename Container::size_type size = 5;
            Container v(size);

            ValType val{};
            for (int i = 0; i < 2'000; ++i) {
                v.push_back(val);
                ++size;
            }
            while (v.size()) {
                ValType x = v.back();
                v.pop_back();
                --size;
                i += *reinterpret_cast<char*>(&x);
            }
        }

        {
            typename Container::size_type size = 5;
            Container v(size);

            for (int i = 0; i < 2'000; ++i) {
                v.push_back(ValType{});
                ++size;
            }
            while (v.size()) {
                ValType x = v.back();
                v.pop_back();
                --size;
                i += *reinterpret_cast<char*>(&x);
            }
        }

        {
            typename Container::size_type size = 5;
            Container v(size);

            for (int i = 0; i < 2'000; ++i) {
                v.emplace_back();
                ++size;
            }
            while (v.size()) {
                ValType x = v.back();
                v.pop_back();
                --size;
                i += *reinterpret_cast<char*>(&x);
            }
        }
    }
    
    return i;
}

template< typename Container >
int front_growth_test(int i)
{
    using ValType = typename Container::value_type;
    
    {
        typename Container::size_type size = 5;
        Container v(size);

        ValType val{};
        for (int i = 0; i < 2'000; ++i) {
            if constexpr( std::is_same_v<Container, std::vector<ValType>>)
                v.insert(v.begin(), val);
            else
                v.push_front(val);
            ++size;
        }
        while (v.size()) {
            ValType x = v.back();
            v.pop_back();
            i += *reinterpret_cast<char*>(&x);
            --size;
        }
    }


    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 2'000; ++i) {
            if constexpr( std::is_same_v<Container, std::vector < ValType>>)
                v.insert(v.begin(), ValType{});
            else
                v.push_front(ValType{});
            ++size;
        }
        while (v.size()) {
            if constexpr( std::is_same_v<Container, veque<ValType>>)
            {
                ValType x = v.pop_back_element();
                i += *reinterpret_cast<char*>(&x);
            }
            else
            {
                // Closest functionality match
                ValType x = v.back();
                v.pop_back();
                i += *reinterpret_cast<char*>(&x);
            }
        }
    }


    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 2'000; ++i) {
            if constexpr( std::is_same_v<Container, std::vector < ValType>>)
                v.emplace(v.begin());
            else
                v.emplace_front();
            ++size;
        }
        while (v.size()) {
            if constexpr( std::is_same_v<Container, veque<ValType>>)
            {
                ValType x = v.pop_front_element();
                i += *reinterpret_cast<char*>(&x);
            }
            else if constexpr( std::is_same_v<Container, std::vector<ValType>>)
            {
                ValType x = v.front();
                v.erase( v.begin() );
                i += *reinterpret_cast<char*>(&x);
            }
            else
            {
                // Closest functionality match
                ValType x = v.front();
                v.pop_front();
                i += *reinterpret_cast<char*>(&x);
            }
        }
    }
    return i;
}

template< typename Container >
int arbitrary_insertion_test(int i)
{
    using ValType = typename Container::value_type;
    
    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 1'000; ++i) {
            ValType val{};
            v.insert(v.begin(), val);
            ++size;
        }
        while (v.size()) {
            ValType x = v.back();
            v.pop_back();
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }

    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 1'000; ++i) {
            ValType val{};
            v.insert(v.end(), val);
            ++size;
        }
        while (v.size()) {
            ValType x = v.back();
            v.pop_back();
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }

    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 1'000; ++i) {
            ValType val{};
            v.insert(v.begin() + v.size() / 2, val);
            ++size;
        }
        while (v.size()) {
            ValType x = v.back();
            v.pop_back();
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }

    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 1'000; ++i) {
            ValType val{};
            v.insert(v.begin() + v.size() / 3, val);
            ++size;
        }
        while (v.size()) {
            ValType x = v.back();
            v.pop_back();
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }

    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 1'000; ++i) {
            v.insert(v.begin() + 2 * v.size() / 3, {});
            ++size;
        }
        while (v.size()) {
            ValType x = v.front();
            v.erase(v.begin());
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }

    {
        typename Container::size_type size = 5;
        Container v(size);

        // Valgrind doesn't like std::random_device.
        //std::random_device rd;
        //std::mt19937 gen(rd());
        for (int i = 0; i < 1'000; ++i) {
            ValType val{};
            //auto index = std::uniform_int_distribution<>(0, v.size())(gen);
            auto index = rand() % (v.size() + 1);
            //v.insert( v.begin() + dis(gen), val );
            v.insert(v.begin() + index, val);
            ++size;
        }
        while (v.size()) {
            ValType x = v.front();
            v.erase(v.begin());
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }
    return i;
}
 
template< typename Container >
int iteration_test(int i)
{
    using ValType = typename Container::value_type;
    
    Container v(2'000'000);
   
    for (const ValType & val : v)
    {
        i += *reinterpret_cast<const char*>(&val);
    }

    return i;
}

// Sample data, each in increasing comparison order
template<typename T, size_t index> const T val;
template<> const int val<int,0> = 0;
template<> const int val<int,1> = 1;
template<> const int val<int,2> = 2;
template<> const int val<int,3> = 3;
template<> const int val<int,4> = 4;
template<> const std::string val<std::string,0> = std::string(100, 'A');
template<> const std::string val<std::string,1> = std::string(200, 'B');
template<> const std::string val<std::string,2> = std::string(300, 'C');
template<> const std::string val<std::string,3> = std::string(400, 'D');
template<> const std::string val<std::string,4> = std::string(500, 'E');
template<> const double val<double,0> = 00.0;
template<> const double val<double,1> = 11.0;
template<> const double val<double,2> = 22.0;
template<> const double val<double,3> = 33.0;
template<> const double val<double,4> = 44.0;
template<> const std::vector<int> val<std::vector<int>,0> = { 0, 1, 2 };
template<> const std::vector<int> val<std::vector<int>,1> = { 1, 2, 3 };
template<> const std::vector<int> val<std::vector<int>,2> = { 2, 3, 4 };
template<> const std::vector<int> val<std::vector<int>,3> = { 3, 4, 5 };
template<> const std::vector<int> val<std::vector<int>,4> = { 4, 5, 6 };

template< typename Container >
int random_operations_test(int i)
{
    using ValType = typename Container::value_type;
    
    Container veq;

    srand(time(NULL));
    
    auto tests = std::vector<std::function<void()>>{
        [&]
        {
            auto new_size = rand() % 20'000;
            veq.resize(new_size);
            i += *reinterpret_cast<char*>(&veq);
        },
        [&]
        {
            auto new_size = rand() % 10'000;
            veq.resize(new_size);
            i += *reinterpret_cast<char*>(&veq);
        },
        [&]
        {
            auto new_size = rand() % 5'000;
            veq.resize(new_size);
            i += *reinterpret_cast<char*>(&veq);
        },
        [&]
        {
            if ( veq.size() )
            {
                auto index = rand() % veq.size();
                ValType x = veq.at(index);
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                auto index = rand() % veq.size();
                ValType x = veq[index];
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                ValType x = veq.front();
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                ValType x = veq.back();
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            veq.push_back( {} );
        },
        [&]
        {
            ValType item = val<ValType,1>;
            veq.push_back( ValType{item} );
            i += *reinterpret_cast<char*>(&item);
        },
        [&]
        {
            ValType item = val<ValType,4>;
            veq.emplace_back( ValType{item} );
        },
        [&]
        {
            if ( veq.size() )
            {
                ValType item = val<ValType,2>;
                auto index = rand() % veq.size();
                veq.insert( veq.begin() + index, item );
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                ValType item = val<ValType,3>;
                auto index = rand() % veq.size();
                veq.insert( veq.begin() + index, ValType{item} );
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                ValType x = val<ValType,0>;
                auto index = rand() % veq.size();
                veq.emplace( veq.begin() + index );
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                ValType x = *veq.begin();
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                ValType x = *veq.rbegin();
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            veq.clear();
        },
        [&]
        {
            if ( veq.size() )
            {
                if constexpr( std::is_same_v<Container, veque<ValType>> )
                {
                    ValType x = veq.pop_front_element();
                    i += *reinterpret_cast<char*>(&x);
                }
                else if constexpr( std::is_same_v<Container, std::vector<ValType>> )
                {
                    ValType x = veq.front();
                    veq.erase( veq.begin() );
                    i += *reinterpret_cast<char*>(&x);
                }
                else
                {
                    ValType x = veq.front();
                    veq.pop_front();
                    i += *reinterpret_cast<char*>(&x);
                }
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                if constexpr( std::is_same_v<Container, veque<ValType>> )
                {
                    ValType x = veq.pop_back_element();
                    i += *reinterpret_cast<char*>(&x);
                }
                else
                {
                    ValType x = veq.back();
                    veq.pop_back();
                    i += *reinterpret_cast<char*>(&x);
                }
            }
        }
    };

    for ( auto test_counter = 0; test_counter != 30'000; ++test_counter )
    {
        tests[ rand() % tests.size() ]();
    }
    return i;
}

template< template<typename ...Args> typename Container >
int run_resizing_test(int i) {
    i += resizing_test<Container<bool> >(i);
    i += resizing_test<Container<int> >(i);
    i += resizing_test<Container<std::string> >(i);
    i += resizing_test<Container<LargeTrivialObject> >(i);
    i += resizing_test<Container<NonTrivialObject> >(i);
    i += resizing_test<Container<ThrowingMoveConstructObject> >(i);
    i += resizing_test<Container<ThrowingMoveObject> >(i);
    return i;
}

template< template<typename ...Args> typename Container >
int run_reassignment_test(int i) {
    i += reassignment_test<Container<bool> >(i);
    i += reassignment_test<Container<int> >(i);
    i += reassignment_test<Container<std::string> >(i);
    i += reassignment_test<Container<LargeTrivialObject> >(i);
    i += reassignment_test<Container<NonTrivialObject> >(i);
    i += reassignment_test<Container<ThrowingMoveConstructObject> >(i);
    i += reassignment_test<Container<ThrowingMoveObject> >(i);
    return i;
}

template< template<typename ...Args> typename Container >
int run_back_growth_test(int i) {
    i += back_growth_test<Container<bool> >(i);
    i += back_growth_test<Container<int> >(i);
    i += back_growth_test<Container<std::string> >(i);
    i += back_growth_test<Container<LargeTrivialObject> >(i);
    i += back_growth_test<Container<NonTrivialObject> >(i);
    return i;
}

template< template<typename ...Args> typename Container >
int run_front_growth_test(int i) {
    i += front_growth_test<Container<bool> >(i);
    i += front_growth_test<Container<int> >(i);
    i += front_growth_test<Container<std::string> >(i);
    i += front_growth_test<Container<LargeTrivialObject> >(i);
    i += front_growth_test<Container<NonTrivialObject> >(i);
    return i;
}

template< template<typename ...Args> typename Container >
int run_arbitrary_insertion_test(int i) {
    i += arbitrary_insertion_test<Container<bool> >(i);
    i += arbitrary_insertion_test<Container<int> >(i);
    i += arbitrary_insertion_test<Container<std::string> >(i);
    i += arbitrary_insertion_test<Container<LargeTrivialObject> >(i);
    i += arbitrary_insertion_test<Container<NonTrivialObject> >(i);
    return i;
}

template< template<typename ...Args> typename Container >
int run_iteration_test(int i) {
    i += iteration_test<Container<int> >(i);
    i += iteration_test<Container<std::string> >(i);
    i += iteration_test<Container<double> >(i);
    i += iteration_test<Container<std::vector<int>> >(i);
    return i;
}

template< template<typename ...Args> typename Container >
int run_random_operations_test(int i) {
    i += random_operations_test<Container<int> >(i);
    i += random_operations_test<Container<std::string> >(i);
    i += random_operations_test<Container<double> >(i);
    i += random_operations_test<Container<std::vector<int>> >(i);
    return i;
}



template< template<typename ...Args> typename Container >
int test(char i, const char * results_name = nullptr ) {

    static std::array<std::chrono::steady_clock::duration,7> results;

    if ( results_name )
    {
        std::cout.imbue(std::locale(""));
        std::cout << '\n' << results_name << " results:\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[0]).count() << " us resizing time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[1]).count() << " us back growth time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[2]).count() << " us front growth time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[3]).count() << " us arbitrary insertion time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[4]).count() << " us iteration time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[5]).count() << " us cache thrashing time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[6]).count() << " us reassignment time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(std::accumulate(results.begin(), results.end(), std::chrono::steady_clock::duration{})).count() << " us total time\n";
        
    }
    else
    {
        auto t1 = std::chrono::steady_clock::now();

        i += run_resizing_test<Container>((int) i);
        auto t2 = std::chrono::steady_clock::now();
        results[0] += (t2 - t1);

        i += run_back_growth_test<Container>((int) i);
        auto t3 = std::chrono::steady_clock::now();
        results[1] += (t3 - t2);

        i += run_front_growth_test<Container>((int) i);
        auto t4 = std::chrono::steady_clock::now();
        results[2] += (t4 - t3);

        i += run_arbitrary_insertion_test<Container>((int) i);
        auto t5 = std::chrono::steady_clock::now();
        results[3] += (t5 - t4);

        i += run_iteration_test<Container>((int) i);
        auto t6 = std::chrono::steady_clock::now();
        results[4] += (t6 - t5);

        i += run_random_operations_test<Container>((int) i);
        auto t7 = std::chrono::steady_clock::now();
        results[5] += (t7 - t6);

        i += run_reassignment_test<Container>((int) i);
        auto t8 = std::chrono::steady_clock::now();
        results[6] += (t8 - t7);
    }
    
    return i;
}

int main(int argc, char** argv) {
    
    std::cout << "\ntesting std::deque (1 of 3)\n";
    argc += test<std::deque>(argv[0][0]);

    std::cout << "\ntesting std::vector (1 of 3)\n";
    argc += test<std::vector>(argv[0][0]);

    std::cout << "\ntesting veque (1 of 3)\n";
    argc += test<veque>(argv[0][0]);

    std::cout << "\ntesting std::deque (2 of 3)\n";
    argc += test<std::deque>(argv[0][0]);

    std::cout << "\ntesting std::vector (2 of 3)\n";
    argc += test<std::vector>(argv[0][0]);

    std::cout << "\ntesting veque (2 of 3 )\n";
    argc += test<veque>(argv[0][0]);

    std::cout << "\ntesting std::deque (3 of 3)\n";
    argc += test<std::deque>(argv[0][0]);

    std::cout << "\ntesting std::vector (3 of 3)\n";
    argc += test<std::vector>(argv[0][0]);

    std::cout << "\ntesting veque (3 of 3)\n";
    argc += test<veque>(argv[0][0]);

    test<std::deque>(argv[0][0], "std::deque");
    test<std::vector>(argv[0][0], "std::vector");
    test<veque>(argv[0][0], "veque");

    return argc;
}
