#ifndef PRT_ARRAY_H
#define PRT_ARRAY_H

//#include "src/memory/container_allocator.h"

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

        inline size_t size() const { return _size; }
        inline T* data() const { return _data; }

        inline T* begin() const { return _data; }
        inline T* end() const { return &_data[_size]; }
    };
}

#endif