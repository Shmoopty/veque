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

namespace veque
{
    struct fast_resize_traits
    {
        // Relative to size(), amount of unused space to reserve when reallocating
        using _front_realloc = std::ratio<1>;
        using _back_realloc = std::ratio<1>;

        // If true, arbitrary insert and erase operations are twice the speed of
        // std::vector, but those operations invalidate all iterators
        static constexpr auto resize_from_closest_side = true;
   };
    
    struct vector_compatible_resize_traits
    {
        // Relative to size(), amount of unused space to reserve when reallocating
        using _front_realloc = std::ratio<1>;
        using _back_realloc = std::ratio<1>;

        // If false, veque is a 100% compatible drop-in replacement for
        // std::vector including iterator invalidation rules
        static constexpr auto resize_from_closest_side = false;
   };
    
    struct std_vector_traits
    {
        // Reserve storage only at back, like std::vector
        using _front_realloc = std::ratio<0>;
        using _back_realloc = std::ratio<1>;

        // Same iterator invalidation rules as std::vector
        static constexpr auto resize_from_closest_side = false;
   };
    
    template< typename T, typename ResizeTraits = fast_resize_traits, typename Allocator = std::allocator<T> >
    class veque
    {
        template<typename I, typename = void>
        struct is_input_iterator : std::false_type {};

        template<typename I>
        struct is_input_iterator<I,typename std::enable_if_t<
            std::is_convertible_v<typename std::iterator_traits<I>::iterator_category,std::input_iterator_tag>>>
            : std::true_type {};
        
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
        veque() noexcept (noexcept(Allocator()))
            : veque ( Allocator{} )
        {
        }

        explicit veque( const Allocator& alloc ) noexcept
            : _data { 0, alloc }
        {
        }

        explicit veque( size_type n, const Allocator& alloc = Allocator() )
            : veque( allocate_uninitialized_tag{}, n, alloc )
        {
            _value_construct_range( begin(), end() );
        }

        veque( size_type n, const T &value, const Allocator& alloc = Allocator() )
            : veque( allocate_uninitialized_tag{}, n, alloc )
        {
            _value_construct_range( begin(), end(), value );
        }

        template< typename InputIt, typename = std::enable_if_t<is_input_iterator<InputIt>::value> >
        veque( InputIt first,  InputIt last, const Allocator& alloc = Allocator() )
            : veque( allocate_uninitialized_tag{}, static_cast<size_type>( std::distance( first, last ) ), alloc )
        {
            _copy_construct_range( begin(), end(), first );
        }

        veque( std::initializer_list<T> lst, const Allocator& alloc = Allocator() )
            : veque( allocate_uninitialized_tag{}, lst.size(), alloc )
        {
            _copy_construct_range( begin(), end(), lst.begin() );
        }

        veque( const veque & other )
            : _size{ other._size }
            , _offset{ 0 }
            , _data { other._size, alloc_traits::select_on_container_copy_construction( other._allocator() ) }
        {
            _copy_construct_range( begin(), end(), other.begin() );
        }
        
        template< typename OtherResizeTraits >
        veque( const veque<T,OtherResizeTraits,Allocator> & other )
            : _size{ other._size }
            , _offset{ 0 }
            , _data { other._size, alloc_traits::select_on_container_copy_construction( other._allocator() ) }
        {
            _copy_construct_range( begin(), end(), other.begin() );
        }
        
        template< typename OtherResizeTraits >
        veque( const veque<T,OtherResizeTraits,Allocator> & other, const Allocator & alloc )
            : _size{ other._size }
            , _offset{ 0 }
            , _data { other._size, alloc }
        {
            _copy_construct_range( begin(), end(), other.begin() );
        }
        
        veque( veque && other ) noexcept
            : _data {}
        {
            _swap_with_allocator( std::move(other) );
        }
        
        template< typename OtherResizeTraits >
        veque( veque<T,OtherResizeTraits,Allocator> && other ) noexcept
            : _data {}
        {
            _swap_with_allocator( std::move(other) );
        }

        template< typename OtherResizeTraits >
        veque( veque<T,OtherResizeTraits,Allocator> && other, const Allocator & alloc ) noexcept
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
        
        ~veque()
        {
            _destroy( begin(), end() );
        }
        
        veque & operator=( const veque & other )
        {
            return _copy_assignment( other );
        }

        template< typename OtherResizeTraits >
        veque & operator=( const veque<T,OtherResizeTraits,Allocator> & other )
        {
            return _copy_assignment( other );
        }

