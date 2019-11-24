/*
 *
 * veque performance test
 *
 * Copyright (C) 2019 Drew Dormann
 *
 * SAMPLE OUTPUT:


testing std::deque (1 of 3)

testing std::vector (1 of 3)

testing veque (1 of 3)

testing std::deque (2 of 3)

testing std::vector (2 of 3)

testing veque (2 of 3 )

testing std::deque (3 of 3)

std::deque results:
     7,016 us resizing_test time
    18,055 us back_growth_test time
    16,196 us front_growth_test time
   921,443 us arbitrary_insertion_test time
   148,111 us iteration_test time
 1,110,823 us total time

testing std::vector (3 of 3)

std::vector results:
     3,525 us resizing_test time
    34,987 us back_growth_test time
 3,959,486 us front_growth_test time
 1,080,113 us arbitrary_insertion_test time
   120,441 us iteration_test time
 5,198,554 us total time

testing veque (3 of 3)

veque results:
     2,786 us resizing_test time
    32,667 us back_growth_test time
    30,277 us front_growth_test time
   419,043 us arbitrary_insertion_test time
   114,309 us iteration_test time
   599,085 us total time



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

struct ThrowingMoveObject {
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
int resizing_test(int i) {
    Container v(5);

    v.resize(15);
    
    auto a = v[0];
    auto b = v[14];

    v.resize(999);

    auto c = v[0];
    auto d = v[998];
    
    v.resize(0);

    v.resize(999);

    v.resize(5);

    auto e = v[0];
    auto f = v[4];
    
    v.shrink_to_fit();
}

template< typename Container >
int back_growth_test(int i) {
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
        }
    }
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
                auto x = v.pop_back_instance();
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
                auto x = v.pop_front_instance();
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
        }
    }

    {
        typename Container::size_type size = 5;
        Container v(size);

        for (int i = 0; i < 1'000; ++i) {
            typename Container::value_type val{};
            v.insert(v.begin() + 2 * v.size() / 3, {});
            ++size;
        }
        while (v.size()) {
            auto x = v.front();
            v.erase(v.begin());
            --size;
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
        }
    }
}

// Sample data, each in increasing comparison order
template<typename T> const T sample;
template<> const int sample<int> = 99999999;
template<> const std::string sample<std::string> = std::string(100, 'X');
template<> const double sample<double> = 55.0;
template<> const std::vector<int> sample<std::vector<int>> = { 6, 7, 8, 9, 10, 11, 12 };


template< typename Container >
int iteration_test(int i) {
    Container v{};
    for ( int c = 0; c < 200'000; ++c )
    {
        v.push_back( sample<typename Container::value_type> );
    }

    for (auto && val : v) {
        auto x = val;
        
        for (auto c = reinterpret_cast<char*>(&x); c != reinterpret_cast<char*>(&x) + sizeof(x); ++c ) {
            i += *c;
        }
    }

    return i;
}

template< template<typename ...Args> typename Container >
int run_resizing_test(int i) {
    i += resizing_test < Container<bool> >(i);
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
    i += back_growth_test < Container<bool> >(i);
    i += back_growth_test<Container<int> >(i);
    i += back_growth_test<Container<std::string> >(i);
    i += back_growth_test<Container<LargeTrivialObject> >(i);
    i += back_growth_test<Container<NonTrivialObject> >(i);
    return i;
}

template< template<typename ...Args> typename Container >
int run_front_growth_test(int i) {
    i += front_growth_test < Container<bool> >(i);
    i += front_growth_test<Container<int> >(i);
    i += front_growth_test<Container<std::string> >(i);
    i += front_growth_test<Container<LargeTrivialObject> >(i);
    i += front_growth_test<Container<NonTrivialObject> >(i);
    return i;
}

template< template<typename ...Args> typename Container >
int run_arbitrary_insertion_test(int i) {
    i += arbitrary_insertion_test < Container<bool> >(i);
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
int test(char i, const char * results_name = nullptr ) {

    static std::array<std::chrono::steady_clock::duration,5> results;

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

    
    if ( results_name )
    {
        std::cout.imbue(std::locale(""));
        std::cout << '\n' << results_name << " results:\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[0]).count() << " us resizing_test time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[1]).count() << " us back_growth_test time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[2]).count() << " us front_growth_test time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[3]).count() << " us arbitrary_insertion_test time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(results[4]).count() << " us iteration_test time\n";
        std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(std::accumulate(results.begin(), results.end(), std::chrono::steady_clock::duration{})).count() << " us total time\n";
        
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
    argc += test<std::deque>(argv[0][0], "std::deque");

    std::cout << "\ntesting std::vector (3 of 3)\n";
    argc += test<std::vector>(argv[0][0], "std::vector");

    std::cout << "\ntesting veque (3 of 3)\n";
    argc += test<veque>(argv[0][0], "veque");

    return argc;
}
