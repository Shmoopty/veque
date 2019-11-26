# veque
_The double-ended vector_

------

[Very fast](https://github.com/Shmoopty/veque/blob/master/performance/main.cpp) C++17 container combining the best features of `std::vector` and `std::deque`

> _"In Most Cases, Prefer Using deque (Controversial)"_
>
> -Herb Sutter, [GotW #54](http://www.gotw.ca/gotw/054.htm)

**veque** is an allocator-aware, efficient container with interface matching both `std::vector` and `std::deque`.  Its organization is very similar to a `std::vector`.  However, while a `std::vector` places all of its unused allocated storage after `end()`, **veque** maintains unused space both _before_ and _after_ the used storage. 

### Features
* Like `std::vector`, **veque** is an ordered container, in cache-friendly, array-compatible contiguous memory.
* Like `std::deque`, **veque** allows fast insertion/deletion from the front of the container
* Because **veque** can resize from both sides, insertions and erasures from arbitrary locations will be faster, because there are often two choices for _what data to shift_.

### Usage
The interface for **veque** maintains the entire interface for `std::vector`, allowing **veque** to be considered as a drop-in replacement.  

The interface for **veque** maintains the entire interface for both `std::vector` and `std::deque`, allowing **veque** to be considered as a drop-in replacement.  

In addition, **veque** provides the following additional functions:
* `push_front()`
* `emplace_front()`
* `pop_front()`
* `resize_front()`
* `resize_back()` (Same as `resize()`, to match `std::vector` and `std::deque` behavior)
* `capacity_front()`
* `capacity_back()` (Same as `capacity()`, to match `std::vector` and `std::deque` behavior)
* `capacity_full()`
* `pop_back_instance()` (Move-optimized pop-with-return, with strong excpetion guarantee)
* `pop_front_instance()` (Move-optimized pop-with-return, with strong excpetion guarantee)

### Tradeoffs
Is **veque** better than `std::vector` in every conceivable way?  No.  But the tradeoffs are appealing.
* **veque** is a bit more eager to preallocate memory than a typical `std::vector` implementation, to anticipate resizing from either end.
* `insert()` and `erase()` function calls should be assumed to invalidate all iterators and references, since the resizing could happen from either direction.  By comparison, the same `std::vector` and `std::deque` operations will sometimes only invalidate *some* of the iterators and references.
* `veque<bool>` is *not* specialized.  Whether that makes it better or worse is up to you.

### To do:
* Perhaps C++14 support?
