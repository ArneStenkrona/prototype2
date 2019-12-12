#ifndef PRT_VECTOR_H
#define PRT_VECTOR_H

#include "src/memory/container_allocator.h"

#include <algorithm>

#include <iostream>

namespace prt
{
    template<class T>
    class vector {
    public:
        vector(): vector(ContainerAllocator::getDefaultContainerAllocator()) {}
        
        vector(size_t count)
        : vector(ContainerAllocator::getDefaultContainerAllocator()) {
            resize(count);
        }

        vector(size_t count, const T& value,
               ContainerAllocator& allocator = ContainerAllocator::getDefaultContainerAllocator())
        : vector(allocator) {
            reserve(count);
            std::fill(&_data[0], &_data[count], value);
            _size = count;
        }

        template< class InputIt >
        vector(InputIt first, InputIt last,
               ContainerAllocator& allocator = ContainerAllocator::getDefaultContainerAllocator())
        : vector(allocator) {
            /*assert(first <= last);
            size_t numOfT = last - first;
            reserve(numOfT);
            std::copy(first, last, _data);
            _size = numOfT;*/

            for (auto it = first; it != last; it++) {
                push_back(*it);
            }
        }

        vector(std::initializer_list<T> ilist, 
                ContainerAllocator& allocator = ContainerAllocator::getDefaultContainerAllocator()) 
        : vector(allocator) {
            reserve(ilist.size());
            for (auto it = ilist.begin(); it < ilist.end(); it++) {
                new (&_data[_size++]) T(*it);
            }
        }

        vector(ContainerAllocator& allocator)
        : _data(nullptr), _size(0), _capacity(0), _allocator(&allocator) {}

        vector(const vector& other)
        : vector(other, *other._allocator) {}

        vector(const vector& other, ContainerAllocator& allocator)
        : vector(allocator) {
            if (this != &other) {
                if (other._data != nullptr) {
                    reserve(other._size);
                    for (size_t i = 0; i < other._size; i++) {
                        new (&_data[i]) T(other[i]);
                    }
                    _size = other._size;
                }
            }
        }

        vector& operator=(const vector& other) {
            if (this != &other) {
                clear();
                _allocator = other._allocator;
                if (other._data != nullptr) {
                    reserve(other._size);
                    for (size_t i = 0; i < other._size; i++) {
                        new (&_data[i]) T(other[i]);
                    }
                    _size = other._size;
                } 
            } 
            return *this;
        }

        ~vector() {
            if (_data != nullptr) {
                std::destroy(&_data[0], &_data[_size]);
                _allocator->free(_data);
            }
        }

        T & operator [](size_t index) {
            assert(index < _size);
            return _data[index];
        }

        const T & operator [](size_t index) const {
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
            if (_size > 0) {
                back().~T();
                _size--;
            }
        }

        void resize(size_t size) {
            if (size > _size) {
                reserve(size);
                for (size_t i = _size; i < size; i++){
                    new (&_data[i]) T();
                }
            }
                _size = size;
        }


        void resize(size_t size, const T& value) {
            if (size > _size) {
                reserve(size);
                for (size_t i = _size; i < size; i++){
                    new (&_data[i]) T(value);
                }
            }
                _size = size;
        }

        void clear() {
            if (_data != nullptr) {
                std::destroy(&_data[0], &_data[_size]);
                _allocator->free(_data);
            }
            _data = nullptr;
            _size = 0;
            _capacity = 0;
        }

        void reserve(size_t capacity) {
            if (capacity <= _capacity) {
                return;
            }

            T* newPointer = static_cast<T*>(_allocator->allocate(capacity * sizeof(T),
                                            alignof(T)));

            if (_data != nullptr) {
                for (size_t i = 0; i < _size; i++){
                    new (&newPointer[i]) T(_data[i]);
                }

                _allocator->free(_data);
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

        inline T* begin() const { return &_data[0]; }
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
        // Allocator
        ContainerAllocator* _allocator;
    };
}

#endif