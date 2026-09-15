/*
 * veque.hpp
 *
 * Efficient generic C++ container - really good for fast. small queues -
 * has all the front() and back() functionality of std::deque, but elements are stored contiguously, like std::vector.
 *
 * This container is smart enough to retain its storage if, e.g., push_back() and pop_front() are occurring
 * in roughly equal number.
 *
 * Copyright (C) 2019-2026 Drew Dormann
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 */

#ifndef VEQUE_HEADER_GUARD
#define VEQUE_HEADER_GUARD

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <limits>
#include <ranges>
#include <ratio>
#include <string>
#include <type_traits>
#include <stdexcept>
#include <utility>

namespace veque
{
    // Very fast resizing behavior
    struct fast_resize_traits
    {
        // Relative to size(), amount of unused space to reserve when reallocating
        using allocation_before_front = std::ratio<1>;
        using allocation_after_back = std::ratio<1>;

        // If true, arbitrary insert and erase operations are twice the speed of
        // std::vector, but those operations invalidate all iterators
        static constexpr auto resize_from_closest_side = true;
    };

    // Match std::vector iterator invalidation rules - only invalidate elements *after* a change
    struct vector_compatible_resize_traits
    {
        // Relative to size(), amount of unused space to reserve when reallocating
        using allocation_before_front = std::ratio<1>;
        using allocation_after_back = std::ratio<1>;

        // If false, veque is a 100% compatible drop-in replacement for
        // std::vector including iterator invalidation rules
        static constexpr auto resize_from_closest_side = false;
    };

    // Resizing behavior resembling std::vector.  Also ideal for queue-like push_back/pop_front behavior.
    struct std_vector_traits
    {
        // Reserve storage only at back, like std::vector
        using allocation_before_front = std::ratio<0>;
        using allocation_after_back = std::ratio<1>;

        // Same iterator invalidation rules as std::vector
        static constexpr auto resize_from_closest_side = false;
    };

    // Never reallocate more storage than is needed
    struct no_reserve_traits
    {
        // Any operation requiring a greater size reserves only that size
        using allocation_before_front = std::ratio<0>;
        using allocation_after_back = std::ratio<0>;

        // Same iterator invalidation rules as std::vector
        static constexpr auto resize_from_closest_side = false;
    };

    // Only a std::ratio deduces to itself through CTAD.
    template< typename T >
    concept is_ratio = requires( T x ) { { std::ratio{x} } -> std::same_as<std::remove_cv_t<T>>; };

    // The requirements on a ResizeTraits: how much unused storage to reserve
    // at each end, and whether arbitrary insert and erase may shift elements
    // toward the closer end.
    //
    // The allocations must be std::ratio instantiations, not merely
    // ratio-shaped: std::ratio arithmetic static_asserts on anything else, so
    // a duck-typed num/den pair fails deep inside <ratio> rather than here.
    // Sign and denominator are validated separately, within veque itself.
    template< typename Traits >
    concept resize_traits = requires
    {
        requires is_ratio< typename Traits::allocation_before_front >;
        requires is_ratio< typename Traits::allocation_after_back >;
        { Traits::resize_from_closest_side } -> std::convertible_to<bool>;
    };

    // A range whose elements can be used to build a T.  Mirrors the
    // standard's exposition-only container-compatible-range.
    template< typename R, typename T >
    concept container_compatible_range =
        std::ranges::input_range<R>
        && std::convertible_to< std::ranges::range_reference_t<R>, T >;

    template< typename T, resize_traits ResizeTraits = fast_resize_traits, typename Allocator = std::allocator<T> >
    class veque
    {
    public:
        
        // Types
        using allocator_type = Allocator;
        using alloc_traits = std::allocator_traits<allocator_type>;
        using value_type = T;
        using reference = T &;
        using const_reference = const T &;
        using pointer = T *;
        using const_pointer = const T *;
        using iterator = T *;
        using const_iterator = const T *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using difference_type = std::ptrdiff_t;
        using size_type = std::size_t;
        using ssize_type = std::ptrdiff_t;

        // Common member functions
        constexpr veque() noexcept ( noexcept(Allocator()) )
            : veque( Allocator() )
        {
        }

        explicit constexpr veque( const Allocator& alloc ) noexcept
            : _data { 0, alloc }
        {
        }

        explicit constexpr veque( size_type n, const Allocator& alloc = Allocator() )
            : veque( _allocate_uninitialized_tag{}, n, alloc )
        {
            _value_construct_range( begin(), end() );
        }

        constexpr veque( size_type n, const T &value, const Allocator& alloc = Allocator() )
            : veque( _allocate_uninitialized_tag{}, n, alloc )
        {
            _value_construct_range( begin(), end(), value );
        }

        template< std::input_iterator InputIt >
        constexpr veque( InputIt b,  InputIt e, const Allocator& alloc = Allocator() )
            : veque( b, e, alloc, _pass_tag_for<InputIt>{} )
        {
        }

        constexpr veque( std::initializer_list<T> lst, const Allocator& alloc = Allocator() )
            : veque( _allocate_uninitialized_tag{}, lst.size(), alloc )
        {
            _copy_construct_range( lst.begin(), lst.end(), begin() );
        }

#ifdef __cpp_lib_containers_ranges
        // Unlike the iterator pair, this accepts a range whose end() is a
        // sentinel of a different type -- which is most pipelines ending in
        // views::take or views::filter.  std::from_range_t is C++23; the
        // range members below need only C++20 and are always available.
        template< container_compatible_range<T> R >
        constexpr veque( std::from_range_t, R && rg, const Allocator & alloc = Allocator() )
            : veque( alloc )
        {
            append_range( std::forward<R>(rg) );
        }
#endif

        constexpr veque( const veque & other )
            : veque( _allocate_uninitialized_tag{}, other.size(), alloc_traits::select_on_container_copy_construction( other._allocator() ) )
        {
            _copy_construct_range( other.begin(), other.end(), begin() );
        }

