#ifndef PRT_VECTOR_H
#define PRT_VECTOR_H

#include "src/memory/container_allocator.h"

#include <algorithm>

namespace prt
{
    template<class T>
    class vector {
    public:
        vector(): vector(ContainerAllocator::getDefaultContainerAllocator(), 1) {}
        
        vector(size_t count, size_t alignment = 1)
        : vector(ContainerAllocator::getDefaultContainerAllocator(), alignment) {
            resize(count);
        }

        vector(size_t count, 
               const T& value,
               ContainerAllocator& allocator = ContainerAllocator::getDefaultContainerAllocator(),
               size_t alignment = 1)
        : vector(allocator, alignment) {
            reserve(count);
            for (size_t i = 0; i < count; i++) {
                    new (&_data[i]) T(value);
            }
            _size = count;
        }

        vector(T* first, T* last,
               ContainerAllocator& allocator = ContainerAllocator::getDefaultContainerAllocator(),
               size_t alignment = 1)
        : vector(allocator, alignment) {
            assert(first <= last);
            size_t numOfT = last - first;
            reserve(numOfT);
            std::copy(first, last, _data);
            _size = numOfT;
        }

        vector(std::initializer_list<T> ilist, 
                ContainerAllocator& allocator = ContainerAllocator::getDefaultContainerAllocator(),
                size_t alignment = 1) 
        : vector(allocator, alignment) {
            reserve(ilist.size());
            std::copy(ilist.begin(), ilist.end(), _data);
            _size = ilist.size();
        }

        vector(ContainerAllocator& allocator, size_t alignment = 1)
        : _data(nullptr), _size(0), _capacity(0),
          _alignment(alignment), _allocator(allocator) {}

        ~vector() {
            if (_data != nullptr) {
                _allocator.free(_data);
            }
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

            new (&_data[_size]) T(t);
            _size++;
        }

        void pop_back() {
            _size--;
        }

        void resize(size_t size) {
            if (size > _size) {
                reserve(size);
                for (size_t i = _size; i < _capacity; i++) {
                    new (&_data[i]) T();
                }
                _size = size;
            }
        }

        void clear() {
            _allocator.free(_data);
            _data = nullptr;
            _size = 0;
            _capacity = 0;
        }

        void reserve(size_t capacity) {
            if (capacity <= _capacity) {
                return;
            }

            T* newPointer = static_cast<T*>(_allocator.allocate(capacity * sizeof(T),
                                                _alignment));

            if (_data != nullptr) {
                std::copy(begin(), end(),
                          newPointer);

                _allocator.free(_data);
            }

            _capacity = capacity;
            _data = newPointer;
        }

        inline bool empty() const { return _size == 0; }

        inline T& front() const { return _data[0]; }
        inline T& back() const { return _data[_size - 1]; }

        inline size_t size() const { return _size; }
        inline size_t capacity() const { return _capacity; }
        inline T* data() const { return _data; }

        inline T* begin() const { return _data; }
        inline T* end() const { return &_data[_size]; }

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