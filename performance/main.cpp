/*
 *
 * veque performance test
 *
 * Copyright (C) 2019 Drew Dormann
 *
 * SAMPLE OUTPUT (g++-9)

std::deque results:
     8,216 us resizing time
    20,018 us back growth time
    18,418 us front growth time
   870,060 us arbitrary insertion time
   137,786 us iteration time
   528,697 us cache thrashing time
 1,583,197 us total time

std::vector results:
     3,579 us resizing time
    34,091 us back growth time
 6,288,065 us front growth time
 1,025,089 us arbitrary insertion time
   116,620 us iteration time
   510,204 us cache thrashing time
 7,977,650 us total time

veque results:
     3,142 us resizing time
    31,221 us back growth time
    28,404 us front growth time
   392,247 us arbitrary insertion time
   107,642 us iteration time
   291,009 us cache thrashing time
   853,669 us total time


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
int resizing_test(int i)
{
    Container v(5);

    v.resize(15);
    auto x = v[0];
    i += *reinterpret_cast<char*>(&x);
    v.resize(20);
    x = v[0];
    i += *reinterpret_cast<char*>(&x);
    v.resize(25, {});
    x = v[0];
    i += *reinterpret_cast<char*>(&x);
    v.resize(30);
    x = v[0];
    i += *reinterpret_cast<char*>(&x);
    v.resize(35, {});
    x = v[0];
    i += *reinterpret_cast<char*>(&x);
    v.resize(999);
    v.resize(0);
    v.resize(999, {});
    x = v[0];
    i += *reinterpret_cast<char*>(&x);
    v.resize(5);
    x = v[0];
    i += *reinterpret_cast<char*>(&x);
    return i;
}

template< typename Container >
int back_growth_test(int i)
{
    {
        typename Container::size_type size = 5;
        Container v(size);

        typename Container::value_type val{};
        for (int i = 0; i < 2'000; ++i) {
            v.push_back(val);
            ++size;
        }
        while (v.size()) {
            auto x = v.back();
            v.pop_back();
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }

    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 2'000; ++i) {
            v.push_back(typename Container::value_type{});
            ++size;
        }
        while (v.size()) {
            auto x = v.back();
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
            auto x = v.back();
            v.pop_back();
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }
    return i;
}

template< typename Container >
int front_growth_test(int i) {

    {
        typename Container::size_type size = 5;
        Container v(size);

        typename Container::value_type val{};
        for (int i = 0; i < 2'000; ++i) {
            if constexpr( std::is_same_v<Container, std::vector<typename Container::value_type>>)
                v.insert(v.begin(), val);
            else
                v.push_front(val);
            ++size;
        }
        while (v.size()) {
            auto x = v.back();
            v.pop_back();
            i += *reinterpret_cast<char*>(&x);
            --size;
        }
    }


    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 2'000; ++i) {
            if constexpr( std::is_same_v<Container, std::vector < typename Container::value_type>>)
                v.insert(v.begin(), typename Container::value_type{});
            else
                v.push_front(typename Container::value_type{});
            ++size;
        }
        while (v.size()) {
            if constexpr( std::is_same_v<Container, veque<typename Container::value_type>>)
            {
                auto x = v.pop_back_element();
                i += *reinterpret_cast<char*>(&x);
            }
            else
            {
                // Closest functionality match
                auto x = v.back();
                v.pop_back();
                i += *reinterpret_cast<char*>(&x);
            }
        }
    }


    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 2'000; ++i) {
            if constexpr( std::is_same_v<Container, std::vector < typename Container::value_type>>)
                v.emplace(v.begin());
            else
                v.emplace_front();
            ++size;
        }
        while (v.size()) {
            if constexpr( std::is_same_v<Container, veque<typename Container::value_type>>)
            {
                auto x = v.pop_front_element();
                i += *reinterpret_cast<char*>(&x);
            }
            else if constexpr( std::is_same_v<Container, std::vector<typename Container::value_type>>)
            {
                auto x = v.front();
                v.erase( v.begin() );
                i += *reinterpret_cast<char*>(&x);
            }
            else
            {
                // Closest functionality match
                auto x = v.front();
                v.pop_front();
                i += *reinterpret_cast<char*>(&x);
            }
        }
    }
    return i;
}

template< typename Container >
int arbitrary_insertion_test(int i) {
    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 1'000; ++i) {
            typename Container::value_type val{};
            v.insert(v.begin(), val);
            ++size;
        }
        while (v.size()) {
            auto x = v.back();
            v.pop_back();
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }

    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 1'000; ++i) {
            typename Container::value_type val{};
            v.insert(v.end(), val);
            ++size;
        }
        while (v.size()) {
            auto x = v.back();
            v.pop_back();
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }

    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 1'000; ++i) {
            typename Container::value_type val{};
            v.insert(v.begin() + v.size() / 2, val);
            ++size;
        }
        while (v.size()) {
            auto x = v.back();
            v.pop_back();
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }

    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 1'000; ++i) {
            typename Container::value_type val{};
            v.insert(v.begin() + v.size() / 3, val);
            ++size;
        }
        while (v.size()) {
            auto x = v.back();
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
            auto x = v.front();
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
            typename Container::value_type val{};
            //auto index = std::uniform_int_distribution<>(0, v.size())(gen);
            auto index = rand() % (v.size() + 1);
            //v.insert( v.begin() + dis(gen), val );
            v.insert(v.begin() + index, val);
            ++size;
        }
        while (v.size()) {
            auto x = v.front();
            v.erase(v.begin());
            --size;
            i += *reinterpret_cast<char*>(&x);
        }
    }
    return i;
}
 
// Sample data, each in increasing comparison order
template<typename T> const T sample;
template<> const int sample<int> = 99999999;
template<> const std::string sample<std::string> = std::string(100, 'X');
template<> const double sample<double> = 55.0;
template<> const std::vector<int> sample<std::vector<int>> = { 6, 7, 8, 9, 10, 11, 12 };


template< typename Container >
int iteration_test(int i)
{
    Container v;

    for ( i = 0; i != 1'000; ++i )
    {
        v.insert( v.end(), 2'000, sample<typename Container::value_type>);
    }
    
    for (auto && val : v)
    {
        i += *reinterpret_cast<char*>(&val);
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
                auto x = veq.at(index);
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                auto index = rand() % veq.size();
                auto x = veq[index];
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                auto x = veq.front();
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                auto x = veq.back();
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            veq.push_back( {} );
        },
        [&]
        {
            auto item = val<typename Container::value_type,1>;
            veq.push_back( typename Container::value_type{item} );
            i += *reinterpret_cast<char*>(&item);
        },
        [&]
        {
            auto item = val<typename Container::value_type,4>;
            veq.emplace_back( typename Container::value_type{item} );
        },
        [&]
        {
            if ( veq.size() )
            {
                auto item = val<typename Container::value_type,2>;
                auto index = rand() % veq.size();
                veq.insert( veq.begin() + index, item );
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                auto item = val<typename Container::value_type,3>;
                auto index = rand() % veq.size();
                veq.insert( veq.begin() + index, typename Container::value_type{item} );
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                auto x = val<typename Container::value_type,0>;
                auto index = rand() % veq.size();
                veq.emplace( veq.begin() + index );
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                auto x = *veq.begin();
                i += *reinterpret_cast<char*>(&x);
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                auto x = *veq.rbegin();
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
                if constexpr( std::is_same_v<Container, veque<typename Container::value_type>> )
                {
                    auto x = veq.pop_front_element();
                    i += *reinterpret_cast<char*>(&x);
                }
                else if constexpr( std::is_same_v<Container, std::vector<typename Container::value_type>> )
                {
                    auto x = veq.front();
                    veq.erase( veq.begin() );
                    i += *reinterpret_cast<char*>(&x);
                }
                else
                {
                    auto x = veq.front();
                    veq.pop_front();
                    i += *reinterpret_cast<char*>(&x);
                }
            }
        },
        [&]
        {
            if ( veq.size() )
            {
                if constexpr( std::is_same_v<Container, veque<typename Container::value_type>> )
                {
                    auto x = veq.pop_back_element();
                    i += *reinterpret_cast<char*>(&x);
                }
                else
                {
                    auto x = veq.back();
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

    static std::array<std::chrono::steady_clock::duration,6> results;

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
