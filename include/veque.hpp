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
#include <stdexcept>
#include <utility>

#if __cplusplus >= 201703L
#define CPP17 1
#endif

    template <typename T, typename Allocator = std::allocator<T> >
    class veque {
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
        template <typename InputIt>
        veque( InputIt first,  InputIt last, const Allocator& = Allocator() );
        veque( std::initializer_list<T>, const Allocator& = Allocator() );
        veque( const veque &, const Allocator& = Allocator() );
        veque( veque && ) noexcept;
        veque( veque &&, const Allocator& ) noexcept;
        ~veque();
        veque<T> & operator=(const veque<T> &);
        veque<T> & operator=(veque<T> &&);
        veque<T> & operator=(std::initializer_list<T>);
        void assign(size_type, const T &value);
        void assign(iterator, iterator);
        void assign(std::initializer_list<T>);

        // Element access
        reference operator[](size_type);
        const_reference operator[](size_type) const;
        reference at(size_type);
        const_reference at(size_type) const;
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
        bool empty() const noexcept;
        size_type size() const noexcept;
        ssize_type ssize() const noexcept;
        size_type max_size() const noexcept;
        void reserve(size_type);
        void reserve_front(size_type);
        void reserve_back(size_type);
        // Returns current storage + unused allocated storage before front()
        size_type capacity_front() const noexcept;
        // Returns current storage + unused allocated storage after back()
        size_type capacity_back() const noexcept;
        // Returns current storage + all unused allocated storage
        size_type capacity_full() const noexcept;
        // To achieve interface parity with std::vector, capacity() returns capacity_back();
        size_type capacity() const noexcept;
        void shrink_to_fit();

        // Modifiers
        void clear() noexcept;
        iterator insert(const_iterator, const T &);
        iterator insert(const_iterator, T &&);
        iterator insert(const_iterator, size_type, const T&);
        template <class InputIt> iterator insert(const_iterator, InputIt, InputIt);
        iterator insert(const_iterator, std::initializer_list<T>);
        template <class ... Args> iterator emplace(const_iterator, Args && ...);
        iterator erase(const_iterator);
        iterator erase(const_iterator, const_iterator);
        void push_back(const T &);
        void push_back(T &&);
        template <class ... Args> void emplace_back(Args && ... args);
        void push_front(const T &);
        void push_front(T &&);
        template <class ... Args> void emplace_front(Args && ... args);
        void pop_back();
        void pop_front();
        // Resizes the veque, by adding or removing from the front. 
        void resize_front(size_type);
        void resize_front(size_type, const T &);
        // Resizes the veque, by adding or removing from the back.
        void resize_back(size_type);
        void resize_back(size_type, const T &);
        // To achieve interface parity with std::vector, resize() performs resize_back();
        void resize(size_type);
        void resize(size_type, const T &);
        void swap(veque<T> &);

    private:
        size_type _size = 0;    // Numer of elements in use
        size_type _offset = 0;  // Element offset of begin()
        struct Data : Allocator // Employing EBO, since the allocator is frequently monostate.
        {
            Data( size_type capacity, const Allocator & alloc )
                : Allocator{alloc}
                , _allocated{capacity}
                , _storage{ this->allocate(capacity) }
            {}
            Data( nullptr_t ) {}
            Data( const Data& ) = delete;
            Data( Data&&o ) { std::swap(_allocated,o._allocated); std::swap(_storage,o._storage); }
            ~Data() { this->deallocate(_storage, _allocated); }
            Data& operator=( const Data& ) = delete;
            Data& operator=( Data&&o ) { std::swap(_allocated,o._allocated); std::swap(_storage,o._storage); return *this; };
            size_type _allocated = 0;
            T *_storage = nullptr;
        } _data;
        
        struct allocate_uninitialized_tag {};

        // Create an empty veque, with specified storage params
        veque( allocate_uninitialized_tag, size_type size, const Allocator& );
        // Create an empty veque, with specified storage params
        veque( size_type allocated, size_type offset, const Allocator& );
        // Destroy elements in range
        void priv_destroy( const_iterator begin, const_iterator end );
        // Construct elements in range
        template< typename ...Args >
        void priv_construct( const_iterator begin, const_iterator end, const Args & ...args );
        
        // Move vector to new storage, with default capacity for current size
        void priv_reallocate();
        void priv_reallocate( size_type allocated, size_type offset );
        // Insert empty space, choosing the most efficient way to shift existing elements
        iterator priv_insert_storage( const_iterator it, size_type count );
        // Moves a valid subrange in the front direction.
        // Vector will grow, if range moves past begin().
        // Vector will shrink if range includes end().
        // Returns iterator to beginning of destructed gap
        iterator priv_shift_front( const_iterator begin, const_iterator end, size_type count );
        // Moves a range towards the back.  Vector will grow, if needed.  Vacated elements are destructed.
        // Moves a valid subrange in the back direction.
        // Vector will grow, if range moves past end().
        // Vector will shrink if range includes begin().
        // Returns iterator to beginning of destructed gap
        iterator priv_shift_back( const_iterator begin, const_iterator end, size_type count );
    };

    template <typename T, typename Alloc>
    void veque<T,Alloc>::priv_destroy( const_iterator b, const_iterator e )
    {
        if constexpr ( ! std::is_trivially_destructible_v<T> )
        {
            for ( auto i = b; i != e; ++i )
            {
                i->~T();
            }        
        }
    }
    
    template <typename T, typename Alloc>
    template< typename ...Args >
    void veque<T,Alloc>::priv_construct( const_iterator b, const_iterator e, const Args & ...args )
    {
        for ( auto i = begin() + std::distance(cbegin(),b); i != e; ++i )
        {
            new(i) T(args...);
        }
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::priv_reallocate()
    {
        priv_reallocate( 3 * size() + 3, size() + 1 );
    }
    
    template <typename T, typename Alloc>
    void veque<T,Alloc>::priv_reallocate( veque<T,Alloc>::size_type allocated, veque<T,Alloc>::size_type offset  )
    {
        auto other = veque<T>( allocated, offset, static_cast<const Alloc &>(_data) );

        if ( size() )
        {
            if constexpr ( std::is_trivially_copyable_v<T> )
            {
                std::memcpy( other.begin(), begin(), size() * sizeof(T) );
            }
            else
            {
                auto dest = other.begin();
                for ( auto && src : *this )
                {
                    if constexpr ( std::is_nothrow_move_constructible_v<T> )
                    {
                        new(dest) T(std::move(src));
                    }
                    else
                    {
                        new(dest) T(src);
                    }
                    ++dest;
                }
            }
        }
        other._size = _size;
        swap( other );
    }
    
    template <typename T, typename Alloc>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::priv_shift_front( veque<T,Alloc>::const_iterator b, veque<T,Alloc>::const_iterator e, veque<T,Alloc>::size_type distance )
    {
        if ( e == begin() )
        {
            _offset -= distance;
            _size += distance;
            return begin();
        }
        auto element_count = std::distance( b, e );
        auto start = begin() + std::distance(cbegin(),b);
        if ( element_count )
        {
            if constexpr ( std::is_trivially_copyable_v<T> )
            {
                std::memmove( start - distance, start, element_count * sizeof(T) );
            }
            else
            {
                auto src = start; 
                auto dest = src - distance;
                auto dest_construct_end = std::min( cbegin(), e - distance );
                for ( ; dest < dest_construct_end; ++src, ++dest )
                {
                    // Construct to destinations before begin()
                    if constexpr ( std::is_nothrow_move_constructible_v<T> )
                    {
                        new(dest) T(std::move(*src));
                    }
                    else
                    {
                        new(dest) T(*src);
                    }
                }
                for ( ; src != e; ++src, ++dest )
                {
                    // Assign to destinations at or after begin()
                    if constexpr ( std::is_nothrow_move_assignable_v<T> )
                    {
                        *dest = std::move(*src);
                    }
                    else
                    {
                        *dest = *src;
                    }
                }
            }
        }
        priv_destroy( std::max( cbegin(), e - distance ), e );
        difference_type new_elements_at_front = cbegin() - b + distance;
        auto range_includes_end = (e == end());

        // If range includes end(), veque has shrunk
        if ( range_includes_end )
        {
            _size -= distance;
        }
        // Otherwise, if range moves before begin(), veque has grown
        else if ( new_elements_at_front > 0 )
        {
            _offset -= new_elements_at_front;
            _size += new_elements_at_front;
        }

        return begin() + std::distance(cbegin(),e) - distance;
    }
    
    template <typename T, typename Alloc>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::priv_shift_back( veque<T,Alloc>::const_iterator b, veque<T,Alloc>::const_iterator e, veque<T,Alloc>::size_type distance )
    {
        auto start = begin() + std::distance(cbegin(),b); 
        if ( b == end() )
        {
            _size += distance;
            return start;
        }
        auto element_count = std::distance( b, e );
        if ( element_count )
        {
            if constexpr ( std::is_trivially_copyable_v<T> )
            {
                std::memmove( start + distance, start, element_count * sizeof(T) );
            }
            else
            {
                auto src = begin() + std::distance( cbegin(), e-1 );
                auto dest = src + distance;
                auto dest_construct_end = std::max( end()-1, dest - element_count );
                for ( ; dest > dest_construct_end; --src, --dest )
                {
                    // Construct to destinations at or after end()
                    if constexpr ( std::is_nothrow_move_constructible_v<T> )
                    {
                        new(dest) T(std::move(*src));
                    }
                    else
                    {
                        new(dest) T(*src);
                    }
                }
                for ( ; src != b-1; --src, --dest )
                {
                    // Assign to destinations before before end()
                    if constexpr ( std::is_nothrow_move_assignable_v<T> )
                    {
                        *dest = std::move(*src);
                    }
                    else
                    {
                        *dest = *src;
                    }
                }
            }
        }
        priv_destroy( b, std::min( cend(), b + distance ) );
        difference_type new_elements_at_back = e - end() + distance;
        auto range_includes_begin = (b == begin());

        // If range moves before begin(), veque has grown
        if ( new_elements_at_back > 0 )
        {
            _size += new_elements_at_back;
        }
        // Otherwise, if range includes begin(), veque has shrunk
        else if ( range_includes_begin )
        {
            _offset += distance;
            _size -= distance;
        }
        
        return start;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::priv_insert_storage(typename veque<T,Alloc>::const_iterator it, size_type count)
    {
        auto required_size = size() + count;
        auto can_shift_back = capacity_back() >= required_size && it != begin();
        auto can_shift_front = capacity_front() >= required_size && it != end();
        
        if ( can_shift_back && can_shift_front)
        {
            // Capacity allows shifting in either direction.
            // Choose the direction with the fewest operations.
            if ( std::distance(cbegin(),it) < size() / 2 )
            {
                return priv_shift_front( begin(), it, count );
            }
            else
            {
                return priv_shift_back( it, end(), count );
            }
        }
        else if ( can_shift_back )
        {
            // Capacity only allows shifting back.
            return priv_shift_back( it, end(), count );
        }
        else if ( can_shift_front )
        {
            // Capacity only allows shifting front.
            return priv_shift_front( begin(), it, count );
        }
        else
        {
            // Insufficient capacity.  Allocate new storage.
            veque<T> other( required_size, static_cast<const Alloc &>(_data) );
            auto index = std::distance( cbegin(), it );
            if constexpr ( std::is_trivially_copyable_v<T> )
            {
                auto after_count = std::distance( it, cend() );
                std::memcpy( other.begin(), begin(), index * sizeof(T) );
                std::memcpy( other.begin() + index + count, it, after_count * sizeof(T) );
            }
            else
            {
                auto dest = other.begin();
                for ( auto src = begin(); src != it; ++src )
                {
                    if constexpr ( std::is_nothrow_move_constructible_v<T> )
                    {
                        new(dest) T(std::move(*src));
                    }
                    else
                    {
                        new(dest) T(*src);
                    }
                    ++dest;
                }
                dest += count;
                for ( auto src = it; src != end(); ++src )
                {
                    if constexpr ( std::is_nothrow_move_constructible_v<T> )
                    {
                        new(dest) T(std::move(*src));
                    }
                    else
                    {
                        new(dest) T(*src);
                    }
                    ++dest;
                }
            }
            other._size = required_size;
            swap(other);
            return begin() + index;
        }
    }
    
    template <typename T, typename Alloc>
    
    veque<T,Alloc>::veque() noexcept (noexcept(Alloc()))
        : _data { 0, Alloc{} }
    {
    }

    template <typename T, typename Alloc>
    veque<T,Alloc>::veque( size_type n, const Alloc& alloc )
        : veque{ allocate_uninitialized_tag{}, n, alloc }
    {
        priv_construct( begin(), end() );
    }

    template <typename T, typename Alloc>
    veque<T,Alloc>::veque( size_type n, const T &value, const Alloc& alloc )
        : veque{ allocate_uninitialized_tag{}, n, alloc }
    {
        priv_construct( begin(), end(), value );
    }

    template <typename T, typename Alloc>
    template <typename InputIt>
    veque<T,Alloc>::veque( InputIt first, InputIt last, const Alloc& alloc )
        : veque{ allocate_uninitialized_tag{}, veque<T,Alloc>::size_type(std::distance(first,last)), alloc }
    {
        for ( auto && dest : *this )
        {
            new(&dest) T(*first);
            ++first;
        }
    }

    template <typename T, typename Alloc>
    veque<T,Alloc>::veque( std::initializer_list<T> lst, const Alloc& alloc  )
        : veque{ allocate_uninitialized_tag{}, lst.size(), alloc }
    {
        auto dest = begin();
        for ( auto && src : lst )
        {
            new(dest) T(src);
            ++dest;
        }
    }

    template <typename T, typename Alloc>
    veque<T,Alloc>::veque( const veque &other, const Alloc& alloc )
        : _size{ other._size }
        , _offset{ 0 }
        , _data { other._size, alloc }
    {
        auto first = other.cbegin();
        for ( auto && val : *this )
        {
            new(&val) T(*first);
            ++first;
        }
    }

    template <typename T, typename Alloc>
    veque<T,Alloc>::veque(veque &&other) noexcept
        : _size{ 0 }
        , _offset{ 0 }
        , _data{ nullptr }
    {
        swap( other );
    }
     
    template <typename T, typename Alloc>
    veque<T,Alloc>::veque(veque &&other, const Alloc& alloc ) noexcept
        : _size{ 0 }
        , _offset{ 0 }
        , _data{ nullptr }
    {
        if ( _data == alloc )
        { 
            swap( other );
        }
        else
        {
            veque newvec( other.size(), alloc );
            if constexpr ( std::is_trivially_copyable_v<T> )
            {
                std::memcpy( newvec.begin(), other.begin(), other.size() * sizeof(T) );
            }
            else
            {
                auto dest = newvec.begin();
                for ( auto && src : other )
                {
                    if constexpr ( std::is_nothrow_move_constructible_v<T> )
                    {
                        new(dest) T(std::move(src));
                    }
                    else
                    {
                        new(dest) T(src);
                    }
                    ++dest;
                }
            }
            newvec._size = other.size();
            swap(newvec);            
        }
    }
     
    // Private impl for setting up custom storage
    template <typename T, typename Alloc>
    veque<T,Alloc>::veque( allocate_uninitialized_tag, size_type size, const Alloc & alloc )
        : _size{ size }
        , _offset{ size + 1 }
        , _data { size * 3 + 3, alloc }
    {
    }        
        
    // Private impl for setting up custom storage
    template <typename T, typename Alloc>
    veque<T,Alloc>::veque( size_type allocated, size_type _offset, const Alloc & alloc )
        : _size{ 0 }
        , _offset{ _offset }
        , _data { allocated, alloc }
    {
    }

    template <typename T, typename Alloc>
    veque<T,Alloc>::~veque()
    {
        priv_destroy( begin(), end() );
    }

    template <typename T, typename Alloc>
    veque<T> & veque<T,Alloc>::operator=( const veque<T> & other )
    {
        *this = veque<T>( other, static_cast<const Alloc &>(_data) );
        return *this;
    }

    template <typename T, typename Alloc>
    veque<T> & veque<T,Alloc>::operator=( veque<T> && other )
    {
        swap( other );
        return *this;
    }

    template <typename T, typename Alloc>
    veque<T> & veque<T,Alloc>::operator=( std::initializer_list<T> lst )
    {
        swap( veque<T>(lst), static_cast<const Alloc &>(_data) );
        return *this;
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::assign( typename veque<T,Alloc>::size_type count, const T & value )
    {
        *this = veque<T>( count,value,static_cast<const Alloc &>(_data) );
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::assign( typename veque<T,Alloc>::iterator first, veque<T,Alloc>::iterator last )
    {
        *this = veque<T>( first,last, static_cast<const Alloc &>(_data) );
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::assign(std::initializer_list<T> lst)
    {
        *this = veque<T>( lst, static_cast<const Alloc &>(_data) );
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::begin() noexcept
    {
        return _data._storage + _offset;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_iterator veque<T,Alloc>::begin() const noexcept
    {
        return cbegin();
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_iterator veque<T,Alloc>::cbegin() const noexcept
    {
        return _data._storage + _offset;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::end() noexcept
    {
        return _data._storage + _offset + size();
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_iterator veque<T,Alloc>::end() const noexcept
    {
        return cend();
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_iterator veque<T,Alloc>::cend() const noexcept
    {
        return _data._storage + _offset + size();
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::reverse_iterator veque<T,Alloc>::rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_reverse_iterator veque<T,Alloc>::rbegin() const noexcept
    {
        return crbegin();
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_reverse_iterator veque<T,Alloc>::crbegin() const noexcept
    {
        return reverse_iterator(cend());
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::reverse_iterator veque<T,Alloc>::rend() noexcept
    {
        return reverse_iterator(begin());
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_reverse_iterator veque<T,Alloc>::rend() const noexcept
    {
        return crend();
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_reverse_iterator veque<T,Alloc>::crend() const noexcept
    {
        return reverse_iterator(cbegin());
    }

    template <typename T, typename Alloc>
    bool veque<T,Alloc>::empty() const noexcept
    {
        return size() == 0;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::size_type veque<T,Alloc>::size() const noexcept
    {
        return _size;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::ssize_type veque<T,Alloc>::ssize() const noexcept
    {
        return _size;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::size_type veque<T,Alloc>::max_size() const noexcept
    {
        return std::numeric_limits<ssize_type>::max();
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::size_type veque<T,Alloc>::capacity_full() const noexcept
    {
        return _data._allocated;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::size_type veque<T,Alloc>::capacity_front() const noexcept
    {
        return _offset + size();
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::size_type veque<T,Alloc>::capacity_back() const noexcept
    {
        return capacity_full() - _offset;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::size_type veque<T,Alloc>::capacity() const noexcept
    {
        return capacity_back();
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::resize_back( veque<T,Alloc>::size_type count )
    {
        if ( count > size() )
        {
            if ( count > capacity_back() )
            {
                priv_reallocate( count * 3, count );
            }
            priv_construct( end(), begin() + count );
        }
        else
        {
            priv_destroy( begin() + count, end() );
        }
        _size = count;
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::resize_back( veque<T,Alloc>::size_type count, const T & value )
    {
        if ( count > size() )
        {
            if ( count > capacity_back() )
            {
                priv_reallocate( count * 3, count );
            }
            priv_construct( end(), begin() + count, value );
        }
        else
        {
            priv_destroy( begin() + count, end() );
        }
        _size = count;
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::resize_front( veque<T,Alloc>::size_type count )
    {
        difference_type delta = count - size();
        auto new_begin = begin() - delta;
        if ( delta > 0 )
        {
            if ( count > capacity_front() )
            {
                priv_reallocate( count * 3, count );
            }
            priv_construct( new_begin, begin() );
        }
        else
        {
            priv_destroy( begin(), new_begin );
        }
        _size = count;
        _offset -= delta;
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::resize_front( veque<T,Alloc>::size_type count, const T & value )
    {
        difference_type delta = count - size();
        auto new_begin = begin() - delta;
        if ( delta > 0 )
        {
            if ( count > capacity_front() )
            {
                priv_reallocate( count * 3, count );
            }
            priv_construct( new_begin, begin(), value );
        }
        else
        {
            priv_destroy( begin(), new_begin );
        }
        _size = count;
        _offset -= delta;
    }
    
    template <typename T, typename Alloc>
    void veque<T,Alloc>::resize( veque<T,Alloc>::size_type count )
    {
        resize_back( count );
    }
    
    
    template <typename T, typename Alloc>
    void veque<T,Alloc>::resize( veque<T,Alloc>::size_type count, const T & value )
    {
        resize_back( count, value );
    }
    
    template <typename T, typename Alloc>
    void veque<T,Alloc>::reserve_front( veque<T,Alloc>::size_type count )
    {
        if ( count > capacity_front() )
        {
            priv_reallocate( size() * 2 + count, count );
        }
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::reserve_back( veque<T,Alloc>::size_type count )
    {
        if ( count > capacity_back() )
        {
            priv_reallocate( size() * 2 + count, size() );
        }
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::reserve( veque<T,Alloc>::size_type count )
    {
        if ( count > capacity_front() || count > capacity_back() )
        {
            auto front_unallocated = std::max( capacity_front(), count ) - size();
            auto back_unallocated = std::max( capacity_back(), count ) - size();
            priv_reallocate( front_unallocated + size() + back_unallocated, front_unallocated );
        }
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::shrink_to_fit()
    {
        if ( size() < capacity_front() || size() < capacity_back() )
        {
            priv_reallocate( size(), 0 );
        }
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::reference veque<T,Alloc>::operator[]( veque<T,Alloc>::size_type idx)
    {
        return *(begin() + idx);
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_reference veque<T,Alloc>::operator[]( veque<T,Alloc>::size_type idx) const
    {
        return *(begin() + idx);
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::reference veque<T,Alloc>::at(size_type pos)
    {
        if ( pos >= size() )
        {
            throw std::out_of_range("veque<T>::at(size_type) out of range");
        }
        return (*this)[pos];
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_reference veque<T,Alloc>::at(size_type pos) const
    {
        if ( pos >= size() )
        {
            throw std::out_of_range("veque<T>::at(size_type) out of range");
        }
        return (*this)[pos];
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::reference veque<T,Alloc>::front()
    {
        return (*this)[0];
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_reference veque<T,Alloc>::front() const
    {
        return (*this)[0];
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::reference veque<T,Alloc>::back()
    {
        return (*this)[size() - 1];
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::const_reference veque<T,Alloc>::back() const
    {
        return (*this)[size() - 1];
    }

    template <typename T, typename Alloc>
    T * veque<T,Alloc>::data() noexcept
    {
        return &begin();
    }

    template <typename T, typename Alloc>
    const T * veque<T,Alloc>::data() const noexcept
    {
        return &cbegin();
    }

    template <typename T, typename Alloc>
    template <class ... Args>
    void veque<T,Alloc>::emplace_back(Args && ... args)
    {
        if ( size() == capacity_back() )
        {
            priv_reallocate();
        }
        new(end()) T(std::forward<Args>(args)...);
        ++_size;
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::push_back(const T &val)
    {
        if ( size() == capacity_back() )
        {
            priv_reallocate();
        }
        new(end()) T(val);
        ++_size;
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::push_back(T &&val)
    {
        if ( size() == capacity_back() )
        {
            priv_reallocate();
        }
        new(end()) T(std::move(val));
        ++_size;
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::pop_back()
    {
        back().~T();
        --_size;
    }

    template <typename T, typename Alloc>
    template <class ... Args>
    void veque<T,Alloc>::emplace_front(Args && ... args)
    {
        if ( size() == capacity_front() )
        {
            priv_reallocate();
        }
        new(begin()-1) T(std::forward<Args>(args) ...);
        ++_size;
        --_offset;
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::push_front(const T &val)
    {
        if ( size() == capacity_front() )
        {
            priv_reallocate();
        }
        new(begin()-1) T(val);
        ++_size;
        --_offset;
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::push_front(T &&val)
    {
        if ( size() == capacity_front() )
        {
            priv_reallocate();
        }
        new(begin()-1) T(std::move(val));
        ++_size;
        --_offset;
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::pop_front()
    {
        front().~T();
        --_size;
        ++_offset;
    }

    template <typename T, typename Alloc>
    template <class ... Args>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::emplace( typename veque<T,Alloc>::const_iterator it, Args && ... args )
    {
        auto res = priv_insert_storage( it, 1 );
        new (res) T( std::forward<Args>(args)... );
        return res;
    }
    
    template <typename T, typename Alloc>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::insert( typename veque<T,Alloc>::const_iterator it, const T &val )
    {
        auto res = priv_insert_storage( it, 1 );
        new (res) T( val );
        return res;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::insert( typename veque<T,Alloc>::const_iterator it, T &&val )
    {
        auto res = priv_insert_storage( it, 1 );
        new (res) T( std::move(val) );
        return res;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::insert( typename veque<T,Alloc>::const_iterator it,  veque<T,Alloc>::size_type cnt, const T &val )
    {
        auto res = priv_insert_storage( it, cnt );
        for ( iterator i = res; i != res + cnt; ++i)
            new(i) T(val);
        return res;
    }

    template <typename T, typename Alloc>
    template <class InputIt>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::insert( typename veque<T,Alloc>::const_iterator it, InputIt first, InputIt last )
    {
        auto res = priv_insert_storage( it, std::distance(first,last) );
        auto dest = res;
        for ( auto src = first; src != last; ++src, ++dest)
        {
            new(dest) T(*src);
        }
        return res;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::insert( typename veque<T,Alloc>::const_iterator it, std::initializer_list<T> lst )
    {
        auto res = priv_insert_storage( it, lst.size() );
        auto dest = res;
        for ( auto && src : lst  )
        {
            new(dest) T(src);
            ++dest;
        }
        return res;
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::erase(typename veque<T,Alloc>::const_iterator it)
    {
        return erase( it, it + 1 );
    }

    template <typename T, typename Alloc>
    typename veque<T,Alloc>::iterator veque<T,Alloc>::erase(typename veque<T,Alloc>::const_iterator first,  veque<T,Alloc>::const_iterator last)
    {
        auto elements_before = first - begin();
        auto elements_after = end() - last;
        auto count = std::distance( first, last );
        if (  elements_before < elements_after )
        {
            return priv_shift_back( begin(), first, count );
        }
        else
        {
            return priv_shift_front( last, end(), count );
        }
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::swap( veque<T> &other )
    {
        std::swap( _size,      other._size );
        std::swap( _offset,    other._offset );
        std::swap( _data,      other._data );
    }

    template <typename T, typename Alloc>
    void veque<T,Alloc>::clear() noexcept
    {
        priv_destroy( begin(), end() );
        _size = 0;
        _offset = capacity_full() / 2;
    }

    template <typename T, typename Alloc>
    bool operator==(const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs)
    {
        if ( lhs.size() != rhs.size() )
        {
            return false;
        }
        return std::equal( lhs.cbegin(), lhs.cend(), rhs.cbegin() );
    }

    template <typename T, typename Alloc>
    bool operator!=(const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs)
    {
        return !( lhs == rhs );
    }

    template <typename T, typename Alloc>
    bool operator<(const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs)
    {
        return std::lexicographical_compare( lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend() );
    }

    template <typename T, typename Alloc>
    bool operator<=(const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs)
    {
        return !( rhs < lhs );
    }

    template <typename T, typename Alloc>
    bool operator>(const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs)
    {
        return ( rhs < lhs );
    }

    template <typename T, typename Alloc>
    bool operator>=(const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs)
    {
        return !( lhs < rhs );
    }
    
    
namespace std
{
  template <typename T>
  struct hash<veque<T>>
  {
    size_t operator()( const veque<T> & v ) const
    {
        size_t hash = 0;
        auto hasher = std::hash<T>{};
        for ( const auto & val : v )
        {
            hash ^= hasher(val) + 0x9e3779b9 + (hash<<6) + (hash>>2);
        }
        return hash;
    }
  };
}

#endif