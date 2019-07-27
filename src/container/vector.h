#ifndef PRT_VECTOR_H
#define PRT_VECTOR_H

#include "src/memory/container_allocator.h"

namespace prt
{
    template<class T>
    class vector {
    public:
        vector()
        : vector(ContainerAllocator::getDefaultContainerAllocator) {}

        vector(ContainerAllocator& allocator)
        : _allocator(allocator), _vectorPointer(nullptr),
          _numberOfElements(0), _capacity(0) {}

        T operator [](size_t index) const { 
            assert(index < _size);
            return _vectorPointer[index];
        }

        T & operator [](size_t index) {
            assert(index < _size);
            return _vectorPointer[index];
        }

        void push_back(const T& t) {
            if (_numberOfElements = _capacity) {
                _capacity *= CAPACITY_INCREASE_CONSTANT;
                reserve(_capacity);
            }

            _vectorPointer[_numberOfElements] = t;
            _numberOfElements++;
        }

        void pop_back() {
            _numberOfElements--;
        }

        void reserve(size_t capacity) {
            if (capacity <= _capacity) {
                return;
            }
            
            T* newPointer = _allocator.allocate(capacity * sizeof(T),
                                                sizeof(T));

            if (_vectorPointer != nullptr) {
                std::copy(_vectorPointer, _vectorPointer + _numberOfElements,
                          newPointer);
            }
            
            _capacity = capacity;
            _vectorPointer = newPointer;
        }

        inline T& front() const { return vectorStart[0]; }
        inline T& back() const { return vectorStart[numberOfElements - 1]; }

        inline size_t size() const { return _numberOfElement; }
        inline void* data() const { return reinterpret_cast<void*>(_vectorPointer); }

    private:
        constexpr size_t CAPACITY_INCREASE_CONSTANT = 2;
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