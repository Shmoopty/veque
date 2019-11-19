# veque
Efficient C++17 container combining the best features of `std::vector` and `std::deque`

> _"In Most Cases, Prefer Using deque (Controversial)"_
>
> -Herb Sutter, GotW #54

`std::vector` has maintained popular usage as a C++ standard container.  It's cache-friendly.  It maintains array layout.  Its iterator invalidation rules are somewhat complex, but developers have learned them.

`std::deque`, however, has grown increasingly unpopular.  Nearly to the extent of `std::list`, and for similar reasons - it is non-contiguous and _very_ cache-unfriendly to iterate.

**veque** is an efficient container with interface and organization very similar to a `std::vector`.  However, while a `std::vector` places all of its unused allocated storage after `end()`, **veque** distributes its unused space both _before_ and _after_ the used storage. 

### Features
* Like `std::vector`, **veque** is an ordered container, in cache-friendly, array-compatible contiguous memory.
* Like `std::deque`, **veque** allows fast insertion/deletion from the front of the container
* Because **veque** can resize from both sides, insertions and erasures from arbitrary locations will be faster, because there are often two choices for _what data to shift_.

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