        veque & operator=( veque && other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_move_assignment::value
            || alloc_traits::is_always_equal::value) )
        {
            return _move_assignment( std::move(other) );
        }
            
        template< typename OtherResizeTraits >
        veque & operator=( veque<T,OtherResizeTraits,Allocator> && other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_move_assignment::value
            || alloc_traits::is_always_equal::value) )
        {
            return _move_assignment( std::move(other) );
        }

        veque & operator=( std::initializer_list<T> lst )
        {
            assign( lst.begin(), lst.end() );
            return *this;
        }

        void assign( size_type count, const T &value )
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

        template< typename InputIt, typename = std::enable_if_t<is_input_iterator<InputIt>::value> >
        void assign( InputIt first, InputIt last )
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
        
        void assign( std::initializer_list<T> lst )
        {
            assign( lst.begin(), lst.end() );
        }

        allocator_type get_allocator() const
        {
            return _allocator();
        }

        // Element access
        reference at( size_type idx )
        {
            if ( idx >= size() )
            {
                throw std::out_of_range("veque<T,ResizeTraits,Alloc>::at(" + std::to_string(idx) + ") out of range");
            }
            return (*this)[idx];
        }

        const_reference at( size_type idx ) const
        {
            if ( idx >= size() )
            {
                throw std::out_of_range("veque<T,ResizeTraits,Alloc>::at(" + std::to_string(idx) + ") out of range");
            }
            return (*this)[idx];
        }

        reference operator[]( size_type idx )
        {
            return *(begin() + idx);
        }
    
        const_reference operator[]( size_type idx ) const
        {
            return *(begin() + idx);
        }

        reference front()
        {
            return (*this)[0];
        }

        const_reference front() const
        {
            return (*this)[0];
        }

        reference back()
        {
            return (*this)[size() - 1];
        }

        const_reference back() const
        {
            return (*this)[size() - 1];
        }

        T * data() noexcept
        {
            return begin();
        }

        const T * data() const noexcept
        {
            return begin();
        }

        // Iterators
        const_iterator cbegin() const noexcept
        {
            return _storage_begin() + _offset;
        }

        iterator begin() noexcept
        {
            return _storage_begin() + _offset;
        }

        const_iterator begin() const noexcept
        {
            return cbegin();
        }

        const_iterator cend() const noexcept
        {
            return _storage_begin() + _offset + size();
        }

        iterator end() noexcept
        {
            return _storage_begin() + _offset + size();
        }

        const_iterator end() const noexcept
        {
            return cend();
        }

        const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(cend());
        }

        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        const_reverse_iterator rbegin() const noexcept
        {
            return crbegin();
        }

        const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(cbegin());
        }

        reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        const_reverse_iterator rend() const noexcept
        {
            return crend();
        }

        // Capacity
        [[nodiscard]] bool empty() const noexcept
        {
            return size() == 0;
        }

        size_type size() const noexcept
        {
            return _size;
        }

        ssize_type ssize() const noexcept
        {
            return _size;
        }

        size_type max_size() const noexcept
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

        void reserve( size_type count )
        {
            reserve( count, count );
        }

        // Reserve front and back capacity, independently.
        void reserve( size_type front, size_type back )
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

        void reserve_front( size_type count )
        {
            if ( count > capacity_front() )
            {
                auto new_full_capacity = capacity_back() - size() + count;
                if ( new_full_capacity > max_size() )
                {
                    throw std::length_error("veque<T,ResizeTraits,Alloc>::reserve_front(" + std::to_string(count) + ") exceeds max_size()");
                }
                _reallocate( new_full_capacity, count );
            }
        }

        void reserve_back( size_type count )
        {
            if ( count > capacity_back() )
            {
                auto new_full_capacity = capacity_front() - size() + count;
                if ( new_full_capacity > max_size() )
                {
                    throw std::length_error("veque<T,ResizeTraits,Alloc>::reserve_back(" + std::to_string(count) + ") exceeds max_size()");
                }
                _reallocate( new_full_capacity, _offset );
            }
        }

        // Returns current size + unused allocated storage before front()
        size_type capacity_front() const noexcept
        {
            return _offset + size();
        }

        // Returns current size + unused allocated storage after back()
        size_type capacity_back() const noexcept
        {
            return capacity_full() - _offset;
        }

        // Returns current size + all unused allocated storage
        size_type capacity_full() const noexcept
        {
            return _data._allocated;
        }

        // To achieve interface parity with std::vector, capacity() returns capacity_back();
        size_type capacity() const noexcept
        {
            return capacity_back();
        }

        void shrink_to_fit()
        {
            if ( size() < capacity_full() )
            {
                _reallocate( size(), 0 );
            }
        }
        
        // Modifiers
        void clear() noexcept
        {
            using unused_front_ratio = std::ratio_divide<_front_realloc,_unused_realloc>;
            
            _destroy( begin(), end() );
            _size = 0;
            _offset = capacity_full() * unused_front_ratio::num / unused_front_ratio::den;
        }

        iterator insert( const_iterator it, const T & value )
        {
            return emplace( it, value );
        }

        iterator insert( const_iterator it, T && value )
        {
            return emplace( it, std::move(value) );
        }

        iterator insert( const_iterator it, size_type count, const T & value )
        {
            auto res = _insert_storage( it, count );
            _value_construct_range( res, res + count, value );
            return res;
        }

        template< typename InputIt, typename = std::enable_if_t<is_input_iterator<InputIt>::value> >
        iterator insert( const_iterator it, InputIt first, InputIt last )
        {
            auto count = std::distance( first, last );
            auto res = _insert_storage( it, count );
            _copy_construct_range( res, res + count, first );
            return res;
        }

        iterator insert( const_iterator it, std::initializer_list<T> lst )
        {
            return insert( it, lst.begin(), lst.end() );
        }
        
        template< typename ...Args >
        iterator emplace( const_iterator it, Args && ... args )
        {
            auto res = _insert_storage( it, 1 );
            alloc_traits::construct( _allocator(), res, std::forward<Args>(args)... );
            return res;
        }

        iterator erase( const_iterator it )
        {
            return erase( it, it + 1 );
        }

        iterator erase( const_iterator first, const_iterator last )
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

        void push_back( const T & value )
        {
            emplace_back( value );
        }

        void push_back( T && value )
        {
            emplace_back( std::move(value) );
        }
        
        template< typename ... Args>
        reference emplace_back( Args && ...args )
        {
            if ( size() == capacity_back() )
            {
                _reallocate_space_at_back( size() + 1 );
            }
            alloc_traits::construct( _allocator(), end(), std::forward<Args>(args)... );
            _move_end( 1 );
            return back();
        }

        void push_front( const T & value )
        {
            emplace_front( value );
        }

        void push_front( T && value )
        {
            emplace_front( std::move(value) );
        }

        template< typename ... Args>
        reference emplace_front( Args && ...args )
        {
            if ( size() == capacity_front() )
            {
                _reallocate_space_at_front( size() + 1 );
            }
            alloc_traits::construct( _allocator(), begin()-1, std::forward<Args>(args)... );
            _move_begin( -1 );
            return front();
        }

        void pop_back()
        {
            alloc_traits::destroy( _allocator(), &back() );
            _move_end( -1 );
        }
        
        // Move-savvy pop back with strong exception guarantee
        T pop_back_element()
        {
            auto res( _nothrow_construct_move(back()) );
            pop_back();
            return res;
        }

        void pop_front()
        {
            alloc_traits::destroy( _allocator(), &front() );
            _move_begin( 1 );
        }

        // Move-savvy pop front with strong exception guarantee
        T pop_front_element()
        {
            auto res( _nothrow_construct_move(front()) );
            pop_front();
            return res;
        }

        // Resizes the veque, by adding or removing from the front. 
        void resize_front( size_type count )
        {
            _resize_front( count );
        }
    
        void resize_front( size_type count, const T & value )
        {
            _resize_front( count, value );
        }

        // Resizes the veque, by adding or removing from the back.
        void resize_back( size_type count )
        {
            _resize_back( count );
        }

        void resize_back( size_type count, const T & value )
        {
            _resize_back( count, value );
        }
        
        // To achieve interface parity with std::vector, resize() performs resize_back();
        void resize( size_type count )
        {
            _resize_back( count );
        }

        void resize( size_type count, const T & value )
        {
            _resize_back( count, value );
        }

        template< typename OtherResizeTraits >
        void swap( veque<T,OtherResizeTraits,Allocator> & other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_swap::value
            || alloc_traits::is_always_equal::value))
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
    
    private:
        
        using _front_realloc = typename ResizeTraits::_front_realloc::type;
        using _back_realloc = typename ResizeTraits::_back_realloc::type;
        using _unused_realloc = std::ratio_add< _front_realloc, _back_realloc >;
        using _full_realloc = std::ratio_add< std::ratio<1>, _unused_realloc >;

        static constexpr auto resize_from_closest_side = ResizeTraits::resize_from_closest_side;

        static_assert( std::ratio_greater_equal_v<_front_realloc,std::ratio<0>> );
        static_assert( std::ratio_greater_equal_v<_back_realloc,std::ratio<0>> );
        static_assert( std::ratio_greater_v<_unused_realloc,std::ratio<0>> );
       
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

            Data() {}
            Data( size_type capacity, const Allocator & alloc )
                : Allocator{alloc}
                , _storage{ std::allocator_traits<Allocator>::allocate( allocator(), capacity ) }
                , _allocated{capacity}
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
        veque( allocate_uninitialized_tag, size_type size, const Allocator & alloc )
            : _size{ size }
            , _offset{ size * _front_realloc::num / _front_realloc::den }
            , _data { size * _full_realloc::num / _full_realloc::den, alloc }
        {
        }

        // Create an uninitialized empty veque, with specified storage params
        veque( allocate_uninitialized_tag, size_type size, size_type allocated, size_type offset, const Allocator & alloc )
            : _size{ size }
            , _offset{ offset }
            , _data { allocated, alloc }
        {
        }
        
        // Acquire Allocator
        Allocator& _allocator() noexcept
        {
            return _data.allocator();
        }

        const Allocator& _allocator() const noexcept
        {
            return _data.allocator();
        }

        // Destroy elements in range
        void _destroy( const_iterator b, const_iterator e )
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
        veque & _copy_assignment( const veque<T,OtherResizeTraits,Allocator> & other )
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
        veque & _move_assignment( veque<T,OtherResizeTraits,Allocator> && other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_move_assignment::value
            || alloc_traits::is_always_equal::value) )
        {
            if constexpr ( !alloc_traits::is_always_equal::value )
            {
                if ( _allocator() != other._allocator() )
                {
                    if constexpr ( alloc_traits::propagate_on_container_move_assignment::value )
                    {
                        _swap_with_allocator( veque( std::move(other), other._allocator() ) );
                    }
                    else
                    {
                        if ( other.size() > capacity_full() )
                        {
                            _swap_with_allocator( veque( std::move(other), _allocator() ) );
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
        void _value_construct_range( const_iterator b, const_iterator e, const Args & ...args )
        {
            if constexpr ( std::is_trivially_copy_constructible_v<T> && sizeof...(args) == 0 && _calls_default_constructor_directly )
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

        template< typename InputIt, typename = std::enable_if_t<is_input_iterator<InputIt>::value> >
        void _copy_construct_range( const_iterator b, const_iterator e, InputIt src )
        {
            if constexpr ( std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
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
        
        template< typename OtherResizeTraits >
        void _swap_with_allocator( veque<T,OtherResizeTraits,Allocator> && other ) noexcept
        {
            // Swap everything
            std::swap( _size,      other._size );
            std::swap( _offset,    other._offset );
            std::swap( _data,      other._data );
        }

        template< typename OtherResizeTraits >
        void _swap_without_allocator( veque<T,OtherResizeTraits,Allocator> && other ) noexcept
        {
            // Don't swap _data.allocator().
            std::swap( _size,            other._size );
            std::swap( _offset,          other._offset );
            std::swap( _data._allocated, other._data._allocated);
            std::swap( _data._storage,   other._data._storage);
        }
        
        template< typename ...Args >
        void _resize_front( size_type count, const Args & ...args )
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
        void _resize_back( size_type count, const Args & ...args )
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
        void _reallocate_space_at_back( size_type count )
        {
            auto allocated = count * _full_realloc::num / _full_realloc::den;
            auto offset = count * _front_realloc::num / _front_realloc::den;
            _reallocate( allocated, offset );
        }

        // ...and yet-unused space at front of this storage
        void _reallocate_space_at_front( size_type count )
        {
            auto allocated = count * _full_realloc::num / _full_realloc::den;
            auto offset = count - size() + count * _front_realloc::num / _front_realloc::den;
            _reallocate( allocated, offset );
        }

        // Move vector to new storage, with specified capacity
        void _reallocate( size_type allocated, size_type offset )
        {
            auto replacement = veque( allocate_uninitialized_tag{}, size(), allocated, offset, _allocator() );
            _nothrow_move_construct_range( replacement.begin(), replacement.end(), begin() );
            _swap_with_allocator( std::move(replacement) );
        }

        // Insert empty space, choosing the most efficient way to shift existing elements
        iterator _insert_storage( const_iterator it, size_type count )
        {
            auto required_size = size() + count;
            auto can_shift_back = capacity_back() >= required_size && it != begin();
            if constexpr ( resize_from_closest_side )
            {
                auto can_shift_front = capacity_front() >= required_size && it != end();

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
                    return _shift_front( begin(), it, count );
                }
            }
            if ( can_shift_back )
            {
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

        // Moves a valid subrange in the front direction.
        // Vector will grow, if range moves past begin().
        // Vector will shrink if range includes end().
        // Returns iterator to beginning of destructed gap
        iterator _shift_front( const_iterator b, const_iterator e, size_type count )
        {
            if ( e == begin() )
            {
                _move_begin( -count );
                return begin();
            }
            auto element_count = std::distance( b, e );
            auto start = _mutable_iterator(b);
            if ( element_count < 0 )
            {
                throw std::runtime_error("X");
            }
            else if ( element_count > 0 )
            {
                auto dest = start - count;
                if constexpr ( std::is_trivially_copyable_v<T> && std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
                {
                    std::memmove( dest, start, element_count * sizeof(T) );
                }
                else
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
            auto new_elements_at_front = static_cast<difference_type>(cbegin() - b + count);
            auto range_includes_end = (e == end());

            // If range includes end(), veque has shrunk
            if ( range_includes_end )
            {
                _move_end( -count );
            }
            // Otherwise, if range moves before begin(), veque has grown
            else if ( new_elements_at_front > 0 )
            {
                _move_begin( -new_elements_at_front );
            }

            return _mutable_iterator(e) - count;
        }

        // Moves a range towards the back.  Vector will grow, if needed.  Vacated elements are destructed.
        // Moves a valid subrange in the back direction.
        // Vector will grow, if range moves past end().
        // Vector will shrink if range includes begin().
        // Returns iterator to beginning of destructed gap
        iterator _shift_back( const_iterator b, const_iterator e, size_type count )
        {
            auto start = _mutable_iterator(b); 
            if ( b == end() )
            {
                _move_end( count );
                return start;
            }
            auto element_count = std::distance( b, e );
            if ( element_count < 0 )
            {
                throw std::runtime_error("X");
            }
            else if ( element_count > 0 )
            {
                if constexpr ( std::is_trivially_copyable_v<T> && std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
                {
                    std::memmove( start + count, start, element_count * sizeof(T) );
                }
                else
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
            difference_type new_elements_at_back = e - end() + count;
            auto range_includes_begin = (b == begin());

            // If range moves after end(), veque has grown
            if ( new_elements_at_back > 0 )
            {
                _move_end( new_elements_at_back );
            }
            // Otherwise, if range includes begin(), veque has shrunk
            else if ( range_includes_begin )
            {
                _move_begin( count );
            }

            return start;
        }

        // Assigns a fitting range of new elements to currently held storage.
        // Favors copying over constructing firstly, and positioning the new elements
        // at the center of storage secondly
        template< typename InputIt, typename = std::enable_if_t<is_input_iterator<InputIt>::value> >
        void _reassign_existing_storage( InputIt first, InputIt last )
        {
            auto count = std::distance( first, last );
            auto size_delta = static_cast<difference_type>( count - size() );
            // The "ideal" begin would put the new data in the center of storage
            auto ideal_begin = _storage_begin() + (capacity_full() - count) / 2;

            if ( size() == 0 )
            {
                // Existing veque is empty.  Construct at the ideal location
                _copy_construct_range( ideal_begin, ideal_begin + count, first );        
            }
            else if ( size_delta == 0 )
            {
                // Existing veque is the same size.  Avoid any construction by copy-assigning everything
                std::copy( first, last, begin() );
                return;
            }
            else if ( size_delta < 0 )
            {
                // New size is smaller.  Copy-assign everything, placing results as close to center as possible
                ideal_begin = std::clamp( ideal_begin, begin(), end() - count );

                _destroy( begin(), ideal_begin );
                auto ideal_end = std::copy( first, last, ideal_begin );
                _destroy( ideal_end, end() );
            }
            else
            {
                // New size is larger.  Copy-assign all existing elements, placing newly
                // constructed elements so final store is as close to center as possible
                ideal_begin = std::clamp( ideal_begin, end() - count, begin() );

                auto src = first;
                _copy_construct_range( ideal_begin, begin(), src );
                src += std::distance( ideal_begin, begin() );
                std::copy( src, src + ssize(), begin() );
                src += ssize();
                _copy_construct_range( end(), end() + std::distance(src,last), src );
            }
            _move_begin( std::distance( begin(), ideal_begin ) );
            _move_end( std::distance( end(), ideal_begin + count ) );
        }

        void _reassign_existing_storage( size_type count, const T & value )
        {
            using ideal_begin_ratio = std::ratio_divide<_front_realloc, _unused_realloc >;

            auto size_delta = static_cast<difference_type>( count - size() );
            // The "ideal" begin would put the new data in the center of storage
            auto ideal_begin = _storage_begin() + (capacity_full() - count) * ideal_begin_ratio::num / ideal_begin_ratio::den;

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
                _value_construct_range( end(), begin() + count, value );
            }
            _move_begin( std::distance( begin(), ideal_begin ) );
            _move_end( std::distance( end(), ideal_begin + count ) );
        }
        
        // Casts to T&& or T&, depending on whether move construction is noexcept
        static decltype(auto) _nothrow_construct_move( T & t )
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
        void _nothrow_move_construct( iterator dest, iterator src )
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

        void _nothrow_move_construct_range( iterator b, iterator e, iterator src )
        {
            auto size = std::distance( b, e );
            if ( size )
            {
                if constexpr ( std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
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
        
        // Move-assigns if noexcept, copies otherwise
        static void _nothrow_move_assign( iterator dest, iterator src )
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

        static void _nothrow_move_assign_range( iterator b, iterator e, iterator src )
        {
            for ( auto dest = b; dest != e; ++dest, ++src )
            {
                _nothrow_move_assign( dest, src );
            }
        }
        
        // Adjust begin(), end() iterators
        void _move_begin( difference_type count ) noexcept
        {
            _size -= count;
            _offset += count;
        }

        void _move_end( difference_type count ) noexcept
        {
            _size += count;
        }

        // Convert a local const_iterator to iterator
        iterator _mutable_iterator( const_iterator i )
        {
            return begin() + std::distance( cbegin(), i );
        }

        // Retrieves beginning of storage, which may be before begin()
        const_iterator _storage_begin() const noexcept
        {
            return _data._storage;
        }

        iterator _storage_begin() noexcept
        {
            return _data._storage;
        }
    };

    template< typename T, typename ResizeTraits, typename Alloc >
    inline bool operator==( const veque<T,ResizeTraits,Alloc> &lhs, const veque<T,ResizeTraits,Alloc> &rhs )
    {
        return ( lhs.size() == rhs.size() ) && std::equal( lhs.cbegin(), lhs.cend(), rhs.cbegin() );
    }

    template< typename T, typename ResizeTraits, typename Alloc >
    inline bool operator!=( const veque<T,ResizeTraits,Alloc> &lhs, const veque<T,ResizeTraits,Alloc> &rhs )
    {
        return !( lhs == rhs );
    }

    template< typename T, typename ResizeTraits, typename Alloc >
    inline bool operator<( const veque<T,ResizeTraits,Alloc> &lhs, const veque<T,ResizeTraits,Alloc> &rhs )
    {
        return std::lexicographical_compare( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() );
    }

    template< typename T, typename ResizeTraits, typename Alloc >
    inline bool operator<=( const veque<T,ResizeTraits,Alloc> &lhs, const veque<T,ResizeTraits,Alloc> &rhs )
    {
        return !( rhs < lhs );
    }

    template< typename T, typename ResizeTraits, typename Alloc >
    inline bool operator>( const veque<T,ResizeTraits,Alloc> &lhs, const veque<T,ResizeTraits,Alloc> &rhs )
    {
        return ( rhs < lhs );
    }

    template< typename T, typename ResizeTraits, typename Alloc >
    inline bool operator>=( const veque<T,ResizeTraits,Alloc> &lhs, const veque<T,ResizeTraits,Alloc> &rhs )
    {
        return !( lhs < rhs );
    }

    template< typename T, typename ResizeTraits, typename Alloc >
    inline void swap( veque<T,ResizeTraits,Alloc> & lhs, veque<T,ResizeTraits,Alloc> & rhs ) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    // Template deduction guide for iterator pair
    template< typename InputIt,
              typename Alloc = std::allocator<typename std::iterator_traits<InputIt>::value_type>>
    veque(InputIt, InputIt, Alloc = Alloc())
      -> veque<typename std::iterator_traits<InputIt>::value_type, Alloc>;

}

namespace std
{
    template< typename T, typename ResizeTraits, typename Alloc >
    struct hash<veque::veque<T,ResizeTraits,Alloc>>
    {
        size_t operator()( const veque::veque<T,ResizeTraits,Alloc> & v ) const
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
