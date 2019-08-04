#ifndef PRT_VECTOR_H
#define PRT_VECTOR_H

#include "src/memory/container_allocator.h"

namespace prt
{
    template<class T>
    class vector {
    public:
        vector(): vector(ContainerAllocator::getDefaultContainerAllocator(), 1) {}
        
        vector(size_t alignment)
        : vector(ContainerAllocator::getDefaultContainerAllocator(), alignment) {}

        vector(ContainerAllocator& allocator, size_t alignment = 1)
        : _allocator(allocator), _data(nullptr), _size(0), _capacity(0),
          _alignment(alignment) {}

        ~vector() {
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
                                                _alignment));

            if (_data != nullptr) {
                std::copy(_data, _data + _size,
                          newPointer);

                if (_data != newPointer) {
                    _allocator.free(_data);
                }
            }

            _capacity = capacity;
            _data = newPointer;
        }

        inline T& front() const { return _data[0]; }
        inline T& back() const { return _data[_size - 1]; }

        inline size_t size() const { return _size; }
        inline size_t capacity() const { return _capacity; }
        inline T* data() const { return reinterpret_cast<void*>(_data); }

        inline T* begin() const { return _data; }
        inline T* end() const { return _data + _size; }

    private:
        // Capacity increase when size exceeds capacity
        static constexpr size_t CAPACITY_INCREASE_CONSTANT = 2;
        // Start of vector.
        T* _data;
        // Number of elements currently in vector
        size_t _size;
        // Buffer size in bytes.
        size_t _capacity;
        // Alignment
        size_t _alignment;
        // Allocator
        ContainerAllocator& _allocator;
    };
}

#endif