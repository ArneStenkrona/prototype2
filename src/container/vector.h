#ifndef PRT_VECTOR_H
#define PRT_VECTOR_H

#include "src/memory/container_allocator.h"

namespace prt
{
    template<class T>
    class vector {
    public:
        vector()
        : _allocator(ContainerAllocator::getDefaultContainerAllocator) {}
        vector(ContainerAllocator& allocator)
        : _allocator(allocator) {}

        T operator [](size_t index) const { 
            assert(index < _size);
            return _vectorPointer[index];
        }

        T & operator [](size_t index) {
            assert(index < _size);
            return _vectorPointer[index];
        }

        void push_back(T t);
        void push_front();

        inline T& front() const { return vectorStart[0]; }
        inline T& back() const { return vectorStart[numberOfElements - 1]; }

        inline size_t size() const { return _numberOfElement; }
        inline void* data() const { return reinterpret_cast<void*>(_vectorPointer); }

    private:
        // start of vector.
        T* _vectorPointer;
        // Number of elements currently in vector
        size_t _numberOfElements;
        // Buffer size in bytes.
        size_t _capacity;

        // Allocator
        ContainerAllocator& _allocator;
    };
}

#endif