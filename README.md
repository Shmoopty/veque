# veque
Efficient C++17 container combining the best features of `std::vector` and `std::deque`

> _"In Most Cases, Prefer Using deque (Controversial)"_
>
> -Herb Sutter, GotW #54

As C++ has matured, `std::vector` has held its ground as a very popular container type.

`std::deque`, however, has grown increasingly unpopular.  Nearly to the extent of `std::list`, and for similar reasons - it is non-contiguous and _very_ cache-unfriendly to iterate.

**veque** is an efficient container with interface and organization very similar to a `std::vector`.  However, while a `std::vector` places all of its unused allocated storage after `end()`, **veque** distributes it unused space both _before_ and _after_ the used storage. 

### Features
* Like `std::vector`, **veque** is an ordered container, in cache-friendly, array-compatible contiguous memory.
* Like `std::deque`, **veque** allows fast insertion/deletion from the front of the container
* Because **veque** can resize from both sides, insertions and erasures from arbitrary locations will be faster.

### Usage
The interface for **veque** includes the entire interface for `std::vector`, with the same iterator invalidation rules and expectations.  allowing **veque** to be used as a drop-in replacement.

In addition, **veque** provides the following additional functions:
* `capacity_front()`
* `capacity_back()` (synonymous with `capacity()`)
* `capacity_full()`
* `push_front()`
* `emplace_front()`
* `pop_front()`
* `resize_front()`
* `resize_back()` (synonymous with `resize()`)

### To do:
* GCC memory allocations are already _very aligned_, but code should explicitly demand correctly aligned memory.
* Implement `std::hash<veque>`
* Allocator support
