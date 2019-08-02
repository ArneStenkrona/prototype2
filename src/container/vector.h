#ifndef PRT_VECTOR_H
#define PRT_VECTOR_H

#include "src/memory/container_allocator.h"

namespace prt
{
    template<class T>
    class vector {
    public:
        vector()
        : vector(ContainerAllocator::getDefaultContainerAllocator()) {}

        vector(ContainerAllocator& allocator)
        : _allocator(allocator), _data(nullptr),
          _size(0), _capacity(0) {}

        T operator [](size_t index) const { 
            assert(index < _size);
            return _data[index];
        }

        T & operator [](size_t index) {
            assert(index < _size);
            return _data[index];
        }

        void push_back(const T& t) {
            if (_size >= _capacity) {
                size_t newCapacity = _capacity * CAPACITY_INCREASE_CONSTANT;
                newCapacity = newCapacity == 0 ? 1 : newCapacity;
                reserve(newCapacity);
            }

            _data[_size] = t;
            _size++;
        }

        void pop_back() {
            _size--;
        }

        void resize(size_t size) {
            if (size > _size) {
                reserve(size);
                for (size_t i = _size; i < size; i++) {
                    _data[i] = T();
                }
                _size = size;
            }
        }

        void reserve(size_t capacity) {
            if (capacity <= _capacity) {
                return;
            }
            
            T* newPointer = static_cast<T*>(_allocator.allocate(capacity * sizeof(T),
                                                sizeof(T)));

            if (_data != nullptr) {
                std::copy(_data, _data + _size,
                          newPointer);
            }
            
            _capacity = capacity;
            _data = newPointer;
        }

        inline T& front() const { return _data[0]; }
        inline T& back() const { return _data[_size - 1]; }

        inline size_t size() const { return _size; }
        inline size_t capacity() const { return _capacity; }
        inline T* data() const { return reinterpret_cast<void*>(_data); }

        inline const T* begin() const { return _data; }

        inline const T* end() const { return _data + _size; }

    private:
        static constexpr size_t CAPACITY_INCREASE_CONSTANT = 2;
        // start of vector.
        T* _data;
        // Number of elements currently in vector
        size_t _size;
        // Buffer size in bytes.
        size_t _capacity;

        // Allocator
        ContainerAllocator& _allocator;
    };
}

#endif