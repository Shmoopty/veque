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
#include <limits>
#include <stdexcept>
#include <utility>


    template <typename T>
    class veque {
    public:
        // types:
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

        // Common member functions:
        veque() noexcept;
        explicit veque(size_type n);
        veque(size_type n, const T &val);
        veque(typename veque<T>::iterator first,  veque<T>::iterator last);
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
        
        // Iterators:
        iterator begin() noexcept;
        const_iterator cbegin() const noexcept;
        iterator end() noexcept;
        const_iterator cend() const noexcept;
        reverse_iterator rbegin() noexcept;
        const_reverse_iterator crbegin() const noexcept;
        reverse_iterator rend() noexcept;
        const_reverse_iterator crend() const noexcept;

        // Capacity:
        bool empty() const noexcept;
        size_type size() const noexcept;
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
        size_type reserved = 12;
        size_type _offset = 0;
        std::byte *_data = nullptr;

        // Create an empty veque, with specified storage params
        veque( size_type reserved, size_type _offset );
        // Insert empty space, choosing the most efficient way to shift existing elements
        iterator insert_empty_space( const_iterator it, size_type count );
        // Moves a range towards the front.  Vector will grow, if needed.  Vacated elements are destructed.
        void shift_front( iterator begin, iterator end, size_type count );
        // Moves a range towards the back.  Vector will grow, if needed.  Vacated elements are destructed.
        void shift_back( iterator begin, iterator end, size_type count );
    };

    template <typename T>
    void veque<T>::shift_front( veque<T>::iterator b, veque<T>::iterator e, veque<T>::size_type count )
    {
        for ( auto i = b; i != e; ++i )
        {
            auto dest = i - count;
            if ( dest < begin() )
            {
                new(dest) T(std::move(*i));
            }
            else
            {
                *dest = std::move(*i);
            }
        }
        
        auto new_elements_at_front = b - begin() + count;
        if ( new_elements_at_front > 0 )
        {
            _offset -= new_elements_at_front;
            _size += new_elements_at_front;
        }
        
        if ( e == end() )
        {
            _size -= count;
        }

        for ( auto i = e; i != e - count; --i )
        {
            i->~T();
        }
    }

    template <typename T>
    void veque<T>::shift_back( veque<T>::iterator b, veque<T>::iterator e, veque<T>::size_type count )
    {
        for ( auto i = e - 1; i != b - 1; --i )
        {
            auto dest = i + count;
            if ( dest >= end() )
            {
                new(dest) T(std::move(*i));
            }
            else
            {
                *dest = std::move(*i);
            }
        }
        
        if ( b == begin() )
        {
            _offset += count;
            _size -= count;
        }
        
        auto new_elements_at_end = end() - e + count;
        if ( new_elements_at_end > 0 )
        {
            _size += new_elements_at_end;
        }

        for ( auto i = b; i != b + count; ++i )
        {
            i->~T();
        }
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::insert_empty_space(typename veque<T>::const_iterator it, size_type count)
    {
        if ( capacity_back() >= size() + count )
        {
            if ( capacity_front() >= size() + count )
            {
                if ( it - begin() < size() / 2 )
                {
                    shift_front( begin(), it, count );
                }
                else
                {
                    shift_back( it, end(), count );
                }
            }
            else
            {
                shift_back( it, end(), count );
            }
        }
        else if ( capacity_front() >= size() + count )
        {
            shift_front( begin(), it, count );
        }
        else
        {
            auto new_size = size() + 1;
            auto index = it - begin();
            veque<T> other( new_size * 3, new_size );
            move( begin(), it, other.begin() );
            move( it, end(), other.begin() + index + 1 );
            other._size = new_size;
            swap(other);
            return begin() + index;
        }
        return it;
    }
    
    template <typename T>
    veque<T>::veque() noexcept
        : _data { new std::byte[sizeof(T) * reserved] }
    {
    }

    template <typename T>
    veque<T>::veque( veque<T>::size_type n )
        : _size{ n }
        , reserved{ _size * 3 }
        , _offset{ _size }
        , _data{ new std::byte[sizeof(T) * reserved] }
    {
        for ( auto & val : *this )
        {
            new(&val) T;
        }
    }

    template <typename T>
    veque<T>::veque(veque<T>::size_type n, const T &value)
        : _size{ n }
        , reserved{ _size * 3 }
        , _offset{ _size }
        , _data{ new std::byte[sizeof(T) * reserved] }
    {
        for ( auto & val : *this )
        {
            new(&val) T(value);
        }
    }

    template <typename T>
    veque<T>::veque(veque<T>::iterator first, veque<T>::iterator last)
        : _size{ last - first }
        , reserved{ _size * 3 }
        , _offset{ _size }
        , _data{ new std::byte[sizeof(T) * reserved] }
    {
        for ( auto & val : *this )
        {
            new(&val) T(*first);
            ++first;
        }
    }

    template <typename T>
    veque<T>::veque( std::initializer_list<T> lst )
        : _size{ lst.size() }
        , reserved{ _size * 3 }
        , _offset{ _size }
        , _data{ new std::byte[sizeof(T) * reserved] }
    {
        auto first = lst.begin();
        for ( auto & val : *this )
        {
            new(&val) T(*first);
            ++first;
        }
    }

    template <typename T>
    veque<T>::veque(const veque<T> &other)
        : _size{ other._size }
        , reserved{ other.reserved }
        , _offset{ other._offset }
        , _data{ new std::byte[sizeof(T) * reserved] }
    {
        auto first = other.begin();
        for ( auto & val : *this )
        {
            new(&val) T(*first);
            ++first;
        }
    }

    template <typename T>
    veque<T>::veque(veque<T> &&other) noexcept
        : _size{ 0 }
        , reserved{ 0 }
        , _offset{ 0 }
        , _data{ nullptr }
    {
        swap( other );
    }
     
    // Private impl for setting up custom storage
    template <typename T>
    veque<T>::veque( size_type reserved, size_type _offset )
        : _size{ 0 }
        , reserved{ reserved }
        , _offset{ _offset }
        , _data{ new std::byte[sizeof(T) * reserved] }
    {
    }

    template <typename T>
    veque<T>::~veque()
    {
        for ( auto & val : *this )
        {
            val.~T();
        }
        delete[] _data; 
    }

    template <typename T>
    veque<T> & veque<T>::operator=(const veque<T> &other)
    {
        swap( veque<T>(other) );
        return *this;
    }

    template <typename T>
    veque<T> & veque<T>::operator=(veque<T> &&other)
    {
        swap( other );
        return *this;
    }

    template <typename T>
    veque<T> & veque<T>::operator=(std::initializer_list<T> lst)
    {
        swap( veque<T>(lst) );
        return *this;
    }

    template <typename T>
    void veque<T>::assign(typename veque<T>::size_type count, const T &value)
    {
        swap( veque<T>(count,value) );
    }

    template <typename T>
    void veque<T>::assign(typename veque<T>::iterator first,  veque<T>::iterator last)
    {
        swap( veque<T>(first,last) );
    }

    template <typename T>
    void veque<T>::assign(std::initializer_list<T> lst)
    {
        *this = lst;
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::begin() noexcept
    {
        return reinterpret_cast<T*>( _data + (sizeof(T) * _offset ) );
    }

    template <typename T>
    typename veque<T>::const_iterator veque<T>::cbegin() const noexcept
    {
        return reinterpret_cast<const T*>( _data + (sizeof(T) * _offset ) );
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::end() noexcept
    {
        return reinterpret_cast<T*>( _data + (sizeof(T) * _offset ) + (sizeof(T) * _size) );
    }

    template <typename T>
    typename veque<T>::const_iterator veque<T>::cend() const noexcept
    {
        return reinterpret_cast<const T*>( _data + (sizeof(T) * _offset ) + (sizeof(T) * _size) );
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
        return _size == 0;
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::size() const noexcept
    {
        return _size;
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max();
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::capacity_full() const noexcept
    {
        return reserved;
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::capacity_front() const noexcept
    {
        return _offset + _size;
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::capacity_back() const noexcept
    {
        return reserved - _offset;
    }

    template <typename T>
    typename veque<T>::size_type veque<T>::capacity() const noexcept
    {
        return capacity_back();
    }

    template <typename T>
    void veque<T>::resize_back( veque<T>::size_type count )
    {
        if ( count > _size )
        {
            const auto add_count = count - size();
            if ( count > capacity_back() )
            {
                veque<T> other( count * 3, count );
                for ( auto && element : *this )
                {
                    other.push_back( std::move(element) );
                }
                swap(other);
            }
            for ( size_type i = 0; i != add_count; ++ i )
            {
                emplace_back();
            }
        }
        else
        {
            const auto remove_count = size() - count;
            for ( size_type i = 0; i != remove_count; ++ i )
            {
                pop_back();
            }
        }
    }

    template <typename T>
    void veque<T>::resize_back( veque<T>::size_type count, const T & value )
    {
        if ( count > _size )
        {
            const auto add_count = count - size();
            if ( count > capacity_back() )
            {
                veque<T> other( count * 3, count );
                for ( auto && element : *this )
                {
                    other.push_back( std::move(element) );
                }
                swap(other);
            }
            for ( size_type i = 0; i != add_count; ++ i )
            {
                push_back( value );
            }
        }
        else
        {
            const auto remove_count = size() - count;
            for ( size_type i = 0; i != remove_count; ++ i )
            {
                pop_back();
            }
        }
    }

    
    template <typename T>
    void veque<T>::resize_front( veque<T>::size_type count )
    {
        if ( count > _size )
        {
            const auto add_count = count - size();
            if ( count > capacity_back() )
            {
                veque<T> other( count * 3, count * 2 );
                for ( auto && element : *this )
                {
                    other.push_front( std::move(element) );
                }
                swap(other);
            }
            for ( size_type i = 0; i != add_count; ++ i )
            {
                emplace_front();
            }
        }
        else
        {
            const auto remove_count = size() - count;
            for ( size_type i = 0; i != remove_count; ++ i )
            {
                pop_front();
            }
        }
    }
    
    
    template <typename T>
    void veque<T>::resize_front( veque<T>::size_type count, const T & value )
    {
        if ( count > _size )
        {
            const auto add_count = count - size();
            if ( count > capacity_back() )
            {
                veque<T> other( count * 3, count * 2 );
                for ( auto && element : *this )
                {
                    other.push_front( std::move(element) );
                }
                swap(other);
            }
            for ( size_type i = 0; i != add_count; ++ i )
            {
                push_front( value );
            }
        }
        else
        {
            const auto remove_count = size() - count;
            for ( size_type i = 0; i != remove_count; ++ i )
            {
                pop_front();
            }
        }
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
            veque<T> other( count + size() * 2, count );
            for ( auto && element : *this )
            {
                other.push_back( std::move(element) );
            }
            swap(other);
        }
    }

    template <typename T>
    void veque<T>::reserve_back( veque<T>::size_type count )
    {
        if ( count > capacity_back() )
        {
            veque<T> other( count + size() * 2, _offset );
            for ( auto && element : *this )
            {
                other.push_back( std::move(element) );
            }
            swap(other);
        }
    }

    template <typename T>
    void veque<T>::reserve( veque<T>::size_type count )
    {
        if ( count > capacity_front() || count > capacity_back() )
        {
            veque<T> other( count * 2 + size(), count );
            for ( auto && element : *this )
            {
                other.push_back( std::move(element) );
            }
            swap(other);
        }
    }

    template <typename T>
    void veque<T>::shrink_to_fit()
    {
        if ( size() < capacity_front() || size() < capacity_back() )
        {
            veque<T> other( size(), 0 );
            for ( auto && element : *this )
            {
                other.push_back( std::move(element) );
            }
            swap(other);
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
        if (pos < _size)
            return (*this)[pos];
        else
            throw std::out_of_range("veque<T>::at(size_type) out of range");
    }

    template <typename T>
    typename veque<T>::const_reference veque<T>::at(size_type pos) const
    {
        if (pos < _size)
            return (*this)[pos];
        else
            throw std::out_of_range("veque<T>::at(size_type) out of range");
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
        return (*this)[_size - 1];
    }

    template <typename T>
    typename veque<T>::const_reference veque<T>::back() const
    {
        return (*this)[_size - 1];
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
            *this = veque<T>( begin(), end() );
        }
        ++_size;
        new(&back()) T(std::forward<Args>(args) ...);
    }

    template <typename T>
    void veque<T>::push_back(const T &val)
    {
        if ( size() == capacity_back() )
        {
            *this = veque<T>( begin(), end() );
        }
        ++_size;
        new(&back()) T(val);
    }

    template <typename T>
    void veque<T>::push_back(T &&val)
    {
        if ( size() == capacity_back() )
        {
            *this = veque<T>( begin(), end() );
        }
        ++_size;
        new(&back()) T(std::move(val));
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
            *this = veque<T>( begin(), end() );
        }
        ++_size;
        --_offset;
        new(&front()) T(std::forward<Args>(args) ...);
    }

    template <typename T>
    void veque<T>::push_front(const T &val)
    {
        if ( size() == capacity_front() )
        {
            *this = veque<T>( begin(), end() );
        }
        ++_size;
        --_offset;
        new(&front()) T(val);
    }

    template <typename T>
    void veque<T>::push_front(T &&val)
    {
        if ( size() == capacity_front() )
        {
            *this = veque<T>( begin(), end() );
        }
        ++_size;
        --_offset;
        new(&front()) T(std::move(val));
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
    typename veque<T>::iterator veque<T>::emplace(typename veque<T>::const_iterator it, Args && ... args)
    {
        it = insert_empty_space( it, 1 );
        new (it) T( std::forward<Args>(args)... );
        return it;
    }
    
    template <typename T>
    typename veque<T>::iterator veque<T>::insert(typename veque<T>::const_iterator it, const T &val)
    {
        it = insert_empty_space( it, 1 );
        new (it) T( val );
        return it;
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::insert(typename veque<T>::const_iterator it, T &&val)
    {
        it = insert_empty_space( it, 1 );
        new (it) T( std::move(val) );
        return it;
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::insert(typename veque<T>::const_iterator it,  veque<T>::size_type cnt, const T &val)
    {
        it = insert_empty_space( it, cnt );
        for ( iterator i = it; cnt--; ++i)
            *i = val;
        return it;
    }

    template <typename T>
    template <class InputIt>
    typename veque<T>::iterator veque<T>::insert(typename veque<T>::const_iterator it, InputIt first, InputIt last)
    {
        it = insert_empty_space( it, std::distance(first,last) );
        for ( iterator i = it; first != last; ++i)
        {
            *i = first;
            ++first;
        }
        return it;
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::insert(typename veque<T>::const_iterator it, std::initializer_list<T> lst)
    {
        it = insert_empty_space( it, lst.size() );
        auto i = it;
        for ( auto const & val : lst  )
        {
            *i = val;
            ++i;
        }
        return it;
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::erase(typename veque<T>::const_iterator it)
    {
        return erase( it, it + 1 );
    }

    template <typename T>
    typename veque<T>::iterator veque<T>::erase(typename veque<T>::const_iterator first,  veque<T>::const_iterator last)
    {
        if ( first - begin() < end() - last )
        {
            shift_back( begin(), first, last - first );
            return last;
        }
        else
        {
            shift_front( last, end(), last - first );
            return first;
        }
    }

    template <typename T>
    void veque<T>::swap( veque<T> &other )
    {
        std::swap( _size, other._size );
        std::swap( reserved, other.reserved );
        std::swap( _offset, other._offset );
        std::swap( _data, other._data );
    }

    template <typename T>
    void veque<T>::clear() noexcept
    {
        for ( auto &val : *this )
        {
            val.~T();
        }
        _size = 0;
        _offset = reserved / 2;
    }

    template <typename T>
    bool operator==(const veque<T> &lhs, const veque<T> &rhs)
    {
        if ( lhs.size() != rhs.size() )
        {
            return false;
        }
        return std::equal( lhs.begin(), lhs.end(), rhs.begin() );
    }

    template <typename T>
    bool operator!=(const veque<T> &lhs, const veque<T> &rhs)
    {
        return !( lhs == rhs );
    }

    template <typename T>
    bool operator<(const veque<T> &lhs, const veque<T> &rhs)
    {
        return std::lexicographical_compare( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() );
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


#endif