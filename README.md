# veque
Very fast C++17 container combining the best features of `std::vector` and `std::deque`

> _"In Most Cases, Prefer Using deque (Controversial)"_
>
> -Herb Sutter, [GotW #54](http://www.gotw.ca/gotw/054.htm)

`std::vector` has maintained popular usage as a C++ standard container.  It is cache-friendly.  It maintains array layout.  Its iterator invalidation rules are somewhat complex, but developers have learned them.

`std::deque`, despite having fast insertion and removal from both ends of the container, has not been able to reach popular use.  Nearly to the extent of `std::list`, and for similar reasons - it is non-contiguous and _very_ cache-unfriendly to iterate.

**veque** is an efficient container with interface and organization very similar to a `std::vector`.  However, while a `std::vector` places all of its unused allocated storage after `end()`, **veque** distributes its unused space both _before_ and _after_ the used storage. 

### Features
* Like `std::vector`, **veque** is an ordered container, in cache-friendly, array-compatible contiguous memory.
* Like `std::deque`, **veque** allows fast insertion/deletion from the front of the container
* Because **veque** can resize from both sides, insertions and erasures from arbitrary locations will be faster, because there are often two choices for _what data to shift_.

### Usage
The interface for **veque** includes the entire interface for `std::vector`, including C++17 and C++20 interface and noexcept specifications.  allowing **veque** to be used as a drop-in replacement.

In addition, **veque** provides the following additional functions:
* `push_front()`
* `emplace_front()`
* `pop_front()`
* `resize_front()`
* `resize_back()` (Same as `resize()`, for `std::vector` parity)
* `capacity_front()`
* `capacity_back()` (Same as `capacity()`, for `std::vector` parity)
* `capacity_full()`

### Tradeoffs
Is **veque** better than `std::vector` in every conceivable way?  No.  But the tradeoffs are appealing.
* **veque** is a bit more eager to preallocate memory than a typical `std::vector` implementation, to anticipate resizing from either end.
* `insert()` and `erase()` function calls should be assumed to invalidate all iterators, since the resizing could happen from either direction.  By comparison, `std::vector` operations will sometimes only invalidate some of the iterators.
* `veque<bool>` is *not* specialized.  Whether that makes it better or worse is up to you.

### To do:
* Perhaps C++14 support?
