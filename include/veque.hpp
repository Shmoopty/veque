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
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>


    template <typename T>
    class veque {
    public:
        // Types
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
        veque() noexcept;
        explicit veque(size_type n);
        veque(size_type n, const T &val);
        template <typename InputIt>
        veque(InputIt first,  InputIt last);
        veque(std::initializer_list<T>);
        veque(const veque<T> &);
        veque(veque<T> &&) noexcept;
        ~veque();
        veque<T> & operator=(const veque<T> &);
        veque<T> & operator=(veque<T> &&);
        veque<T> & operator=(std::initializer_list<T>);
        void assign(size_type, const T &value);
        void assign(typename veque<T>::iterator,  veque<T>::iterator);
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
        const_iterator cbegin() const noexcept;
        iterator end() noexcept;
        const_iterator cend() const noexcept;
        reverse_iterator rbegin() noexcept;
        const_reverse_iterator crbegin() const noexcept;
        reverse_iterator rend() noexcept;
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
        size_type _size = 0;
        size_type _allocated = 0;
        size_type _offset = 0;
        std::byte *_data = nullptr;

        struct allocate_uninitialized_tag {};

        // Create an empty veque, with specified storage params
        veque( allocate_uninitialized_tag, size_type size );
        // Create an empty veque, with specified storage params
        veque( size_type allocated, size_type offset );
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

    template <typename T>
    void veque<T>::priv_destroy( const_iterator b, const_iterator e )
    {
        if constexpr ( ! std::is_trivially_destructible_v<T> )
        {
            for ( auto i = b; i != e; ++i )
            {
                i->~T();
            }        
        }
    }
    
    template <typename T>
    template< typename ...Args >
    void veque<T>::priv_construct( const_iterator b, const_iterator e, const Args & ...args )
    {
        for ( auto i = begin() + std::distance(cbegin(),b); i != e; ++i )
        {
            new(i) T(args...);
        }
    }

    template <typename T>
    void veque<T>::priv_reallocate()
    {
        priv_reallocate( 3 * size() + 3, size() + 1 );
    }
    
    template <typename T>
    void veque<T>::priv_reallocate( veque<T>::size_type allocated, veque<T>::size_type offset  )
    {
        auto other = veque<T>( allocated, offset );

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
    
    template <typename T>
    typename veque<T>::iterator veque<T>::priv_shift_front( veque<T>::const_iterator b, veque<T>::const_iterator e, veque<T>::size_type distance )
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
    
    template <typename T>
    typename veque<T>::iterator veque<T>::priv_shift_back( veque<T>::const_iterator b, veque<T>::const_iterator e, veque<T>::size_type distance )
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

    template <typename T>
    typename veque<T>::iterator veque<T>::priv_insert_storage(typename veque<T>::const_iterator it, size_type count)
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
            veque<T> other( required_size * 3, required_size );
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
    
    template <typename T>
    
    veque<T>::veque() noexcept
        : _data { reinterpret_cast<std::byte*>( std::aligned_alloc( alignof(T), sizeof(T) * capacity_full() ) ) }
    {
    }

    template <typename T>
    veque<T>::veque( veque<T>::size_type n )
        : veque{ allocate_uninitialized_tag{}, n }
    {
        priv_construct( begin(), end() );
    }

    template <typename T>
    veque<T>::veque(veque<T>::size_type n, const T &value)
        : veque{ allocate_uninitialized_tag{}, n }
    {
        priv_construct( begin(), end(), value );
    }

    template <typename T>
    template <typename InputIt>
    veque<T>::veque(InputIt first, InputIt last)
        : veque{ allocate_uninitialized_tag{}, veque<T>::size_type(std::distance(first,last)) }
    {
        for ( auto && dest : *this )
        {
            new(&dest) T(*first);
            ++first;
        }
    }

    template <typename T>
    veque<T>::veque( std::initializer_list<T> lst )
        : veque{ allocate_uninitialized_tag{}, lst.size() }
    {
        auto dest = begin();
        for ( auto && src : lst )
        {
            new(dest) T(src);
            ++dest;
        }
    }

    template <typename T>
    veque<T>::veque(const veque<T> &other)
        : _size{ other._size }
        , _allocated{ other._allocated }
        , _offset{ other._offset }
        , _data { reinterpret_cast<std::byte*>( std::aligned_alloc( alignof(T), sizeof(T) * capacity_full() ) ) }
    {
        auto first = other.cbegin();
        for ( auto && val : *this )
        {
            new(&val) T(*first);
            ++first;
        }
    }

    template <typename T>
    veque<T>::veque(veque<T> &&other) noexcept
        : _size{ 0 }
        , _allocated{ 0 }
        , _offset{ 0 }
        , _data{ nullptr }
    {
        swap( other );
    }
     
    // Private impl for setting up custom storage
    template <typename T>
    veque<T>::veque( allocate_uninitialized_tag, size_type size )
        : _size{ size }
        , _allocated{ size * 3 + 3 }
        , _offset{ size + 1 }
        , _data { reinterpret_cast<std::byte*>( std::aligned_alloc( alignof(T), sizeof(T) * capacity_full() ) ) }
    {
    }        
        
    // Private impl for setting up custom storage
    template <typename T>
    veque<T>::veque( size_type _allocated, size_type _offset )
        : _size{ 0 }
        , _allocated{ _allocated }
        , _offset{ _offset }
        , _data { reinterpret_cast<std::byte*>( std::aligned_alloc( alignof(T), sizeof(T) * capacity_full() ) ) }
    {
    }

    template <typename T>
    veque<T>::~veque()
    {
        priv_destroy( begin(), end() );
        std::free( _data ); 
    }

    template <typename T>
    veque<T> & veque<T>::operator=( const veque<T> & other )
    {
        *this = veque<T>(other);
        return *this;
    }

    template <typename T>
    veque<T> & veque<T>::operator=( veque<T> && other )
    {
        swap( other );
        return *this;
    }

    template <typename T>
    veque<T> & veque<T>::operator=( std::initializer_list<T> lst )
    {
        swap( veque<T>(lst) );
        return *this;
    }

    template <typename T>
    void veque<T>::assign( typename veque<T>::size_type count, const T & value )
    {
        *this = veque<T>(count,value);
    }

    template <typename T>
    void veque<T>::assign( typename veque<T>::iterator first, veque<T>::iterator last )
    {
        *this = veque<T>(first,last);
    }

    template <typename T>
    void veque<T>::assign(std::initializer_list<T> lst)
    {
        *this = veque<T>(lst);
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::begin() noexcept
    {
        return reinterpret_cast<iterator>( _data ) + _offset;
    }

    template <typename T>
    typename veque<T>::const_iterator veque<T>::cbegin() const noexcept
    {
        return reinterpret_cast<iterator>( _data ) + _offset;
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::end() noexcept
    {
        return reinterpret_cast<iterator>( _data ) + _offset + size();
    }

    template <typename T>
    typename veque<T>::const_iterator veque<T>::cend() const noexcept
    {
        return reinterpret_cast<iterator>( _data ) + _offset + size();
    }

    template <typename T>
    typename veque<T>::reverse_iterator veque<T>::rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    template <typename T>
    typename veque<T>::const_reverse_iterator veque<T>::crbegin() const noexcept
    {
        return reverse_iterator(cend());
    }

    template <typename T>
    typename veque<T>::reverse_iterator veque<T>::rend() noexcept
    {
        return reverse_iterator(begin());
    }

    template <typename T>
    typename veque<T>::const_reverse_iterator veque<T>::crend() const noexcept
    {
        return reverse_iterator(cbegin());
    }

    template <typename T>
    bool veque<T>::empty() const noexcept
    {
        return size() == 0;
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::size() const noexcept
    {
        return _size;
    }

    template <typename T>
    typename veque<T>::ssize_type veque<T>::ssize() const noexcept
    {
        return _size;
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::max_size() const noexcept
    {
        return std::numeric_limits<ssize_type>::max();
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::capacity_full() const noexcept
    {
        return _allocated;
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::capacity_front() const noexcept
    {
        return _offset + size();
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::capacity_back() const noexcept
    {
        return capacity_full() - _offset;
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::capacity() const noexcept
    {
        return capacity_back();
    }

    template <typename T>
    void veque<T>::resize_back( veque<T>::size_type count )
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

    template <typename T>
    void veque<T>::resize_back( veque<T>::size_type count, const T & value )
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

    template <typename T>
    void veque<T>::resize_front( veque<T>::size_type count )
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

    template <typename T>
    void veque<T>::resize_front( veque<T>::size_type count, const T & value )
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
    
    template <typename T>
    void veque<T>::resize( veque<T>::size_type count )
    {
        resize_back( count );
    }
    
    
    template <typename T>
    void veque<T>::resize( veque<T>::size_type count, const T & value )
    {
        resize_back( count, value );
    }
    
    template <typename T>
    void veque<T>::reserve_front( veque<T>::size_type count )
    {
        if ( count > capacity_front() )
        {
            priv_reallocate( size() * 2 + count, count );
        }
    }

    template <typename T>
    void veque<T>::reserve_back( veque<T>::size_type count )
    {
        if ( count > capacity_back() )
        {
            priv_reallocate( size() * 2 + count, size() );
        }
    }

    template <typename T>
    void veque<T>::reserve( veque<T>::size_type count )
    {
        if ( count > capacity_front() || count > capacity_back() )
        {
            auto front_unallocated = std::max( capacity_front(), count ) - size();
            auto back_unallocated = std::max( capacity_back(), count ) - size();
            priv_reallocate( front_unallocated + size() + back_unallocated, front_unallocated );
        }
    }

    template <typename T>
    void veque<T>::shrink_to_fit()
    {
        if ( size() < capacity_front() || size() < capacity_back() )
        {
            priv_reallocate( size(), 0 );
        }
    }

    template <typename T>
    typename veque<T>::reference veque<T>::operator[]( veque<T>::size_type idx)
    {
        return *(begin() + idx);
    }

    template <typename T>
    typename veque<T>::const_reference veque<T>::operator[]( veque<T>::size_type idx) const
    {
        return *(begin() + idx);
    }

    template <typename T>
    typename veque<T>::reference veque<T>::at(size_type pos)
    {
        if ( pos >= size() )
        {
            throw std::out_of_range("veque<T>::at(size_type) out of range");
        }
        return (*this)[pos];
    }

    template <typename T>
    typename veque<T>::const_reference veque<T>::at(size_type pos) const
    {
        if ( pos >= size() )
        {
            throw std::out_of_range("veque<T>::at(size_type) out of range");
        }
        return (*this)[pos];
    }

    template <typename T>
    typename veque<T>::reference veque<T>::front()
    {
        return (*this)[0];
    }

    template <typename T>
    typename veque<T>::const_reference veque<T>::front() const
    {
        return (*this)[0];
    }

    template <typename T>
    typename veque<T>::reference veque<T>::back()
    {
        return (*this)[size() - 1];
    }

    template <typename T>
    typename veque<T>::const_reference veque<T>::back() const
    {
        return (*this)[size() - 1];
    }

    template <typename T>
    T * veque<T>::data() noexcept
    {
        return &begin();
    }

    template <typename T>
    const T * veque<T>::data() const noexcept
    {
        return &cbegin();
    }

    template <typename T>
    template <class ... Args>
    void veque<T>::emplace_back(Args && ... args)
    {
        if ( size() == capacity_back() )
        {
            priv_reallocate();
        }
        new(end()) T(std::forward<Args>(args)...);
        ++_size;
    }

    template <typename T>
    void veque<T>::push_back(const T &val)
    {
        if ( size() == capacity_back() )
        {
            priv_reallocate();
        }
        new(end()) T(val);
        ++_size;
    }

    template <typename T>
    void veque<T>::push_back(T &&val)
    {
        if ( size() == capacity_back() )
        {
            priv_reallocate();
        }
        new(end()) T(std::move(val));
        ++_size;
    }

    template <typename T>
    void veque<T>::pop_back()
    {
        back().~T();
        --_size;
    }

    template <typename T>
    template <class ... Args>
    void veque<T>::emplace_front(Args && ... args)
    {
        if ( size() == capacity_front() )
        {
            priv_reallocate();
        }
        new(begin()-1) T(std::forward<Args>(args) ...);
        ++_size;
        --_offset;
    }

    template <typename T>
    void veque<T>::push_front(const T &val)
    {
        if ( size() == capacity_front() )
        {
            priv_reallocate();
        }
        new(begin()-1) T(val);
        ++_size;
        --_offset;
    }

    template <typename T>
    void veque<T>::push_front(T &&val)
    {
        if ( size() == capacity_front() )
        {
            priv_reallocate();
        }
        new(begin()-1) T(std::move(val));
        ++_size;
        --_offset;
    }

    template <typename T>
    void veque<T>::pop_front()
    {
        front().~T();
        --_size;
        ++_offset;
    }

    template <typename T>
    template <class ... Args>
    typename veque<T>::iterator veque<T>::emplace( typename veque<T>::const_iterator it, Args && ... args )
    {
        auto res = priv_insert_storage( it, 1 );
        new (res) T( std::forward<Args>(args)... );
        return res;
    }
    
    template <typename T>
    typename veque<T>::iterator veque<T>::insert( typename veque<T>::const_iterator it, const T &val )
    {
        auto res = priv_insert_storage( it, 1 );
        new (res) T( val );
        return res;
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::insert( typename veque<T>::const_iterator it, T &&val )
    {
        auto res = priv_insert_storage( it, 1 );
        new (res) T( std::move(val) );
        return res;
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::insert( typename veque<T>::const_iterator it,  veque<T>::size_type cnt, const T &val )
    {
        auto res = priv_insert_storage( it, cnt );
        for ( iterator i = res; i != res + cnt; ++i)
            new(i) T(val);
        return res;
    }

    template <typename T>
    template <class InputIt>
    typename veque<T>::iterator veque<T>::insert( typename veque<T>::const_iterator it, InputIt first, InputIt last )
    {
        auto res = priv_insert_storage( it, std::distance(first,last) );
        auto dest = res;
        for ( auto src = first; src != last; ++src, ++dest)
        {
            new(dest) T(*src);
        }
        return res;
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::insert( typename veque<T>::const_iterator it, std::initializer_list<T> lst )
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

    template <typename T>
    typename veque<T>::iterator veque<T>::erase(typename veque<T>::const_iterator it)
    {
        return erase( it, it + 1 );
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::erase(typename veque<T>::const_iterator first,  veque<T>::const_iterator last)
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

    template <typename T>
    void veque<T>::swap( veque<T> &other )
    {
        std::swap( _size,      other._size );
        std::swap( _allocated, other._allocated );
        std::swap( _offset,    other._offset );
        std::swap( _data,      other._data );
    }

    template <typename T>
    void veque<T>::clear() noexcept
    {
        priv_destroy( begin(), end() );
        _size = 0;
        _offset = capacity_full() / 2;
    }

    template <typename T>
    bool operator==(const veque<T> &lhs, const veque<T> &rhs)
    {
        if ( lhs.size() != rhs.size() )
        {
            return false;
        }
        return std::equal( lhs.cbegin(), lhs.cend(), rhs.cbegin() );
    }

    template <typename T>
    bool operator!=(const veque<T> &lhs, const veque<T> &rhs)
    {
        return !( lhs == rhs );
    }

    template <typename T>
    bool operator<(const veque<T> &lhs, const veque<T> &rhs)
    {
        return std::lexicographical_compare( lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend() );
    }

    template <typename T>
    bool operator<=(const veque<T> &lhs, const veque<T> &rhs)
    {
        return !( rhs < lhs );
    }

    template <typename T>
    bool operator>(const veque<T> &lhs, const veque<T> &rhs)
    {
        return ( rhs < lhs );
    }

    template <typename T>
    bool operator>=(const veque<T> &lhs, const veque<T> &rhs)
    {
        return !( lhs < rhs );
    }
    
    
namespace std
{
  template <typename T>
  struct hash<veque<T>
  {
    size_t operator()( const veque & v ) const
    {
        size_t hash = 0;
        auto hasher = std::hash<T>{};
        for ( auto && val : v )
        {
            hash ^= hasher(v) + 0x9e3779b9 + (hash<<6) + (hash>>2);
        }
        return hash;
    }
  };
}

#endif