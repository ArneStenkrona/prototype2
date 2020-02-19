#ifndef PRT_ARRAY_H
#define PRT_ARRAY_H

#include <cstdint>
#include <cassert>

namespace prt {
    template <typename T, size_t N>
    struct array {
    public:
        typedef T data_type;
        enum {
            _size = N,
            _data_size = sizeof(T) * N
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

        constexpr inline size_t data_size() const { return _data_size; }
        // constexpr inline size_t data_size() const { return reinterpret_cast<size_t>(reinterpret_cast<uintptr_t>(&_data[_size]) - reinterpret_cast<uintptr_t>(&_data[0])); }
    };
}

#endif