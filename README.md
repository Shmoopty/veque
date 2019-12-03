# veque
_The double-ended vector_

------

A [very fast](performance/main.cpp#L7) C++17 container combining the best features of `std::vector` and `std::deque`

> _"In Most Cases, Prefer Using deque (Controversial)"_
>
> -Herb Sutter, [GotW #54](http://www.gotw.ca/gotw/054.htm)

**veque** is an allocator-aware, efficient container with interface matching both `std::vector` and `std::deque`.  Its data layout is very similar to `std::vector`, but with unused storage maintained both _before_ and _after_ the used storage. 

### Features
* Like `std::vector`, **veque** is an ordered container in cache-friendly, array-compatible contiguous memory.
* Like `std::deque`, **veque** allows fast insertion/deletion from the front of the container
* Because **veque** can resize from both sides, insertions and erasures from arbitrary locations will be **twice as fast**, because there are often two choices for _what data to shift_.

### Usage
The complete API documentation may be viewed [here](API.md).

The interface for **veque** maintains the entire interface for `std::vector`, allowing **veque** to be considered as a drop-in replacement.  (See [tradeoffs](#tradeoffs))

#### In addition, **veque** provides the following additional functions:

_`std::deque` interface:_
* `push_front()`
* `emplace_front()`
* `pop_front()`

_End-specific resizing:_
* `resize_front()`
* `resize_back()` (Same as `resize()`, to match `std::vector` and `std::deque` behavior)
* `capacity_front()`
* `capacity_back()` (Same as `capacity()`, to match `std::vector` and `std::deque` behavior)
* `capacity_full()`
* `reserve(size_type, size_type)`

_Strong exception guarantee pop-and-return, courtesy C++17:_
* `pop_back_element()`
* `pop_front_element()`

_In the spirit of C++20, it is reasonable to ask for the size as a signed type:_
* `ssize()`

### Tradeoffs
Is **veque** better than `std::vector` in every conceivable way?  No.  But the tradeoffs are appealing.
* **veque** is a bit more eager to preallocate memory than a typical `std::vector` implementation, to anticipate resizing from either end.  **(New - configurable via traits class!)**
* `insert()` and `erase()` function calls should be assumed to invalidate all iterators and references, since the resizing could happen from either direction.  By comparison, the same `std::vector` and `std::deque` operations will sometimes only invalidate *some* of the iterators and references.  **(New - configurable via traits class!)**
* `veque<bool>` is *not* specialized.  Whether that makes it better or worse is up to you.

### Why "veque"?

As a developer, I am not good at naming things.

`double_ended_vector`?

`dextor`?

### To do:
* Promote configuration constants in the class to template parameters?
* Perhaps C++14 support?