        template< typename OtherResizeTraits >
        constexpr veque( const veque<T,OtherResizeTraits,Allocator> & other )
            : veque( _allocate_uninitialized_tag{}, other.size(), alloc_traits::select_on_container_copy_construction( other._allocator() ) )
        {
            _copy_construct_range( other.begin(), other.end(), begin() );
        }

        template< typename OtherResizeTraits >
        constexpr veque( const veque<T,OtherResizeTraits,Allocator> & other, const Allocator & alloc )
            : veque( _allocate_uninitialized_tag{}, other.size(), alloc )
        {
            _copy_construct_range( other.begin(), other.end(), begin() );
        }

        constexpr veque( veque && other ) noexcept
        {
            _swap_with_allocator( std::move(other) );
        }

        template< typename OtherResizeTraits >
        constexpr veque( veque<T,OtherResizeTraits,Allocator> && other ) noexcept
        {
            _swap_with_allocator( std::move(other) );
        }

        template< typename OtherResizeTraits >
        constexpr veque( veque<T,OtherResizeTraits,Allocator> && other, const Allocator & alloc )
            : veque( alloc )
        {
            if constexpr ( !alloc_traits::is_always_equal::value )
            {
                if ( alloc != other._allocator() )
                {
                    // Incompatible allocators.  Allocate new storage.
                    auto replacement = veque( _allocate_uninitialized_tag{}, other.size(), alloc );
                    _nothrow_move_construct_range( other.begin(), other.end(), replacement.begin() );
                    _swap_without_allocator( std::move(replacement) );
                    return;
                }
            }
            _swap_without_allocator( std::move(other) );
        }

        constexpr ~veque()
        {
            _destroy( begin(), end() );
        }

        constexpr veque & operator=( const veque & other )
        {
            return _copy_assignment( other );
        }

        template< typename OtherResizeTraits >
        constexpr veque & operator=( const veque<T,OtherResizeTraits,Allocator> & other )
        {
            return _copy_assignment( other );
        }

        constexpr veque & operator=( veque && other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_move_assignment::value
            || alloc_traits::is_always_equal::value) )
        {
            return _move_assignment( std::move(other) );
        }

