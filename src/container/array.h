#ifndef PRT_ARRAY_H
#define PRT_ARRAY_H

#include <cstdint>
#include <cassert>

namespace prt {
    template <typename T, size_t N>
    struct array {
    public:
        enum {
            _size = N
        };
        
        T _data[N];    

        T operator [](size_t index) const { 
            assert(index < _size);
            return _data[index];
        }

        T & operator [](size_t index) {
            assert(index < _size);
            return _data[index];
        }

        constexpr inline size_t size() const { return _size; }
        constexpr inline T* data() { return &_data[0]; }

        constexpr inline T* begin() { return &_data[0]; }
        constexpr inline T* end() { return &_data[_size]; }
    };
}

#endif