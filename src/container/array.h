#ifndef PRT_ARRAY_H
#define PRT_ARRAY_H

#include "src/memory/container_allocator.h"

#include <cstdint>
#include <cassert>

namespace prt {
    template <typename T>
    class array {
    public:        
        array(size_t size, size_t alignment = 1)
        : array(ContainerAllocator::getDefaultContainerAllocator(), size, alignment) {}

        array(ContainerAllocator& allocator, size_t size, size_t alignment = 1)
            : _data(nullptr), _size(size), _allocator(allocator) {
            _data = static_cast<T*>(_allocator.allocate(_size * sizeof(T),
                                                    alignment));

            for (size_t i = 0; i < size; i++) {
                _data[i] = T();
            }
        }

        ~array() {
            _allocator.free(_data);
        }

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
        inline T* end() const { return _data + _size; }
    private:
        T* _data;
        size_t _size; 
        
        ContainerAllocator& _allocator;
    };
}

#endif