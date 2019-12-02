/*
 * veque.hpp
 *
 * Efficient generic C++ container combining useful features of std::vector and std::deque
 *
 * Copyright (C) 2019 Drew Dormann
 *
 */

#ifndef VEQUE_HEADER_GUARD
#define VEQUE_HEADER_GUARD

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <limits>
#include <ratio>
#include <type_traits>
#include <stdexcept>
#include <utility>

    template< typename T, typename Allocator = std::allocator<T> >
    class veque
    {
        template<typename I, typename = void>
        struct is_input_iterator : std::false_type {};

        template<typename I>
        struct is_input_iterator<I, std::void_t<
            typename std::iterator_traits<I>::iterator_category, // Is an iterator
            decltype(std::declval<I&>() == std::declval<I&>())   // InputIterator
        > > : std::true_type {};
        
        using alloc_traits = std::allocator_traits<Allocator>;

    public:
        // Types
        using allocator_type = Allocator;
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
        veque() noexcept (noexcept(Allocator()));
        explicit veque( const Allocator& ) noexcept;
        explicit veque( size_type n, const Allocator& = Allocator() );
        veque( size_type n, const T &val, const Allocator& = Allocator() );
        template< typename InputIt, typename = std::enable_if_t<is_input_iterator<InputIt>::value> >
        veque( InputIt first,  InputIt last, const Allocator& = Allocator() );
        veque( std::initializer_list<T>, const Allocator& = Allocator() );
        veque( const veque & );
        veque( const veque &, const Allocator& );
        veque( veque && ) noexcept;
        veque( veque &&, const Allocator& ) noexcept;
        ~veque();
        veque & operator=( const veque & );
        veque & operator=( veque && ) noexcept(
            noexcept(alloc_traits::propagate_on_container_move_assignment::value
            || alloc_traits::is_always_equal::value) );
        veque & operator=( std::initializer_list<T> );
        void assign( size_type, const T &value );
        template< typename InputIt, typename = std::enable_if_t<is_input_iterator<InputIt>::value> >
        void assign( InputIt, InputIt );
        void assign( std::initializer_list<T> );
        allocator_type get_allocator() const;

        // Element access
        reference at(size_type);
        const_reference at(size_type) const;
        reference operator[](size_type) ;
        const_reference operator[](size_type) const;
        reference front();
        const_reference front() const;
        reference back();
        const_reference back() const;
        T * data() noexcept;
        const T * data() const noexcept;

        // Iterators
        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;
        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;
        reverse_iterator rbegin() noexcept;
        const_reverse_iterator rbegin() const noexcept;
        const_reverse_iterator crbegin() const noexcept;
        reverse_iterator rend() noexcept;
        const_reverse_iterator rend() const noexcept;
        const_reverse_iterator crend() const noexcept;

        // Capacity
        [[nodiscard]] bool empty() const noexcept;
        size_type size() const noexcept;
        ssize_type ssize() const noexcept;
        size_type max_size() const noexcept;
        void reserve( size_type );
        // Reserve front and back capacity, independently.
        void reserve( size_type, size_type );
        void reserve_front( size_type );
        void reserve_back( size_type );
        // Returns current size + unused allocated storage before front()
        size_type capacity_front() const noexcept;
        // Returns current size + unused allocated storage after back()
        size_type capacity_back() const noexcept;
        // Returns current size + all unused allocated storage
        size_type capacity_full() const noexcept;
        // To achieve interface parity with std::vector, capacity() returns capacity_back();
        size_type capacity() const noexcept;
        void shrink_to_fit();

        // Modifiers
        void clear() noexcept;
        iterator insert( const_iterator, const T & );
        iterator insert( const_iterator, T && );
        iterator insert( const_iterator, size_type, const T& );
        template< typename InputIt, typename = std::enable_if_t<is_input_iterator<InputIt>::value> >
        iterator insert( const_iterator, InputIt, InputIt );
        iterator insert( const_iterator, std::initializer_list<T> );
        template< typename ...Args >
        iterator emplace(const_iterator, Args && ... );
        iterator erase( const_iterator );
        iterator erase( const_iterator, const_iterator );
        void push_back( const T & );
        void push_back( T && );
        template< typename ... Args>
        reference emplace_back( Args && ...args );
        void push_front( const T & );
        void push_front( T && );
        template< typename ... Args>
        reference emplace_front( Args && ...args );
        void pop_back();
        // Move-savvy pop back with strong exception guarantee
        T pop_back_element();
        void pop_front();
        // Move-savvy pop front with strong exception guarantee
        T pop_front_element();
        // Resizes the veque, by adding or removing from the front. 
        void resize_front( size_type );
        void resize_front( size_type, const T & );
        // Resizes the veque, by adding or removing from the back.
        void resize_back( size_type );
        void resize_back( size_type, const T & );
        // To achieve interface parity with std::vector, resize() performs resize_back();
        void resize( size_type );
        void resize( size_type, const T & );
        void swap( veque & ) noexcept(
            noexcept(alloc_traits::propagate_on_container_swap::value
            || alloc_traits::is_always_equal::value));

    private:
        
        // If true, arbitrary insert and erase operations are twice the speed of
        //   std::vector, but those operations invalidate all iterators
        //   
        // If false, veque is a 100% compatible drop-in replacement for
        //   std::vector including iterator invalidation rules
        static constexpr auto resize_from_closest_side = true;

        // Relative to size(), amount of unused space to reserve when reallocating
        using front_realloc = std::ratio<1>;
        using back_realloc = std::ratio<1>;
        
        static_assert( std::ratio_greater_equal_v<front_realloc,std::ratio<0>> );
        static_assert( std::ratio_greater_equal_v<back_realloc,std::ratio<0>> );

        
        template<typename Alloc, typename = void >
        struct has_no_allocator_default_constructor : std::true_type {};

        template<typename Alloc>
        struct has_no_allocator_default_constructor< Alloc, std::void_t<
            decltype(std::declval<Alloc&>().construct(std::declval<T*>()))
        > > : std::false_type {};
        
        template<typename Alloc, typename = void >
        struct has_no_allocator_copy_constructor : std::true_type {};

        template<typename Alloc>
        struct has_no_allocator_copy_constructor< Alloc, std::void_t<
            decltype(std::declval<Alloc&>().construct(std::declval<T*>(), std::declval<const T&>()))
        > > : std::false_type {};
        
        template<typename Alloc, typename = void >
        struct has_no_allocator_destructor : std::true_type {};

        template<typename Alloc>
        struct has_no_allocator_destructor< Alloc, std::void_t<
            decltype(std::declval<Alloc&>().destroy(std::declval<T*>()))
        > > : std::false_type {};
        
        // Confirmation that allocator_traits will only directly call placement new(ptr)T()
        static constexpr auto calls_default_constructor_directly = 
            std::is_same_v<void,std::allocator<T>> ||
            has_no_allocator_default_constructor<Allocator>::value;
        // Confirmation that allocator_traits will only directly call placement new(ptr)T(const T&)
        static constexpr auto calls_copy_constructor_directly = 
            std::is_same_v<void,std::allocator<T>>||
            has_no_allocator_copy_constructor<Allocator>::value;
        // Confirmation that allocator_traits will only directly call ~T()
        static constexpr auto calls_destructor_directly =
            std::is_same_v<void,std::allocator<T>> ||
            has_no_allocator_destructor<Allocator>::value;
        
        using full_realloc = std::ratio_add<std::ratio<1>,std::ratio_add<front_realloc,back_realloc>>;

        size_type _size = 0;    // Number of elements in use
        size_type _offset = 0;  // Number of uninitialized elements before begin()
        
        struct Data : Allocator // Employing EBO, since the allocator is frequently monostate.
        {
            size_type _allocated = 0;
            T *_storage = nullptr;

            Data() {}
            Data( size_type capacity, const Allocator & alloc )
                : Allocator{alloc}
                , _allocated{capacity}
                , _storage{ std::allocator_traits<Allocator>::allocate( allocator(), capacity ) }
            {
            }
            Data( const Allocator & alloc ) : Allocator{alloc} {}
            Data( const Data& ) = delete;
            Data( Data && other )
            {
                *this = std::move(other);
            }
            ~Data()
            {
                std::allocator_traits<Allocator>::deallocate( allocator(), _storage, _allocated );
            }
            Data& operator=( const Data & ) = delete;
            Data& operator=( Data && other )
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
            Allocator& allocator() { return *this; }
            const Allocator& allocator() const { return *this; }
        } _data;

        struct allocate_uninitialized_tag {};

        // Create an uninitialized empty veque, with storage for expected size
        veque( allocate_uninitialized_tag, size_type size, const Allocator& );
        // Create an uninitialized empty veque, with specified storage params
        veque( allocate_uninitialized_tag, size_type size, size_type allocated, size_type offset, const Allocator& );
        // Acquire Allocator
        Allocator& _allocator() noexcept;
        const Allocator& _allocator() const noexcept;
        // Destroy elements in range
        void _destroy( const_iterator begin, const_iterator end );
        // Construct elements in range
        template< typename ...Args >
        void _value_construct_range( const_iterator begin, const_iterator end, const Args & ...args );
        void _copy_construct_range( const_iterator begin, const_iterator end, const_iterator src );

        void _swap_with_allocator( veque && other ) noexcept;
        void _swap_without_allocator( veque && other ) noexcept;

        template< typename ...Args >
        void _resize_front(size_type, const Args & ...);
        template< typename ...Args >
        void _resize_back(size_type, const Args & ...);

        // Move veque to new storage, with specified capacity...
        // ...and yet-unused space at back of this storage
        void _reallocate_space_at_back( size_type count );
        // ...and yet-unused space at front of this storage
        void _reallocate_space_at_front( size_type count );
        // Move vector to new storage, with specified capacity
        void _reallocate( size_type allocated, size_type offset );
        // Insert empty space, choosing the most efficient way to shift existing elements
        iterator _insert_storage( const_iterator it, size_type count );
        // Moves a valid subrange in the front direction.
        // Vector will grow, if range moves past begin().
        // Vector will shrink if range includes end().
        // Returns iterator to beginning of destructed gap
        iterator _shift_front( const_iterator begin, const_iterator end, size_type count );
        // Moves a range towards the back.  Vector will grow, if needed.  Vacated elements are destructed.
        // Moves a valid subrange in the back direction.
        // Vector will grow, if range moves past end().
        // Vector will shrink if range includes begin().
        // Returns iterator to beginning of destructed gap
        iterator _shift_back( const_iterator begin, const_iterator end, size_type count );
        // Assigns a fitting range of new elements to currently held storage.
        // Favors copying over constructing firstly, and positioning the new elements
        // at the center of storage secondly
        template< typename InputIt, typename = std::enable_if_t<is_input_iterator<InputIt>::value> >
        void _reassign_existing_storage( InputIt first, InputIt last );
        void _reassign_existing_storage( size_type count, const T & value );

        // Casts to T&& or T&, depending on whether move construction is noexcept
        static decltype(auto) _nothrow_move( T & t );
        // Move-constructs if noexcept, copies otherwise
        void _nothrow_move_construct( iterator dest, iterator src );
        void _nothrow_move_construct_range( iterator b, iterator e, iterator src );
        
        // Move-assigns if noexcept, copies otherwise
        static void _nothrow_move_assign( iterator dest, iterator src );
        static void _nothrow_move_assign_range( iterator b, iterator e, iterator src );
        // Adjust begin(), end() iterators
        void _move_begin( difference_type ) noexcept;
        void _move_end( difference_type ) noexcept;
        // Convert a local const_iterator to iterator
        iterator _mutable_iterator( const_iterator );
    };

    template< typename T, typename Alloc >
    void veque<T,Alloc>::_move_begin( difference_type count ) noexcept
    {
        _size -= count;
        _offset += count;
    }
    
    template< typename T, typename Alloc >
    void veque<T,Alloc>::_move_end( difference_type count ) noexcept
    {
        _size += count;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::_mutable_iterator( const_iterator i )
    {
        return begin() + std::distance( cbegin(), i );
    }
    
    template< typename T, typename Alloc >
    Alloc& veque<T,Alloc>::_allocator() noexcept
    {
        return _data.allocator();
    }

    template< typename T, typename Alloc >
    const Alloc& veque<T,Alloc>::_allocator() const noexcept
    {
        return _data.allocator();
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::_destroy( const_iterator const b, const_iterator const e )
    {
        if constexpr ( std::is_trivially_destructible_v<T> && calls_destructor_directly )
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

    template< typename T, typename Alloc >
    template< typename ...Args >
    void veque<T,Alloc>::_value_construct_range( const_iterator b, const_iterator e, const Args & ...args )
    {
        if constexpr ( std::is_trivially_copy_constructible_v<T> && sizeof...(args) == 0 && calls_default_constructor_directly )
        {
            std::memset( _mutable_iterator(b), 0, std::distance( b, e ) * sizeof(T) );
        }
        else
        {
            for ( auto dest = _mutable_iterator(b); dest != e; ++dest )
            {
                alloc_traits::construct( _allocator(), dest, args... );
            }
        }
    }
    
    template< typename T, typename Alloc >
    void veque<T,Alloc>::_copy_construct_range( const_iterator b, const_iterator e, const_iterator src )
    {
        if constexpr ( std::is_trivially_copy_constructible_v<T> && calls_copy_constructor_directly )
        {
            std::memcpy( _mutable_iterator(b), src, std::distance( b, e ) * sizeof(T) );
        }
        else
        {
            for ( auto dest = _mutable_iterator(b); dest != e; ++dest, ++src )
            {
                alloc_traits::construct( _allocator(), dest, *src );
            }
        }
    }
    
    template< typename T, typename Alloc >
    void veque<T,Alloc>::_swap_with_allocator( veque && other ) noexcept
    {
        // Swap everything
        std::swap( _size,      other._size );
        std::swap( _offset,    other._offset );
        std::swap( _data,      other._data );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::_swap_without_allocator( veque && other ) noexcept
    {
        // Don't swap _data.allocator().
        std::swap( _size,            other._size );
        std::swap( _offset,          other._offset );
        std::swap( _data._allocated, other._data._allocated);
        std::swap( _data._storage,   other._data._storage);
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::_reallocate_space_at_back( size_type count )
    {
        auto allocated = count * full_realloc::num / full_realloc::den;
        auto offset = count * front_realloc::type::num / front_realloc::type::den;
        _reallocate( allocated, offset );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::_reallocate_space_at_front( size_type count )
    {
        auto allocated = count * full_realloc::num / full_realloc::den;
        auto offset = count - size() + count * front_realloc::type::num / front_realloc::type::den;
        _reallocate( allocated, offset );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::_reallocate( size_type allocated, size_type offset )
    {
        auto replacement = veque( allocate_uninitialized_tag{}, size(), allocated, offset, _allocator() );
        _nothrow_move_construct_range( replacement.begin(), replacement.end(), begin() );
        _swap_with_allocator( std::move(replacement) );
    }

    template< typename T, typename Alloc >
    decltype(auto) veque<T,Alloc>::_nothrow_move( T & t )
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

    template< typename T, typename Alloc >
    void veque<T,Alloc>::_nothrow_move_construct( iterator dest, iterator src )
    {
        if constexpr ( std::is_trivially_copy_constructible_v<T> && calls_copy_constructor_directly )
        {
            *dest = *src;
        }
        else
        {
            alloc_traits::construct( _allocator(), dest, _nothrow_move(*src) );
        }
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::_nothrow_move_construct_range( iterator b, iterator e, iterator src )
    {
        auto size = std::distance( b, e );
        if ( size )
        {
            if constexpr ( std::is_trivially_copy_constructible_v<T> && calls_copy_constructor_directly )
            {
                std::memcpy( b, src, size * sizeof(T) );
            }
            else
            {
                for ( auto dest = b; dest != e; ++dest, ++src )
                {
                    _nothrow_move_construct( dest, src );
                }
            }
        }
    }
               
    template< typename T, typename Alloc >
    void veque<T,Alloc>::_nothrow_move_assign( iterator dest, iterator src )
    {
        *dest = _nothrow_move(*src);
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::_nothrow_move_assign_range( iterator b, iterator e, iterator src )
    {
        for ( auto dest = b; dest != e; ++dest, ++src )
        {
            _nothrow_move_assign( dest, src );
        }
    }
    
    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::_shift_front( const_iterator b, const_iterator e, size_type distance )
    {
        if ( e == begin() )
        {
            _move_begin( -distance );
            return begin();
        }
        auto element_count = std::distance( b, e );
        auto start = _mutable_iterator(b);
        if ( element_count )
        {
            auto dest = start - distance;
            if constexpr ( std::is_trivially_copyable_v<T> && calls_copy_constructor_directly )
            {
                std::memmove( dest, start, element_count * sizeof(T) );
            }
            else
            {
                auto src = start;
                auto dest_construct_end = std::min( begin(), _mutable_iterator(e) - distance );
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
        _destroy( std::max( cbegin(), e - distance ), e );
        auto new_elements_at_front = static_cast<difference_type>(cbegin() - b + distance);
        auto range_includes_end = (e == end());

        // If range includes end(), veque has shrunk
        if ( range_includes_end )
        {
            _move_end( -distance );
        }
        // Otherwise, if range moves before begin(), veque has grown
        else if ( new_elements_at_front > 0 )
        {
            _move_begin( -new_elements_at_front );
        }

        return _mutable_iterator(e) - distance;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::_shift_back( const_iterator b, const_iterator e, size_type distance )
    {
        auto start = _mutable_iterator(b); 
        if ( b == end() )
        {
            _move_end( distance );
            return start;
        }
        auto element_count = std::distance( b, e );
        if ( element_count )
        {
            if constexpr ( std::is_trivially_copyable_v<T> && calls_copy_constructor_directly )
            {
                std::memmove( start + distance, start, element_count * sizeof(T) );
            }
            else
            {
                auto src = _mutable_iterator(e-1);
                auto dest = src + distance;
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
        _destroy( b, std::min( cend(), b + distance ) );
        difference_type new_elements_at_back = e - end() + distance;
        auto range_includes_begin = (b == begin());

        // If range moves after end(), veque has grown
        if ( new_elements_at_back > 0 )
        {
            _move_end( new_elements_at_back );
        }
        // Otherwise, if range includes begin(), veque has shrunk
        else if ( range_includes_begin )
        {
            _move_begin( distance );
        }

        return start;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::_insert_storage( const_iterator it, size_type count )
    {
        auto required_size = size() + count;
        auto can_shift_back = capacity_back() >= required_size && it != begin();
        if constexpr ( resize_from_closest_side )
        {
            auto can_shift_front = capacity_front() >= required_size && it != end();

            if ( can_shift_back && can_shift_front)
            {
                // Capacity allows shifting in either direction.
                // Remove the choice with the greater operations.
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
                // Capacity only allows shifting front.
                return _shift_front( begin(), it, count );
            }
        }
        if ( can_shift_back )
        {
            // Capacity only allows shifting back.
            return _shift_back( it, end(), count );
        }

        // Insufficient capacity.  Allocate new storage.
        auto replacement = veque( allocate_uninitialized_tag{}, required_size, _allocator() );
        auto index = std::distance( cbegin(), it );
        auto insertion_point = begin() + index;
        
        _nothrow_move_construct_range( replacement.begin(), replacement.begin() + index, begin() );
        _nothrow_move_construct_range( replacement.begin() + index + count, replacement.end(), insertion_point );
        _swap_with_allocator( std::move(replacement) );
        return begin() + index;
    }

    template< typename T, typename Alloc >
    template< typename InputIt, typename >
    void veque<T,Alloc>::_reassign_existing_storage( InputIt first, InputIt last )
    {
        auto count = std::distance( first, last );
        auto size_delta = static_cast<difference_type>( count - size() );
        auto ideal_begin = _data._storage + (capacity_full() - count) / 2;
        
        if ( size() == 0 )
        {
            _copy_construct_range( ideal_begin, ideal_begin + count, first );        
            _move_begin( std::distance( begin(), ideal_begin ) );
            _move_end( std::distance( end(), ideal_begin + count ) );
        }
        else if ( size_delta == 0 )
        {
            std::copy( first, last, begin() );
        }
        else if ( size_delta < 0 )
        {
            ideal_begin = std::clamp( ideal_begin, begin(), end() - count );

            auto dest = begin();
            for ( ; dest != ideal_begin; ++dest )
            {
                alloc_traits::destroy( _allocator(), dest );
            }
            dest = std::copy( first, last, dest );
            for ( ; dest != end(); ++dest )
            {
                alloc_traits::destroy( _allocator(), dest );
            }
            _move_begin( std::distance( begin(), ideal_begin ) );
            _move_end( std::distance( end(), ideal_begin + count ) );
        }
        else
        {
            ideal_begin = std::clamp( ideal_begin, end() - count, begin() );

            auto src = first;
            auto dest = ideal_begin;
            for ( ; dest != begin(); ++dest, ++src )
            {
                alloc_traits::construct( _allocator(), dest, *src );
            }
            dest = std::copy( src, src + size(), dest );
            src += size();
            for ( ; src != last; ++dest, ++src  )
            {
                alloc_traits::construct( _allocator(), dest, *src );
            }
            _move_begin( std::distance( begin(), ideal_begin ) );
            _move_end( std::distance( end(), ideal_begin + count ) );
        }
    }
    
    template< typename T, typename Alloc >
    void veque<T,Alloc>::_reassign_existing_storage( size_type count, const T & value )
    {
        auto size_delta = static_cast<difference_type>( count - size() );
        auto ideal_begin = _data._storage + (capacity_full() - count) / 2;
        
        if ( size_delta == 0 )
        {
            std::fill( begin(), end(), value );
        }
        else
        {
            if ( size_delta < 0 )
            {
                ideal_begin = std::clamp( ideal_begin, begin(), end() - count );

                auto dest = begin();
                for ( ; dest != ideal_begin; ++dest )
                {
                    alloc_traits::destroy( _allocator(), dest );
                }
                std::fill( ideal_begin, ideal_begin + count, value );
                for ( ; dest != end(); ++dest )
                {
                    alloc_traits::destroy( _allocator(), dest );
                }
            }
            else
            {
                ideal_begin = std::clamp( ideal_begin, end() - count, begin() );

                auto dest = ideal_begin;
                for ( ; dest != begin(); ++dest )
                {
                    if constexpr ( std::is_trivially_copy_constructible_v<T> && calls_copy_constructor_directly )
                    {
                        *dest = value;
                    }
                    else
                    {
                        alloc_traits::construct( _allocator(), dest, value );
                    }
                }
                std::fill( begin(), end(), value );
                for ( ; dest != ideal_begin + count; ++dest  )
                {
                    if constexpr ( std::is_trivially_copy_constructible_v<T> && calls_copy_constructor_directly )
                    {
                        *dest = value;
                    }
                    else
                    {
                        alloc_traits::construct( _allocator(), dest, value );
                    }
                }
            }
            _move_begin( std::distance( begin(), ideal_begin ) );
            _move_end( std::distance( end(), ideal_begin + count ) );
        }
    }
    
    template< typename T, typename Alloc >
    veque<T,Alloc>::veque() noexcept (noexcept(Alloc()))
        : _data { 0, Alloc{} }
    {
    }

    template< typename T, typename Alloc >
    veque<T,Alloc>::veque( size_type n, const Alloc& alloc )
        : veque( allocate_uninitialized_tag{}, n, alloc )
    {
        _value_construct_range( begin(), end() );
    }

    template< typename T, typename Alloc >
    veque<T,Alloc>::veque( size_type n, const T &value, const Alloc& alloc )
        : veque( allocate_uninitialized_tag{}, n, alloc )
    {
        _value_construct_range( begin(), end(), value );
    }

    template< typename T, typename Alloc >
    template< typename InputIt, typename >
    veque<T,Alloc>::veque( InputIt first, InputIt last, const Alloc& alloc )
        : veque( allocate_uninitialized_tag{}, static_cast<size_type>(std::distance( first, last )), alloc )
    {
        _copy_construct_range( begin(), end(), first );
    }

    template< typename T, typename Alloc >
    veque<T,Alloc>::veque( std::initializer_list<T> lst, const Alloc& alloc  )
        : veque( allocate_uninitialized_tag{}, lst.size(), alloc )
    {
        _copy_construct_range( begin(), end(), lst.begin() );
    }

    template< typename T, typename Alloc >
    veque<T,Alloc>::veque( const veque &other )
        : _size{ other._size }
        , _offset{ 0 }
        , _data { other._size, alloc_traits::select_on_container_copy_construction( other._allocator() ) }
    {
        _copy_construct_range( begin(), end(), other.begin() );
    }

    template< typename T, typename Alloc >
    veque<T,Alloc>::veque( const veque &other, const Alloc& alloc )
        : _size{ other._size }
        , _offset{ 0 }
        , _data { other._size, alloc }
    {
        _copy_construct_range( begin(), end(), other.begin() );
    }

    template< typename T, typename Alloc >
    veque<T,Alloc>::veque(veque &&other) noexcept
        : _data {}
    {
        _swap_with_allocator( std::move(other) );
    }

    template< typename T, typename Alloc >
    veque<T,Alloc>::veque( veque && other, const Alloc& alloc ) noexcept
        : _size{ 0 }
        , _offset{ 0 }
        , _data{ alloc }
    {
        if constexpr ( !alloc_traits::is_always_equal::value )
        {
            if ( alloc != other._allocator() )
            {
                // Incompatible allocators.  Allocate new storage.
                auto replacement = veque( allocate_uninitialized_tag{}, other.size(), alloc );
                _nothrow_move_construct_range( replacement.begin(), replacement.end(), other.begin() );
                _swap_without_allocator( std::move(replacement) );
                return;
            }
        }
        _swap_with_allocator( std::move(other) );
    }

    // Private impl for setting up custom storage
    template< typename T, typename Alloc >
    veque<T,Alloc>::veque( allocate_uninitialized_tag, size_type size, const Alloc & alloc )
        : _size{ size }
        , _offset{ size * front_realloc::type::num / front_realloc::type::den }
        , _data { size * full_realloc::num / full_realloc::den, alloc }
    {
    }

    // Private impl for setting up custom storage
    template< typename T, typename Alloc >
    veque<T,Alloc>::veque( allocate_uninitialized_tag, size_type size, size_type allocated, size_type _offset, const Alloc & alloc )
        : _size{ size }
        , _offset{ _offset }
        , _data { allocated, alloc }
    {
    }

    template< typename T, typename Alloc >
    veque<T,Alloc>::~veque()
    {
        _destroy( begin(), end() );
    }

    template< typename T, typename Alloc >
    veque<T,Alloc> & veque<T,Alloc>::operator=( const veque & other )
    {
        if constexpr ( alloc_traits::propagate_on_container_copy_assignment::value &&
                      !alloc_traits::is_always_equal::value )
        {
            if ( other._allocator() != _allocator() || other.size() > capacity_full() )
            {
                _swap_with_allocator( veque( other, other._allocator() ) );
                return *this;
            }
        }
        else
        {
            if ( other.size() > capacity_full() )
            {
                _swap_without_allocator( veque( other, _allocator() ) );
                return *this;
            }
        }

        _reassign_existing_storage( other.begin(), other.end() );

        return *this;
    }

    template< typename T, typename Alloc >
    veque<T,Alloc> & veque<T,Alloc>::operator=( veque && other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_move_assignment::value
            || alloc_traits::is_always_equal::value) )
    {
        if constexpr ( !alloc_traits::propagate_on_container_move_assignment::value && 
                       !alloc_traits::is_always_equal::value )
        {
            if ( _allocator() != other._allocator() )
            {
                _swap_with_allocator( veque( std::move(other), _allocator() ) );
                return *this;
            }
        }
        _swap_with_allocator( std::move(other) );
        return *this;
    }

    template< typename T, typename Alloc >
    veque<T,Alloc> & veque<T,Alloc>::operator=( std::initializer_list<T> lst )
    {
        assign( lst );
        return *this;
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::assign( size_type count, const T & value )
    {
        if ( count > capacity_full() )
        {
            _swap_with_allocator( veque( count, value, _allocator() ) );
        }
        else
        {
            _reassign_existing_storage( count, value );        
        }
    }
    
    template< typename T, typename Alloc >
    template< typename InputIt, typename >
    void veque<T,Alloc>::assign( InputIt first, InputIt last )
    {
        if ( std::distance( first, last ) > static_cast<difference_type>(capacity_full()) )
        {
            _swap_with_allocator( veque( first, last, _data.allocator() ) );
        }
        else
        {
            _reassign_existing_storage( first, last );        
        }
    }
    
    template< typename T, typename Alloc >
    void veque<T,Alloc>::assign( std::initializer_list<T> lst )
    {
        if ( lst.size() > capacity_full() )
        {
            _swap_with_allocator( veque( lst.begin(), lst.end(), _data.allocator() ) );
        }
        else
        {
            _reassign_existing_storage( lst.begin(), lst.end() );        
        }
    }

    template< typename T, typename Alloc >
    Alloc veque<T,Alloc>::get_allocator() const
    {
        return _allocator();
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::begin() noexcept
    {
        return _data._storage + _offset;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_iterator veque<T,Alloc>::begin() const noexcept
    {
        return cbegin();
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_iterator veque<T,Alloc>::cbegin() const noexcept
    {
        return _data._storage + _offset;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::end() noexcept
    {
        return _data._storage + _offset + size();
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_iterator veque<T,Alloc>::end() const noexcept
    {
        return cend();
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_iterator veque<T,Alloc>::cend() const noexcept
    {
        return _data._storage + _offset + size();
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::reverse_iterator veque<T,Alloc>::rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_reverse_iterator veque<T,Alloc>::rbegin() const noexcept
    {
        return crbegin();
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_reverse_iterator veque<T,Alloc>::crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::reverse_iterator veque<T,Alloc>::rend() noexcept
    {
        return reverse_iterator(begin());
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_reverse_iterator veque<T,Alloc>::rend() const noexcept
    {
        return crend();
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_reverse_iterator veque<T,Alloc>::crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    template< typename T, typename Alloc >
    bool veque<T,Alloc>::empty() const noexcept
    {
        return size() == 0;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::size_type veque<T,Alloc>::size() const noexcept
    {
        return _size;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::ssize_type veque<T,Alloc>::ssize() const noexcept
    {
        return _size;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::size_type veque<T,Alloc>::max_size() const noexcept
    {
        constexpr auto compile_time_limit = std::min(
            // The ssize type's ceiling
            std::numeric_limits<ssize_type>::max() / sizeof(T),
            // Ceiling imposed by std::ratio math
            std::numeric_limits<size_type>::max() / front_realloc::type::num
        );
        
        // The allocator's ceiling
        auto runtime_limit = alloc_traits::max_size(_allocator());
                
        return std::min( compile_time_limit, runtime_limit );
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::size_type veque<T,Alloc>::capacity_full() const noexcept
    {
        return _data._allocated;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::size_type veque<T,Alloc>::capacity_front() const noexcept
    {
        return _offset + size();
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::size_type veque<T,Alloc>::capacity_back() const noexcept
    {
        return capacity_full() - _offset;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::size_type veque<T,Alloc>::capacity() const noexcept
    {
        return capacity_back();
    }

    template< typename T, typename Alloc >
    template< typename ...Args >
    void veque<T,Alloc>::_resize_back( size_type count, const Args & ... args )
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

    template< typename T, typename Alloc >
    void veque<T,Alloc>::resize_back( size_type count )
    {
        _resize_back( count );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::resize_back( size_type count, const T & value )
    {
        _resize_back( count, value );
    }

    template< typename T, typename Alloc >
    template< typename ...Args >
    void veque<T,Alloc>::_resize_front( size_type count, const Args & ...args )
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

    template< typename T, typename Alloc >
    void veque<T,Alloc>::resize_front( size_type count )
    {
        _resize_front( count );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::resize_front( size_type count, const T & value )
    {
        _resize_front( count, value );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::resize( size_type count )
    {
        _resize_back( count );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::resize( size_type count, const T & value )
    {
        _resize_back( count, value );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::reserve_front( size_type count )
    {
        if ( count > capacity_front() )
        {
            auto new_full_capacity = capacity_back() - size() + count;
            if ( new_full_capacity > max_size() )
            {
                throw std::length_error("veque<T,Alloc>::reserve_front(" + std::to_string(count) + ") exceeds max_size()");
            }
            _reallocate( new_full_capacity, count );
        }
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::reserve_back( size_type count )
    {
        if ( count > capacity_back() )
        {
            auto new_full_capacity = capacity_front() - size() + count;
            if ( new_full_capacity > max_size() )
            {
                throw std::length_error("veque<T,Alloc>::reserve_back(" + std::to_string(count) + ") exceeds max_size()");
            }
            _reallocate( new_full_capacity, _offset );
        }
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::reserve( size_type count )
    {
        reserve( count, count );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::reserve( size_type front, size_type back )
    {
        if ( front > capacity_front() || back > capacity_back() )
        {
            auto allocated_before_begin = std::max( capacity_front(), front ) - size();
            auto allocated_after_begin = std::max( capacity_back(), back );
            auto new_full_capacity = allocated_before_begin + allocated_after_begin;
            if ( new_full_capacity > max_size() )
            {
                throw std::length_error("veque<T,Alloc>::reserve(" + std::to_string(front) + ", " + std::to_string(back) + ") exceeds max_size()");
            }
            _reallocate( new_full_capacity, allocated_before_begin );
        }
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::shrink_to_fit()
    {
        if ( size() < capacity_full() )
        {
            _reallocate( size(), 0 );
        }
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::reference veque<T,Alloc>::operator[]( size_type idx)
    {
        return *(begin() + idx);
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_reference veque<T,Alloc>::operator[]( size_type idx) const
    {
        return *(begin() + idx);
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::reference veque<T,Alloc>::at( size_type pos )
    {
        if ( pos >= size() )
        {
            throw std::out_of_range("veque<T,Alloc>::at(" + std::to_string(pos) + ") out of range");
        }
        return (*this)[pos];
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_reference veque<T,Alloc>::at( size_type pos ) const
    {
        if ( pos >= size() )
        {
            throw std::out_of_range("veque<T,Alloc>::at(" + std::to_string(pos) + ") out of range");
        }
        return (*this)[pos];
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::reference veque<T,Alloc>::front()
    {
        return (*this)[0];
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_reference veque<T,Alloc>::front() const
    {
        return (*this)[0];
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::reference veque<T,Alloc>::back()
    {
        return (*this)[size() - 1];
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::const_reference veque<T,Alloc>::back() const
    {
        return (*this)[size() - 1];
    }

    template< typename T, typename Alloc >
    T * veque<T,Alloc>::data() noexcept
    {
        return begin();
    }

    template< typename T, typename Alloc >
    const T * veque<T,Alloc>::data() const noexcept
    {
        return begin();
    }

    template< typename T, typename Alloc >
    template< typename ... Args >
    typename veque<T,Alloc>::reference veque<T,Alloc>::emplace_back( Args && ... args )
    {
        if ( size() == capacity_back() )
        {
            _reallocate_space_at_back( size() + 1 );
        }
        alloc_traits::construct( _allocator(), end(), std::forward<Args>(args)... );
        _move_end( 1 );
        return back();
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::push_back( const T &val )
    {
        emplace_back( val );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::push_back( T &&val )
    {
        emplace_back( std::move(val) );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::pop_back()
    {
        alloc_traits::destroy( _allocator(), &back() );
        _move_end( -1 );
    }

    template< typename T, typename Alloc >
    T veque<T,Alloc>::pop_back_element()
    {
        auto res( _nothrow_move(back()) );
        pop_back();
        return res;
    }

    template< typename T, typename Alloc >
    template< typename ... Args >
    typename veque<T,Alloc>::reference veque<T,Alloc>::emplace_front( Args && ... args )
    {
        if ( size() == capacity_front() )
        {
            _reallocate_space_at_front( size() + 1 );
        }
        alloc_traits::construct( _allocator(), begin()-1, std::forward<Args>(args)... );
        _move_begin( -1 );
        return front();
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::push_front( const T &val )
    {
        emplace_front( val );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::push_front( T &&val )
    {
        emplace_front( std::move(val) );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::pop_front()
    {
        alloc_traits::destroy( _allocator(), &front() );
        _move_begin( 1 );
    }

    template< typename T, typename Alloc >
    T veque<T,Alloc>::pop_front_element()
    {
        auto res( _nothrow_move(front()) );
        pop_front();
        return res;
    }

    template< typename T, typename Alloc >
    template< typename ... Args >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::emplace( const_iterator it, Args && ... args )
    {
        auto res = _insert_storage( it, 1 );
        alloc_traits::construct( _allocator(), res, std::forward<Args>(args)... );
        return res;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::insert( const_iterator it, const T &val )
    {
        return emplace( it, val );
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::insert( const_iterator it, T &&val )
    {
        return emplace( it, std::move(val) );
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::insert( const_iterator it,  size_type count, const T &val )
    {
        auto res = _insert_storage( it, count );
        _value_construct_range( res, res + count, val );
        return res;
    }

    template< typename T, typename Alloc >
    template< typename InputIt, typename >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::insert( const_iterator it, InputIt first, InputIt last )
    {
        auto count = std::distance( first, last );
        auto res = _insert_storage( it, count );
        _copy_construct_range( res, res + count, first );
        return res;
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::insert( const_iterator it, std::initializer_list<T> lst )
    {
        return insert( it, lst.begin(), lst.end() );
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::erase( const_iterator it )
    {
        return erase( it, it + 1 );
    }

    template< typename T, typename Alloc >
    typename veque<T,Alloc>::iterator veque<T,Alloc>::erase( const_iterator first, const_iterator last )
    {
        auto count = std::distance( first, last );
        if constexpr ( resize_from_closest_side )
        {
            auto elements_before = std::distance( cbegin(), first );
            auto elements_after = std::distance( last, cend( ) );
            if (  elements_before < elements_after )
            {
                return _shift_back( begin(), first, count );
            }
        }
        return _shift_front( last, end(), count );
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::swap( veque<T,Alloc> &other ) noexcept( noexcept(
            alloc_traits::propagate_on_container_swap::value ||
            alloc_traits::is_always_equal::value))
    {
        if constexpr ( alloc_traits::propagate_on_container_swap::value )
        {
            _swap_with_allocator( std::move(other) );
        }
        else
        {
            _swap_without_allocator( std::move(other) );
        }
    }

    template< typename T, typename Alloc >
    void veque<T,Alloc>::clear() noexcept
    {
        _destroy( begin(), end() );
        _size = 0;
        _offset = capacity_full() / 2;
    }

    template< typename T, typename Alloc >
    bool operator==( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )
    {
        if ( lhs.size() != rhs.size() )
        {
            return false;
        }
        return std::equal( lhs.cbegin(), lhs.cend(), rhs.cbegin() );
    }

    template< typename T, typename Alloc >
    bool operator!=( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )
    {
        return !( lhs == rhs );
    }

    template< typename T, typename Alloc >
    bool operator<( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )
    {
        return std::lexicographical_compare( lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend() );
    }

    template< typename T, typename Alloc >
    bool operator<=( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )
    {
        return !( rhs < lhs );
    }

    template< typename T, typename Alloc >
    bool operator>( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )
    {
        return ( rhs < lhs );
    }

    template< typename T, typename Alloc >
    bool operator>=( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )
    {
        return !( lhs < rhs );
    }

    template< typename T, typename Alloc >
    void swap( veque<T,Alloc> & lhs, veque<T,Alloc> & rhs ) noexcept(
        noexcept(std::allocator_traits<Alloc>::propagate_on_container_swap::value
        || std::allocator_traits<Alloc>::is_always_equal::value))
    {
        lhs.swap(rhs);
    }

    // Template deduction guide for iterator pair
    template< typename InputIt,
              typename Alloc = std::allocator<typename std::iterator_traits<InputIt>::value_type>>
    veque(InputIt, InputIt, Alloc = Alloc())
      -> veque<typename std::iterator_traits<InputIt>::value_type, Alloc>;
   
namespace std
{
    template< typename T, typename Alloc >
    struct hash<veque<T,Alloc>>
    {
        size_t operator()( const veque<T,Alloc> & v ) const
        {
            size_t hash = 0;
            auto hasher = std::hash<T>{};
            for ( auto && val : v )
            {
                hash ^= hasher(val) + 0x9e3779b9 + (hash<<6) + (hash>>2);
            }
            return hash;
        }
    };
}

#endif
