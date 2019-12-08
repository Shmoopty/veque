# veque API
_The double-ended vector_

------

This is intended to be a thorough description of the entire `veque` interface.

To jump to the API details that are unique to `veque`, go to the [Capacity](#capacity) and [Modifiers](#modifiers) sections.

    template<typename T, typename ResizeTraits=fast_resize_traits, typename Allocator=std::allocator<T>>
    class veque
    
## Iterator invalidation

| Operations | Invalidated |
|---|---|
| All read only operations, swap | Never |
| `clear`, `operator=`, `assign` | Always |
| `insert`, `emplace`, `erase`<br/>using resize traits with `resize_from_closest_side=true` |  **Always**.  Consider using the returned iterator |
| `insert`, `emplace`, `erase`<br/>using resize traits with `resize_from_closest_side=false`| If the new `size()` is greater than `capacity()`, all iterators and references are invalidated. Otherwise, only the iterators and references before the insert/erase point remain valid. |
| all `reserve`s, `shrink_to_fit` | If the vector changed capacity, all of them. If not, none |
| `push_back`, `emplace_back`, `resize`, `resize_back` | If the new `size()` is greater than `capacity_back()`, all of them. If not, only `end()` |
| `push_front`, `emplace_front`, `resize_front` | If the new `size()` is greater than `capacity_front()`, all of them. If not, only `begin()` |
| All `resize`s | If the vector changed capacity, all of them. If not, only `begin()` or `end()` |
| All `pop_back`s | The element erased and `end()` |
| All `pop_front`s | The element erased and `begin()` |

## Member types
  
        using allocator_type         = Allocator
        using value_type             = T
        using reference              = T &
        using const_reference        = const T &
        using pointer                = T *
        using const_pointer          = const T *
        using iterator               = T *
        using const_iterator         = const T *
        using reverse_iterator       = std::reverse_iterator<iterator>
        using const_reverse_iterator = std::reverse_iterator<const_iterator>
        using difference_type        = std::ptrdiff_t
        using size_type              = std::size_t
        using ssize_type             = std::ptrdiff_t

## Member functions

All constructors match the behavior, complexity, and exception rules of [C++17 `std::vector` constructors](https://en.cppreference.com/w/cpp/container/vector/vector)

Construction from a veque with different `ResizeTraits` is supported.

        veque() noexcept ( noexcept(Allocator()) )
        
        explicit veque( const Allocator& ) noexcept
        
        explicit veque( size_type n, const Allocator& = Allocator() )
        
        veque( size_type n, const T &val, const Allocator& = Allocator() )
        
        template <typename InputIt>
        veque( InputIt first,  InputIt last, const Allocator& = Allocator() )
        
        veque( std::initializer_list<T>, const Allocator& = Allocator() )
        
        veque( const veque & )
        
        veque( const veque &, const Allocator& )
        
        veque( veque && ) noexcept
        
        veque( veque &&, const Allocator& ) noexcept
        
Destructs the container. The destructors of the elements are called and the used storage is deallocated.

        ~veque()

All assignment operators match the behavior, complexity, and exception rules of [C++17 `std::vector` assignment operators](https://en.cppreference.com/w/cpp/container/vector/operator%3D)

Assignment from a veque with different `ResizeTraits` is supported.

        veque & operator=( const veque & )
        
        veque & operator=( veque && ) noexcept(
            noexcept(std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value
            || std::allocator_traits<Allocator>::is_always_equal::value) )
            
        veque & operator=( std::initializer_list<T> )
        
All `assign` functions match the behavior, complexity, and exception rules of [C++17 `std::vector::assign`](https://en.cppreference.com/w/cpp/container/vector/assign)

        void assign( size_type, const T &value )
        
        template <typename InputIt>
        void assign( InputIt, InputIt )
        
        void assign( std::initializer_list<T> )
        
        allocator_type get_allocator() const

### Element access

All element access functions match the behavior, complexity, and exception rules of [C++17 `std::vector::assign`](https://en.cppreference.com/w/cpp/container/vector#Element_access)

        reference at( size_type )
        
        const_reference at( size_type ) const
        
        reference operator[]( size_type )
        
        const_reference operator[]( size_type ) const
        
        reference front()
        
        const_reference front() const
        
        reference back()
        
        const_reference back() const
        
        T * data() noexcept
        
        const T * data() const noexcept
        
### Iterators

All iterator functions match the behavior, complexity, and exception rules of [C++17 `std::vector` iterator functions](https://en.cppreference.com/w/cpp/container/vector#Iterators)

        iterator begin() noexcept
        
        const_iterator begin() const noexcept
        
        const_iterator cbegin() const noexcept
        
        iterator end() noexcept
        
        const_iterator end() const noexcept
        
        const_iterator cend() const noexcept
        
        reverse_iterator rbegin() noexcept
        
        const_reverse_iterator rbegin() const noexcept
        
        const_reverse_iterator crbegin() const noexcept
        
        reverse_iterator rend() noexcept
        
        const_reverse_iterator rend() const noexcept
        
        const_reverse_iterator crend() const noexcept

### Capacity

Returns current size + unused allocated storage before front().  Can be used to determine if adding elements at begin() will trigger a reallocation

        size_type capacity_front() const noexcept

Returns current size + unused allocated storage after back().  Can be used to determine if adding elements at end() will trigger a reallocation.  This function behaves identically to `capacity()`

        size_type capacity_back() const noexcept

Returns all allocated storage, used and unused

        size_type capacity_full() const noexcept

Ensures sufficient storage for growth after end().  This function behaves identically to `std::vector::reserve`

        void reserve_back( size_type )
        
Ensures sufficient storage for growth before begin()

        void reserve_front( size_type )
        
Equivalent to `reserve_front(front); reserve_back(back);`, performing at most one reallocation

        void reserve( size_type front, size_type back );

Ensures sufficient storage for both front and back growth.  Equivalent to `reserve(size,size)`

        void reserve( size_type size )

Returns the size as a signed integer type

        ssize_type ssize() const noexcept

All other capacity functions match the behavior, complexity, and exception rules of C++17 `std::vector`

        [[nodiscard]] bool empty() const noexcept
        
        size_type size() const noexcept
        
        size_type max_size() const noexcept
        
        void reserve( size_type )
        
        size_type capacity() const noexcept
        
        void shrink_to_fit()

### Modifiers

Pops and returns back element.  Strong exception safety guaranteed.  Moves element when appropriate

        T pop_back_element()

Pops and returns front element.  Strong exception safety guaranteed.  Moves element when appropriate

        T pop_front_element()
        
Resizes the veque, by adding or removing from the front. 

        void resize_front( size_type )

        void resize_front( size_type, const T & )

Resizes the veque, by adding or removing from the back.  This function behaves identically to `resize()`

        void resize_back( size_type )
        
        void resize_back( size_type, const T & )
        
Adds a new element to the front of the veque

        void push_front( const T & )
        
        void push_front( T && )
        
        template <class ... Args> reference
        emplace_front( Args && ... args )

Removes element at front of the veque

        void pop_front()

All `insert`, `emplace` and `erase` functions perform the same tasks as their `std::vector` counterparts.  **However**,
* The `veque` may be resized from either end.
* This makes these operations often perform much faster
* All iterators are invalidated, though.  Consider utilizing the returned iterator.

        iterator insert( const_iterator, const T & )
        
        iterator insert( const_iterator, T && )
        
        iterator insert( const_iterator, size_type, const T& )
        
        template <class InputIt>
        iterator insert( const_iterator, InputIt, InputIt )
        
        iterator insert( const_iterator, std::initializer_list<T> )
        
        template <class ... Args> iterator emplace( const_iterator, Args && ... )
        
        iterator erase( const_iterator )
        
        iterator erase( const_iterator, const_iterator )
        
All other modifier functions match the behavior, complexity, and exception rules of C++17 `std::vector`
        
        void clear() noexcept

        void push_back( const T & )
        
        void push_back( T && )
        
        template <class ... Args>
        reference emplace_back( Args && ... args )
        
        void pop_back()

        void resize( size_type )
        
        void resize( size_type, const T & )
        
        void swap( veque & ) noexcept(
            noexcept(std::allocator_traits<Allocator>::propagate_on_container_swap::value
            || std::allocator_traits<Allocator>::is_always_equal::value))

## Non-member functions

    bool operator==( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )
    
    bool operator!=( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )
    
    bool operator< ( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )
    
    bool operator<=( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )
    
    bool operator> ( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )
    
    bool operator>=( const veque<T,Alloc> &lhs, const veque<T,Alloc> &rhs )

    void swap( veque<T,Alloc> & lhs, veque<T,Alloc> & rhs ) noexcept(
        noexcept(std::allocator_traits<Alloc>::propagate_on_container_swap::value
        || std::allocator_traits<Alloc>::is_always_equal::value))

## Non-member types

    struct std::hash<veque<T,Alloc>>

## Deduction Guides

(It does what you'd imagine)

    template< class InputIt, class Alloc = std::allocator<typename std::iterator_traits<InputIt>::value_type>>
    veque(InputIt, InputIt, Alloc = Alloc())
      -> veque<typename std::iterator_traits<InputIt>::value_type, Alloc>