        template< typename OtherResizeTraits >
        constexpr veque & operator=( veque<T,OtherResizeTraits,Allocator> && other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_move_assignment::value
            || alloc_traits::is_always_equal::value) )
        {
            return _move_assignment( std::move(other) );
        }

        constexpr veque & operator=( std::initializer_list<T> lst )
        {
            _assign( lst.begin(), lst.end() );
            return *this;
        }

        constexpr void assign( size_type count, const T &value )
        {
            if ( count > capacity_full() )
            {
                _swap_without_allocator( veque( count, value, _allocator() ) );
            }
            else
            {
                _reassign_existing_storage( count, value );
            }
        }

        template< std::input_iterator InputIt >
        constexpr void assign( InputIt b, InputIt e )
        {
            _assign( b, e, _pass_tag_for<InputIt>{} );
        }

        constexpr void assign( std::initializer_list<T> lst )
        {
            _assign( lst.begin(), lst.end() );
        }

        template< container_compatible_range<T> R >
        constexpr void assign_range( R && rg )
        {
            if constexpr ( std::ranges::forward_range<R> )
            {
                // views::common makes begin() and end() the same type, so a
                // sentinel range can reuse the existing storage too.
                auto common = std::views::common( std::forward<R>(rg) );
                _assign( std::ranges::begin(common), std::ranges::end(common) );
            }
            else
            {
                clear();
                append_range( std::forward<R>(rg) );
            }
        }

        constexpr allocator_type get_allocator() const
        {
            return _allocator();
        }

        // Element access
        constexpr reference at( size_type idx )
        {
            if ( idx >= size() )
            {
                throw std::out_of_range("veque<T,ResizeTraits,Alloc>::at(" + std::to_string(idx) + ") out of range");
            }
            return (*this)[idx];
        }

        constexpr const_reference at( size_type idx ) const
        {
            if ( idx >= size() )
            {
                throw std::out_of_range("veque<T,ResizeTraits,Alloc>::at(" + std::to_string(idx) + ") out of range");
            }
            return (*this)[idx];
        }

        constexpr reference operator[]( size_type idx )
        {
            return *(begin() + idx);
        }

        constexpr const_reference operator[]( size_type idx ) const
        {
            return *(begin() + idx);
        }

        constexpr reference front()
        {
            return (*this)[0];
        }

        constexpr const_reference front() const
        {
            return (*this)[0];
        }

        constexpr reference back()
        {
            return (*this)[size() - 1];
        }

        constexpr const_reference back() const
        {
            return (*this)[size() - 1];
        }

        constexpr T * data() noexcept
        {
            return begin();
        }

        constexpr const T * data() const noexcept
        {
            return begin();
        }

        // Iterators
        constexpr const_iterator cbegin() const noexcept
        {
            return _storage_begin() + _offset;
        }

        constexpr iterator begin() noexcept
        {
            return _storage_begin() + _offset;
        }

        constexpr const_iterator begin() const noexcept
        {
            return cbegin();
        }

        constexpr const_iterator cend() const noexcept
        {
            return _storage_begin() + _offset + size();
        }

        constexpr iterator end() noexcept
        {
            return _storage_begin() + _offset + size();
        }

        constexpr const_iterator end() const noexcept
        {
            return cend();
        }

        constexpr const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(cend());
        }

        constexpr reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        constexpr const_reverse_iterator rbegin() const noexcept
        {
            return crbegin();
        }

        constexpr const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(cbegin());
        }

        constexpr reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        constexpr const_reverse_iterator rend() const noexcept
        {
            return crend();
        }

        // Capacity
        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return size() == 0;
        }

        constexpr size_type size() const noexcept
        {
            return _size;
        }

        constexpr ssize_type ssize() const noexcept
        {
            return _size;
        }

        constexpr size_type max_size() const noexcept
        {
            constexpr auto compile_time_limit = std::min(
                // The ssize type's ceiling
                std::numeric_limits<ssize_type>::max() / sizeof(T),
                // Ceiling imposed by std::ratio math
                std::numeric_limits<size_type>::max() / _full_realloc::num
            );

            // The allocator's ceiling
            auto runtime_limit = alloc_traits::max_size(_allocator() );

            return std::min( compile_time_limit, runtime_limit );
        }

        // Reserve front and back capacity, in one operation.
        constexpr void reserve( size_type front, size_type back )
        {
            if ( front > capacity_front() || back > capacity_back() )
            {
                auto allocated_before_begin = std::max( capacity_front(), front ) - size();
                auto allocated_after_begin = std::max( capacity_back(), back );
                auto new_full_capacity = allocated_before_begin + allocated_after_begin;
                
                if ( new_full_capacity > max_size() )
                {
                    throw std::length_error("veque<T,ResizeTraits,Alloc>::reserve(" + std::to_string(front) + ", " + std::to_string(back) + ") exceeds max_size()");
                }
                _reallocate( new_full_capacity, allocated_before_begin );
            }
        }

        constexpr void reserve_front( size_type count )
        {
            reserve( count, 0 );
        }

        constexpr void reserve_back( size_type count )
        {
            reserve( 0, count );
        }

        constexpr void reserve( size_type count )
        {
            reserve( count, count );
        }

        // Returns current size + unused allocated storage before front()
        constexpr size_type capacity_front() const noexcept
        {
            return _offset + size();
        }

        // Returns current size + unused allocated storage after back()
        constexpr size_type capacity_back() const noexcept
        {
            return capacity_full() - _offset;
        }

        // Returns current size + all unused allocated storage
        constexpr size_type capacity_full() const noexcept
        {
            return _data._allocated;
        }

        // To achieve interface parity with std::vector, capacity() returns capacity_back();
        constexpr size_type capacity() const noexcept
        {
            return capacity_back();
        }

        constexpr void shrink_to_fit()
        {
            if ( size() < capacity_full() )
            {
                _reallocate( size(), 0 );
            }
        }

        // Modifiers
        constexpr void clear() noexcept
        {
            _destroy( begin(), end() );
            _size = 0;
            _offset = 0;
            if constexpr ( std::ratio_greater_v<_unused_realloc, std::ratio<0>> )
            {
                using unused_front_ratio = std::ratio_divide<_front_realloc,_unused_realloc>;
                _offset = capacity_full() * unused_front_ratio::num / unused_front_ratio::den;
            }
        }

        constexpr iterator insert( const_iterator it, const T & value )
        {
            return emplace( it, value );
        }

        constexpr iterator insert( const_iterator it, T && value )
        {
            return emplace( it, std::move(value) );
        }

        constexpr iterator insert( const_iterator it, size_type count, const T & value )
        {
            auto res = _insert_storage( it, count );
            _value_construct_range( res, res + count, value );
            return res;
        }

        template< std::input_iterator InputIt >
        constexpr iterator insert( const_iterator it, InputIt b, InputIt e )
        {
            return _insert( it, b, e, _pass_tag_for<InputIt>{} );
        }

        constexpr iterator insert( const_iterator it, std::initializer_list<T> lst )
        {
            return insert( it, lst.begin(), lst.end() );
        }

        template< container_compatible_range<T> R >
        constexpr iterator insert_range( const_iterator it, R && rg )
        {
            if constexpr ( std::ranges::forward_range<R> )
            {
                // views::common makes begin() and end() the same type, so a
                // sentinel-terminated range can reuse the sized insert path.
                auto common = std::views::common( std::forward<R>(rg) );
                return _insert( it, std::ranges::begin(common), std::ranges::end(common) );
            }
            else
            {
                // Single-pass: the length cannot be known before consuming the
                // range, so collect it before disturbing this veque.
                veque collected( _allocator() );
                collected.append_range( std::forward<R>(rg) );
                return _insert( it, collected.begin(), collected.end() );
            }
        }

        template< typename ...Args >
        constexpr iterator emplace( const_iterator it, Args && ... args )
        {
            auto res = _insert_storage( it, 1 );
            alloc_traits::construct( _allocator(), res, std::forward<Args>(args)... );
            return res;
        }

        constexpr iterator erase( const_iterator it )
        {
            return erase( it, std::next(it) );
        }

        constexpr iterator erase( const_iterator b, const_iterator e )
        {
            auto count = std::distance( b, e );
            if constexpr ( _resize_from_closest_side )
            {
                auto elements_before = std::distance( cbegin(), b );
                auto elements_after = std::distance( e, cend( ) );
                if (  elements_before < elements_after )
                {
                    _shift_back( begin(), b, count );
                    _move_begin( count );
                    return _mutable_iterator(e);
                }
            }
            _shift_front( e, end(), count );
            _move_end(-count);
            return _mutable_iterator(b);
        }

        constexpr void push_back( const T & value )
        {
            emplace_back( value );
        }

        constexpr void push_back( T && value )
        {
            emplace_back( std::move(value) );
        }

        template< typename ... Args>
        constexpr reference emplace_back( Args && ...args )
        {
            if ( size() == capacity_back() )
            {
                _reallocate_space_at_back( size() + 1 );
            }
            alloc_traits::construct( _allocator(), end(), std::forward<Args>(args)... );
            _move_end( 1 );
            return back();
        }

        constexpr void push_front( const T & value )
        {
            emplace_front( value );
        }

        constexpr void push_front( T && value )
        {
            emplace_front( std::move(value) );
        }

        template< typename ... Args>
        constexpr reference emplace_front( Args && ...args )
        {
            if ( size() == capacity_front() )
            {
                _reallocate_space_at_front( size() + 1 );
            }
            alloc_traits::construct( _allocator(), begin()-1, std::forward<Args>(args)... );
            _move_begin( -1 );
            return front();
        }

        template< container_compatible_range<T> R >
        constexpr void append_range( R && rg )
        {
            if constexpr ( std::ranges::forward_range<R> )
            {
                insert_range( end(), std::forward<R>(rg) );
            }
            else
            {
                if constexpr ( std::ranges::sized_range<R> )
                {
                    reserve_back( size() + static_cast<size_type>( std::ranges::size(rg) ) );
                }
                for ( auto && element : rg )
                {
                    emplace_back( std::forward<decltype(element)>(element) );
                }
            }
        }

        // std::vector has no counterpart: prepending to it shifts every
        // element.  Inserting at begin() here shifts into the storage veque
        // already reserves at the front, which is the point of the container.
        template< container_compatible_range<T> R >
        constexpr void prepend_range( R && rg )
        {
            insert_range( begin(), std::forward<R>(rg) );
        }

        constexpr void pop_back()
        {
            alloc_traits::destroy( _allocator(), &back() );
            _move_end( -1 );
        }

        // Move-savvy pop back with strong exception guarantee
        constexpr T pop_back_element()
        {
            auto res( _nothrow_construct_move(back()) );
            pop_back();
            return res;
        }

        constexpr void pop_front()
        {
            alloc_traits::destroy( _allocator(), &front() );
            _move_begin( 1 );
        }

        // Move-savvy pop front with strong exception guarantee
        constexpr T pop_front_element()
        {
            auto res( _nothrow_construct_move(front()) );
            pop_front();
            return res;
        }

        // Resizes the veque, by adding or removing from the front. 
        constexpr void resize_front( size_type count )
        {
            _resize_front( count );
        }

        constexpr void resize_front( size_type count, const T & value )
        {
            _resize_front( count, value );
        }

        // Resizes the veque, by adding or removing from the back.
        constexpr void resize_back( size_type count )
        {
            _resize_back( count );
        }

        constexpr void resize_back( size_type count, const T & value )
        {
            _resize_back( count, value );
        }

        // To achieve interface parity with std::vector, resize() performs resize_back();
        constexpr void resize( size_type count )
        {
            _resize_back( count );
        }

        constexpr void resize( size_type count, const T & value )
        {
            _resize_back( count, value );
        }

        template< typename OtherResizeTraits >
        constexpr void swap( veque<T,OtherResizeTraits,Allocator> & other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_swap::value
            || alloc_traits::is_always_equal::value))
        {
            if constexpr ( alloc_traits::propagate_on_container_swap::value )
            {
                _swap_with_allocator( std::move(other) );
            }
            else
            {
                if ( _allocator() == other._allocator() )
                {
                    _swap_without_allocator( std::move(other) );
                }
                else
                {
                    // std::vector would declare this UB.  Allocate compatible storage and make it work.
                    auto new_this = veque( _allocate_uninitialized_tag{}, other.size(), _allocator() );
                    _nothrow_move_construct_range( other.begin(), other.end(), new_this.begin() );

                    auto new_other = veque( _allocate_uninitialized_tag{}, size(), other._allocator() );
                    _nothrow_move_construct_range( begin(), end(), new_other.begin() );

                    _swap_without_allocator( std::move(new_this) );
                    other._swap_without_allocator( std::move(new_other) );
                }
            }
        }

    private:

        // Whether a range can be traversed more than once -- so its length can
        // be measured up front and the storage allocated exactly once.
        //
        // This asks std::forward_iterator rather than reading the legacy
        // iterator_category, because the two disagree for C++20 range
        // iterators that yield prvalues: views::iota, views::transform with a
        // by-value function, views::zip and views::enumerate are all
        // random-access by concept while reporting input_iterator_tag.
        // Dispatching on the tag sent those down the single-pass path and
        // reallocated on every growth step.
        struct _single_pass_tag {};
        struct _multi_pass_tag {};

        template< typename It >
        using _pass_tag_for = std::conditional_t< std::forward_iterator<It>,
                                                  _multi_pass_tag,
                                                  _single_pass_tag >;

        // Every veque instantiation is a friend, so that operations between
        // veques with differing ResizeTraits can access each other's internals.
        template< typename OtherT, resize_traits OtherResizeTraits, typename OtherAllocator >
        friend class veque;

        using _front_realloc = typename ResizeTraits::allocation_before_front::type;
        using _back_realloc = typename ResizeTraits::allocation_after_back::type;
        using _unused_realloc = std::ratio_add< _front_realloc, _back_realloc >;
        using _full_realloc = std::ratio_add< std::ratio<1>, _unused_realloc >;

        static constexpr auto _resize_from_closest_side = ResizeTraits::resize_from_closest_side;

        static_assert( _front_realloc::den > 0  );
        static_assert( _back_realloc::den > 0  );
        static_assert( std::ratio_greater_equal_v<_front_realloc,std::ratio<0>>, "Reserving negative space is not well-defined" );
        static_assert( std::ratio_greater_equal_v<_back_realloc,std::ratio<0>>, "Reserving negative space is not well-defined" );
        static_assert( std::ratio_greater_equal_v<_unused_realloc,std::ratio<0>>, "Reserving negative space is not well-defined" );

        // Confirmation that allocator_traits will only directly call placement new(ptr)T()
        static constexpr auto _calls_default_constructor_directly = 
            std::is_same_v<allocator_type,std::allocator<T>>;
        // Confirmation that allocator_traits will only directly call placement new(ptr)T(const T&)
        static constexpr auto _calls_copy_constructor_directly = 
            std::is_same_v<allocator_type,std::allocator<T>>;
        // Confirmation that allocator_traits will only directly call ~T()
        static constexpr auto _calls_destructor_directly =
            std::is_same_v<allocator_type,std::allocator<T>>;

        size_type _size = 0;    // Number of elements in use
        size_type _offset = 0;  // Number of uninitialized elements before begin()

        // Deriving from allocator to leverage empty base optimization
        struct Data : Allocator
        {
            T *_storage = nullptr;
            size_type _allocated = 0;

            Data() = default;
            constexpr Data( size_type size, const Allocator & alloc )
                : Allocator{alloc}
                , _storage{size ? std::allocator_traits<Allocator>::allocate( allocator(), size ) : nullptr}
                , _allocated{size}
            {
            }
            Data( const Data& ) = delete;
            constexpr Data( Data && other )
            {
                *this = std::move(other);
            }
            constexpr ~Data()
            {
                if ( _storage )
                {
                    std::allocator_traits<Allocator>::deallocate( allocator(), _storage, _allocated );
                }
            }
            Data& operator=( const Data & ) = delete;
            constexpr Data& operator=( Data && other )
            {
                using std::swap;
                if constexpr( ! std::is_empty_v<Allocator> )
                {
                    swap(allocator(), other.allocator());
                }
                swap(_allocated,  other._allocated);
                swap(_storage,    other._storage);
                return *this;
            }
            constexpr Allocator& allocator() { return *this; }
            constexpr const Allocator& allocator() const { return *this; }
        } _data;

        template< typename InputIt >
        constexpr veque( InputIt b, InputIt e, const Allocator & alloc, _single_pass_tag )
            : veque{alloc}
        {
            for ( ; b != e; ++b )
            {
                push_back( *b );
            }
        }

        template< typename InputIt >
        constexpr veque( InputIt b, InputIt e, const Allocator & alloc, _multi_pass_tag )
            : veque( _allocate_uninitialized_tag{}, std::ranges::distance( b, e ), alloc )
        {
            _copy_construct_range( b, e, begin() );
        }

        // Private tag to indicate initial allocation
        struct _allocate_uninitialized_tag {};
        // Private tag to indicate resizing allocation
        struct _reallocate_uninitialized_tag {};

        // Create an uninitialized empty veque, with specified storage params
        constexpr veque( _allocate_uninitialized_tag, size_type size, size_type allocated, size_type offset, const Allocator & alloc )
            : _size{ size }
            , _offset{ offset }
            , _data { allocated, alloc }
        {
        }

        // Create an uninitialized empty veque, with storage for expected size
        constexpr veque( _allocate_uninitialized_tag, size_type size, const Allocator & alloc )
            : veque( _allocate_uninitialized_tag{}, size, size, 0, alloc )
        {
        }

        // Create an uninitialized empty veque, with storage for expected reallocated size
        constexpr veque( _reallocate_uninitialized_tag, size_type size, const Allocator & alloc )
            : veque( _allocate_uninitialized_tag{}, size, _calc_reallocation(size), _calc_offset(size), alloc )
        {
        }

        static constexpr size_type _calc_reallocation( size_type size )
        {
            return size * _full_realloc::num / _full_realloc::den;
        }

        static constexpr size_type _calc_offset( size_type size )
        {
            return size * _front_realloc::num / _front_realloc::den;
        }

        // Acquire Allocator
        constexpr Allocator& _allocator() noexcept
        {
            return _data.allocator();
        }

        constexpr const Allocator& _allocator() const noexcept
        {
            return _data.allocator();
        }

        // Destroy elements in range
        constexpr void _destroy( const_iterator b, const_iterator e )
        {
            if constexpr ( std::is_trivially_destructible_v<T> && _calls_destructor_directly )
            {
                (void)b; (void)e; // Unused
            }
            else
            {
                auto start = _mutable_iterator(b);
                for ( auto i = start; i != e; ++i )
                {
                    alloc_traits::destroy( _allocator(), i );
                }
            }
        }

        template< typename OtherResizeTraits >
        constexpr veque & _copy_assignment( const veque<T,OtherResizeTraits,Allocator> & other )
        {
            if constexpr ( alloc_traits::propagate_on_container_copy_assignment::value )
            {
                if constexpr ( !alloc_traits::is_always_equal::value )
                {
                    if ( other._allocator() != _allocator() || other.size() > capacity_full() )
                    {
                        _swap_with_allocator( veque( other, other._allocator() ) );
                        return *this;
                    }
                }
            }
            if ( other.size() > capacity_full() )
            {
                _swap_without_allocator( veque( other, _allocator() ) );
            }
            else
            {
                _reassign_existing_storage( other.begin(), other.end() );
            }
            return *this;
        }

        template< typename OtherResizeTraits >
        constexpr veque & _move_assignment( veque<T,OtherResizeTraits,Allocator> && other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_move_assignment::value
            || alloc_traits::is_always_equal::value) )
        {
            if constexpr ( !alloc_traits::is_always_equal::value )
            {
                if ( _allocator() != other._allocator() )
                {
                    if constexpr ( alloc_traits::propagate_on_container_move_assignment::value )
                    {
                        _swap_with_allocator( std::move(other) );
                    }
                    else
                    {
                        if ( other.size() > capacity_full() )
                        {
                            _swap_without_allocator( veque( std::move(other), _allocator() ) );
                        }
                        else
                        {
                            _reassign_existing_storage( std::move_iterator(other.begin()), std::move_iterator(other.end()) );
                        }
                    }
                    return *this;
                }
            }
            _swap_without_allocator( std::move(other) );
            return *this;
        }

        // Construct elements in range
        template< typename ...Args >
        constexpr void _value_construct_range( const_iterator b, const_iterator e, const Args & ...args )
        {
            static_assert( sizeof...(args) <= 1, "This is for default- or copy-constructing" );

            // The byte-wise paths below cannot run during constant
            // evaluation, which falls through to the element-wise loop.
            if ( !std::is_constant_evaluated() )
            {
                if constexpr ( std::is_trivially_copy_constructible_v<T> && _calls_default_constructor_directly )
                {
                    if constexpr ( sizeof...(args) == 0 )
                    {
                        auto count = std::distance( b, e );
                        if ( count )
                        {
                            std::memset( _mutable_iterator(b), 0, count * sizeof(T) );
                        }
                    }
                    else
                    {
                        std::fill( _mutable_iterator(b), _mutable_iterator(e), args...);
                    }
                    return;
                }
            }
            for ( auto dest = _mutable_iterator(b); dest != e; ++dest )
            {
                alloc_traits::construct( _allocator(), dest, args... );
            }
        }

        template< typename It >
        constexpr void _copy_construct_range( It b, It e, iterator dest )
        {
            static_assert( std::forward_iterator<It> );
            // The memcpy fast path requires the source to be a raw pointer; a
            // foreign iterator (e.g. std::vector's) is not necessarily
            // contiguous, so route it through element-wise construction.
            if ( !std::is_constant_evaluated() )
            {
                if constexpr ( std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly && std::is_pointer_v<It> )
                {
                    auto count = std::distance( b, e );
                    if ( count )
                    {
                        std::memcpy( dest, b, count * sizeof(T) );
                    }
                    return;
                }
            }
            for ( ; b != e; ++dest, ++b )
            {
                alloc_traits::construct( _allocator(), dest, *b );
            }
        }
        
        template< typename It >
        constexpr void _assign( It b, It e )
        {
            static_assert( std::forward_iterator<It> );
            if ( std::ranges::distance( b, e ) > static_cast<difference_type>(capacity_full()) )
            {
                _swap_without_allocator( veque( b, e, _allocator() ) );
            }
            else
            {
                _reassign_existing_storage( b, e );
            }
        }

        template< typename It >
        constexpr void _assign( It b, It e, _multi_pass_tag )
        {
            _assign( b, e );
        }

        template< typename It >
        constexpr void _assign( It b, It e, _single_pass_tag )
        {
            // Input Iterators require a single-pass solution
            clear();
            for ( ; b != e; ++b )
            {
                push_back( *b );
            }
        }

        template< typename It >
        constexpr iterator _insert( const_iterator it, It b, It e )
        {
            static_assert( std::forward_iterator<It> );
            auto res = _insert_storage( it, std::ranges::distance( b, e ) );
            _copy_construct_range( b, e, res );
            return res;
        }

        template< typename It >
        constexpr iterator _insert( const_iterator it, It b, It e, _multi_pass_tag )
        {
            return _insert( it, b, e );
        }

        template< typename It >
        constexpr iterator _insert( const_iterator it, It b, It e, _single_pass_tag )
        {
            // Input Iterators require a single-pass solution
            auto allocated = veque( b, e );
            return _insert( it, allocated.begin(), allocated.end() );
        }

        template< typename OtherResizeTraits >
        constexpr void _swap_with_allocator( veque<T,OtherResizeTraits,Allocator> && other ) noexcept
        {
            // Swap everything.  Members are swapped individually rather than
            // swapping the whole Data, because Data is a distinct type for each
            // veque instantiation (it is nested), so a differing-ResizeTraits
            // other has an incompatible Data type.
            using std::swap;
            swap( _size,             other._size );
            swap( _offset,           other._offset );
            swap( _data._allocated,  other._data._allocated );
            swap( _data._storage,    other._data._storage );
            if constexpr ( ! std::is_empty_v<Allocator> )
            {
                swap( _allocator(), other._allocator() );
            }
        }

        template< typename OtherResizeTraits >
        constexpr void _swap_without_allocator( veque<T,OtherResizeTraits,Allocator> && other ) noexcept
        {
            // Don't swap _data.allocator().
            std::swap( _size,            other._size );
            std::swap( _offset,          other._offset );
            std::swap( _data._allocated, other._data._allocated);
            std::swap( _data._storage,   other._data._storage);
        }

        template< typename ...Args >
        constexpr void _resize_front( size_type count, const Args & ...args )
        {
            difference_type delta = count - size();
            if ( delta > 0 )
            {
                if ( count > capacity_front() )
                {
                    _reallocate_space_at_front( count );
                }
                _value_construct_range( begin() - delta, begin(), args... );
            }
            else
            {
                _destroy( begin(), begin() - delta );
            }
            _move_begin( -delta );
        }

        template< typename ...Args >
        constexpr void _resize_back( size_type count, const Args & ...args )
        {
            difference_type delta = count - size();
            if ( delta > 0 )
            {
                if ( count > capacity_back() )
                {
                    _reallocate_space_at_back( count );
                }
                _value_construct_range( end(), end() + delta, args... );
            }
            else
            {
                _destroy( end() + delta, end() );
            }
            _move_end( delta );
        }

        // Move veque to new storage, with specified capacity...
        // ...and yet-unused space at back of this storage
        constexpr void _reallocate_space_at_back( size_type count )
        {
            auto storage_needed = _calc_reallocation(count);
            auto current_capacity = capacity_full();
            auto new_offset = _calc_offset(count);
            if ( storage_needed <= current_capacity )
            {
                // Shift elements toward front
                auto distance = _offset - new_offset;
                _shift_front( begin(), end(), distance  );
                _move_begin(-distance);
                _move_end(-distance);
            }
            else
            {
                _reallocate( storage_needed, new_offset );
            }
        }
        
        // ...and yet-unused space at front of this storage
        constexpr void _reallocate_space_at_front( size_type count )
        {
            auto storage_needed = _calc_reallocation(count);
            auto current_capacity = capacity_full();
            auto new_offset = count - size() + _calc_offset(count);
            if ( storage_needed <= current_capacity )
            {
                // Shift elements toward back
                auto distance = new_offset - _offset;
                _shift_back( begin(), end(), distance );
                _move_begin(distance);
                _move_end(distance);
            }
            else
            {
                _reallocate( storage_needed, new_offset );
            }
        }
        
        // Move veque to new storage, with specified capacity
        constexpr void _reallocate( size_type allocated, size_type offset )
        {
            auto replacement = veque( _allocate_uninitialized_tag{}, size(), allocated, offset, _allocator() );
            _nothrow_move_construct_range( begin(), end(), replacement.begin() );
            _swap_without_allocator( std::move(replacement) );
        }

        // Insert empty space, choosing the most efficient way to shift existing elements
        constexpr iterator _insert_storage( const_iterator it, size_type count )
        {
            auto required_size = size() + count;
            auto can_shift_back = capacity_back() >= required_size;
            if constexpr ( std::ratio_greater_v<_front_realloc,std::ratio<0>> )
            {
                if ( can_shift_back && it == begin() )
                {
                    // Don't favor shifting entire contents back
                    // if realloc will create space
                    can_shift_back = false;
                }
            }

            if constexpr ( _resize_from_closest_side )
            {
                auto can_shift_front = capacity_front() >= required_size;
                if constexpr ( std::ratio_greater_v<_back_realloc,std::ratio<0>> )
                {
                    if ( can_shift_front && it == end() )
                    {
                        // Don't favor shifting entire contents front
                        // if realloc will create space
                        can_shift_front = false;
                    }
                }

                if ( can_shift_back && can_shift_front)
                {
                    // Capacity allows shifting in either direction.
                    // Remove the choice with the greater operation count.
                    auto index = std::distance( cbegin(), it );
                    if ( index <= ssize() / 2 )
                    {
                        can_shift_back = false;
                    }
                    else
                    {
                        can_shift_front = false;
                    }
                }

                if ( can_shift_front )
                {
                    _shift_front( begin(), it, count );
                    _move_begin( -count );
                    return _mutable_iterator(it) - count;
                }
            }
            if ( can_shift_back )
            {
                _shift_back( it, end(), count );
                _move_end( count );
                return _mutable_iterator(it);
            }

            // Insufficient capacity.  Allocate new storage.
            auto replacement = veque( _reallocate_uninitialized_tag{}, required_size, _allocator() );
            auto index = std::distance( cbegin(), it );
            auto insertion_point = begin() + index;

            _nothrow_move_construct_range( begin(), insertion_point, replacement.begin() );
            _nothrow_move_construct_range( insertion_point, end(), replacement.begin() + index + count );
            _swap_with_allocator( std::move(replacement) );
            return begin() + index;
        }

        // Moves a valid subrange in the front direction.
        // Veque will grow, if range moves past begin().
        // Veque will shrink if range includes end().
        // Returns iterator to beginning of destructed gap
        constexpr void _shift_front( const_iterator b, const_iterator e, size_type count )
        {
            if ( e == begin() )
            {
                return;
            }
            auto element_count = std::distance( b, e );
            auto start = _mutable_iterator(b);
            if ( element_count > 0 )
            {
                auto dest = start - count;
                bool moved_as_bytes = false;
                if constexpr ( std::is_trivially_copyable_v<T> && std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
                {
                    if ( !std::is_constant_evaluated() )
                    {
                        std::memmove( dest, start, element_count * sizeof(T) );
                        moved_as_bytes = true;
                    }
                }
                if ( !moved_as_bytes )
                {
                    auto src = start;
                    auto dest_construct_end = std::min( begin(), _mutable_iterator(e) - count );
                    for ( ; dest < dest_construct_end; ++src, ++dest )
                    {
                        _nothrow_move_construct( dest, src );
                    }
                    for ( ; src != e; ++src, ++dest )
                    {
                        _nothrow_move_assign( dest, src );
                    }
                }
            }
            _destroy( std::max( cbegin(), e - count ), e );
        }

        // Moves a range towards the back.  Veque will grow, if needed.  Vacated elements are destructed.
        // Moves a valid subrange in the back direction.
        // Veque will grow, if range moves past end().
        // Veque will shrink if range includes begin().
        // Returns iterator to beginning of destructed gap
        constexpr void _shift_back( const_iterator b, const_iterator e, size_type count )
        {
            auto start = _mutable_iterator(b); 
            if ( b == end() )
            {
                return;
            }
            auto element_count = std::distance( b, e );
            if ( element_count > 0 )
            {
                bool moved_as_bytes = false;
                if constexpr ( std::is_trivially_copyable_v<T> && std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
                {
                    if ( !std::is_constant_evaluated() )
                    {
                        std::memmove( start + count, start, element_count * sizeof(T) );
                        moved_as_bytes = true;
                    }
                }
                if ( !moved_as_bytes )
                {
                    auto src = _mutable_iterator(e-1);
                    auto dest = src + count;
                    auto dest_construct_end = std::max( end()-1, dest - element_count );
                    for ( ; dest > dest_construct_end; --src, --dest )
                    {
                        // Construct to destinations at or after end()
                        _nothrow_move_construct( dest, src );
                    }
                    for ( ; src != b-1; --src, --dest )
                    {
                        // Assign to destinations before before end()
                        _nothrow_move_assign( dest, src );
                    }
                }
            }
            _destroy( b, std::min( cend(), b + count ) );
        }

        // Assigns a fitting range of new elements to currently held storage.
        // Favors copying over constructing firstly, and positioning the new elements
        // at the center of storage secondly
        template< typename It >
        constexpr void _reassign_existing_storage( It b, It e )
        {
            static_assert( std::forward_iterator<It> );

            auto count = std::ranges::distance( b, e );
            auto size_delta = static_cast<difference_type>( count - size() );
            // The "ideal" begin would put the new data in the center of storage
            auto ideal_begin = _storage_begin() + (capacity_full() - count) / 2;

            if ( size() == 0 )
            {
                // Existing veque is empty.  Construct at the ideal location
                _copy_construct_range( b, e, ideal_begin );        
            }
            else if ( size_delta == 0 )
            {
                // Existing veque is the same size.  Avoid any construction by copy-assigning everything
                std::copy( b, e, begin() );
                return;
            }
            else if ( size_delta < 0 )
            {
                // New size is smaller.  Copy-assign everything, placing results as close to center as possible
                ideal_begin = std::clamp( ideal_begin, begin(), end() - count );

                _destroy( begin(), ideal_begin );
                auto ideal_end = std::copy( b, e, ideal_begin );
                _destroy( ideal_end, end() );
            }
            else
            {
                // New size is larger.  Copy-assign all existing elements, placing newly
                // constructed elements so final store is as close to center as possible
                ideal_begin = std::clamp( ideal_begin, end() - count, begin() );

                // std::ranges::next rather than +: a forward-only source
                // (std::list, std::set) cannot be indexed, and this stays O(1)
                // for the random-access sources that can.
                const auto constructed_before =
                    static_cast<std::iter_difference_t<It>>( std::distance( ideal_begin, begin() ) );
                const auto assign_begin = std::ranges::next( b, constructed_before );
                const auto assign_end =
                    std::ranges::next( assign_begin, static_cast<std::iter_difference_t<It>>( ssize() ) );

                _copy_construct_range( b, assign_begin, ideal_begin );
                std::copy( assign_begin, assign_end, begin() );
                _copy_construct_range( assign_end, e, end() );
            }
            _move_begin( std::distance( begin(), ideal_begin ) );
            _move_end( std::distance( end(), ideal_begin + count ) );
        }

        constexpr void _reassign_existing_storage( size_type count, const T & value )
        {
            auto size_delta = static_cast<difference_type>( count - size() );
            auto ideal_begin = _storage_begin();
            // The "ideal" begin would put the new data in the center of storage
            if constexpr ( std::ratio_greater_v<_unused_realloc, std::ratio<0>> )
            {
                using ideal_begin_ratio = std::ratio_divide<_front_realloc, _unused_realloc >;
                ideal_begin += (capacity_full() - count) * ideal_begin_ratio::num / ideal_begin_ratio::den;
            }

            if ( size() == 0 )
            {
                // Existing veque is empty.  Construct at the ideal location
                _value_construct_range( ideal_begin, ideal_begin + count, value );
            }
            else if ( size_delta == 0 )
            {
                // Existing veque is the same size.  Avoid any construction by copy-assigning everything
                std::fill( begin(), end(), value );
                return;
            }
            else if ( size_delta < 0 )
            {
                // New size is smaller.  Copy-assign everything, placing results as close to center as possible
                ideal_begin = std::clamp( ideal_begin, begin(), end() - count );

                _destroy( begin(), ideal_begin );
                std::fill( ideal_begin, ideal_begin + count, value );
                _destroy( ideal_begin + count, end() );
            }
            else
            {
                // New size is larger.  Copy-assign all existing elements, placing newly
                // constructed elements so final store is as close to center as possible
                ideal_begin = std::clamp( ideal_begin, end() - count, begin() );
                _value_construct_range( ideal_begin, begin(), value );
                std::fill( begin(), end(), value );
                _value_construct_range( end(), ideal_begin + count, value );
            }
            _move_begin( std::distance( begin(), ideal_begin ) );
            _move_end( std::distance( end(), ideal_begin + count ) );
        }

        // Casts to T&& or T&, depending on whether move construction is noexcept
        static constexpr decltype(auto) _nothrow_construct_move( T & t )
        {
            if constexpr ( std::is_nothrow_move_constructible_v<T> )
            {
                return std::move(t);
            }
            else
            {
                return t;
            }
        }

        // Move-constructs if noexcept, copies otherwise
        constexpr void _nothrow_move_construct( iterator dest, iterator src )
        {
            if constexpr ( std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
            {
                *dest = *src;
            }
            else
            {
                alloc_traits::construct( _allocator(), dest, _nothrow_construct_move(*src) );
            }
        }

        constexpr void _nothrow_move_construct_range( iterator b, iterator e, iterator dest )
        {
            auto size = std::distance( b, e );
            if ( size )
            {
                if ( !std::is_constant_evaluated() )
                {
                    if constexpr ( std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
                    {
                        std::memcpy( dest, b, size * sizeof(T) );
                        return;
                    }
                }
                for ( ; b != e; ++dest, ++b )
                {
                    _nothrow_move_construct( dest, b );
                }
            }
        }

        // Move-assigns if noexcept, copies otherwise
        static constexpr void _nothrow_move_assign( iterator dest, iterator src )
        {
            if constexpr ( std::is_nothrow_move_assignable_v<T> )
            {
                *dest = std::move(*src);
            }
            else
            {
                *dest = *src;
            }
        }

        static constexpr void _nothrow_move_assign_range( iterator b, iterator e, iterator src )
        {
            for ( auto dest = b; dest != e; ++dest, ++src )
            {
                _nothrow_move_assign( dest, src );
            }
        }

        // Adjust begin(), end() iterators
        constexpr void _move_begin( difference_type count ) noexcept
        {
            _size -= count;
            _offset += count;
        }

        constexpr void _move_end( difference_type count ) noexcept
        {
            _size += count;
        }

        // Convert a local const_iterator to iterator
        constexpr iterator _mutable_iterator( const_iterator i )
        {
            return begin() + std::distance( cbegin(), i );
        }

        // Retrieves beginning of storage, which may be before begin()
        constexpr const_iterator _storage_begin() const noexcept
        {
            return _data._storage;
        }

        constexpr iterator _storage_begin() noexcept
        {
            return _data._storage;
        }
    };

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline constexpr bool operator==( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return std::equal( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() );
    }

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline constexpr bool operator!=( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return !( lhs == rhs );
    }

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline constexpr bool operator<( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return std::lexicographical_compare( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() );
    }

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline constexpr bool operator<=( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return !( rhs < lhs );
    }

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline constexpr bool operator>( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return ( rhs < lhs );
    }

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline constexpr bool operator>=( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return !( lhs < rhs );
    }

    template< typename T, typename ResizeTraits, typename Alloc >
    inline constexpr void swap( veque<T,ResizeTraits,Alloc> & lhs, veque<T,ResizeTraits,Alloc> & rhs ) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

#ifdef __cpp_lib_containers_ranges
    // Template deduction guide for a range
    template< std::ranges::input_range R,
              typename Alloc = std::allocator<std::ranges::range_value_t<R>> >
    veque( std::from_range_t, R &&, Alloc = Alloc() )
        -> veque< std::ranges::range_value_t<R>, fast_resize_traits, Alloc >;
#endif

    // Template deduction guide for iterator pair
    template< typename InputIt,
              typename Alloc = std::allocator<typename std::iterator_traits<InputIt>::value_type>>
    veque(InputIt, InputIt, Alloc = Alloc())
      -> veque<typename std::iterator_traits<InputIt>::value_type, fast_resize_traits, Alloc>;

}

namespace std
{
    template< typename T, typename ResizeTraits, typename Alloc >
    struct hash<veque::veque<T,ResizeTraits,Alloc>>
    {
        constexpr size_t operator()( const veque::veque<T,ResizeTraits,Alloc> & v ) const
        {
            size_t hash = 0;
            auto hasher = std::hash<T>();
            for ( auto && val : v )
            {
                hash ^= hasher(val) + 0x9e3779b9 + (hash<<6) + (hash>>2);
            }
            return hash;
        }
    };
}

#endif
