![meme](docs/main_image.png)
<div align="center">

![c++17 label](https://img.shields.io/badge/recomend-gray?style=flat&logo=C%2B%2B&label=C%2B%2B17&labelColor=blue)![static library label](https://img.shields.io/badge/static%20library-%23d77f47?style=flat)![header_only_label](https://img.shields.io/badge/header%20only-gray?style=flat&label=.h&labelColor=blue)
</div>

# BitArray
A static C++ library for using arrays with configurable [bit](https://en.wikipedia.org/wiki/Bit) size per element. The element size can be in the range `[1...63]` bits.

# Instalation
Install the [latest](https://github.com/ganda1fero/BitArray/releases/tag/1.0.0) version. Place the [BitArray.h](https://github.com/ganda1fero/BitArray/releases/download/1.0.0/BitArray.h) file in the project folder and add the:
```cpp
#include "BitArray.h"
```

# Using
Usage is similar to the implementation of [std::vector](https://en.wikipedia.org/wiki/Sequence_container_(C%2B%2B)#Vector)

Excellent memory savings, for example when using `BitArray<1>` (1 bit per element) vs. `std::vector<char>` (1 byte per element)

However, random access (`operator[]`) is **more expensive**, so for maximum speed, it's recommended to use `iterators` for traversal. **Maximum performance** is achieved by sequentially iterating through the iterator.

# Recommended Application
- Large arrays of compact values ​​(flags, small counters, state tables).
- Storing economical representations of large matrices/networks/bit fields.
- Scenarios where memory is a priority, not excessively frequent random access.