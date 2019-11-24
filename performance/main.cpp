/*
 *
 * veque performance test
 *
 * Copyright (C) 2019 Drew Dormann
 *
 * SAMPLE OUTPUT:

testing std::deque
    6052 us resizing_test time
   14431 us back_growth_test time
   12193 us front_growth_test time
  410732 us arbitrary_insertion_test time
   59103 us iteration_test time
  502512 us total time

testing std::vector
    1914 us resizing_test time
   19277 us back_growth_test time
 1377079 us front_growth_test time
  583795 us arbitrary_insertion_test time
   37729 us iteration_test time
 2019797 us total time

testing veque
     847 us resizing_test time
   17490 us back_growth_test time
    9755 us front_growth_test time
  174511 us arbitrary_insertion_test time
   28383 us iteration_test time
  230987 us total time


 */

#include "include/veque.hpp"
#include <vector>
#include <deque>
#include <chrono>
#include <string>
#include <iostream>
#include <ios>
#include <iostream>
#include <iomanip>
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

    v.resize(999);

    v.resize(0);

    v.resize(999);

    v.resize(5);

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
            if constexpr( std::is_same_v<Container, std::vector < typename Container::value_type>>)
                v.insert(v.begin(), val);
            else
                v.push_front(val);
            ++size;
        }
        while (v.size()) {
            v.pop_back();
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
            v.pop_back();
            --size;
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
            v.pop_back();
            --size;
        }
    }
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
        for (auto c = reinterpret_cast<char*>(&val); c != reinterpret_cast<char*>(&val) + sizeof(val); ++c ) {
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
int test(char i) {

    static auto locale = std::locale("");
    std::cout.imbue(locale);

    auto t1 = std::chrono::steady_clock::now();

    i += run_resizing_test<Container>((int) i);
    auto t2 = std::chrono::steady_clock::now();
    std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << " us resizing_test time\n";

    i += run_back_growth_test<Container>((int) i);
    auto t3 = std::chrono::steady_clock::now();
    std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count() << " us back_growth_test time\n";

    i += run_front_growth_test<Container>((int) i);
    auto t4 = std::chrono::steady_clock::now();
    std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count() << " us front_growth_test time\n";

    i += run_arbitrary_insertion_test<Container>((int) i);
    auto t5 = std::chrono::steady_clock::now();
    std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(t5 - t4).count() << " us arbitrary_insertion_test time\n";

    i += run_iteration_test<Container>((int) i);
    auto t6 = std::chrono::steady_clock::now();
    std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(t6 - t5).count() << " us iteration_test time\n";

    std::cout << std::setw(10) << std::right << std::chrono::duration_cast<std::chrono::microseconds>(t6 - t1).count() << " us total time\n";
    return i;
}

int main(int argc, char** argv) {
    std::cout << "\ntesting std::deque\n";
    argc += test<std::deque>(argv[0][0]);

    std::cout << "\ntesting std::vector\n";
    argc += test<std::vector>(argv[0][0]);

    std::cout << "\ntesting veque\n";
    argc += test<veque>(argv[0][0]);

    return argc;
}
