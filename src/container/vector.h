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
        explicit vector(ContainerAllocator& allocator)
        : m_data(nullptr), m_alignment(alignof(T)), m_size(0), m_capacity(0), m_allocator(&allocator) {}

        vector(): vector(ContainerAllocator::getDefaultContainerAllocator()) {}
        
        explicit vector(ALIGNMENT alignment)
        : vector() {
            m_alignment = size_t(alignment);
        }
        
        explicit vector(size_t count)
        : vector(ContainerAllocator::getDefaultContainerAllocator()) {
            resize(count);
        }

        vector(size_t count, T const & value,
               ContainerAllocator& allocator = ContainerAllocator::getDefaultContainerAllocator())
        : vector(allocator) {
            reserve(count);
            std::fill(&m_data[0], &m_data[count], value);
            m_size = count;
        }

        template< class InputIt >
        vector(InputIt const & first, InputIt const & last,
               ContainerAllocator& allocator = ContainerAllocator::getDefaultContainerAllocator())
        : vector(allocator) {
            assert(first <= last);
            size_t numOfT = last - first;
            reserve(numOfT);
            std::copy(first, last, m_data);
            m_size = numOfT;
        }

        vector(std::initializer_list<T> ilist, 
                ContainerAllocator& allocator = ContainerAllocator::getDefaultContainerAllocator()) 
        : vector(allocator) {
            reserve(ilist.size());
            size_t sz = 0;
            for (auto it = ilist.begin(); it < ilist.end(); it++) {
                new (&m_data[sz++]) T(*it);
            }
            m_size = sz;
        }

        vector(vector const & other)
        : vector(other, *other.m_allocator) {}

        vector(vector const & other, ContainerAllocator& allocator)
        : vector(allocator) {
            if (this != &other) {
                m_alignment = other.m_alignment;
                if (other.m_data != nullptr) {
                    reserve(other.m_size);
                    for (size_t i = 0; i < other.m_size; i++) {
                        new (&m_data[i]) T(other[i]);
                    }
                }
                m_size = other.m_size;
            }
        }

        vector& operator=(vector const & other) {
            if (this != &other) {
                m_alignment = other.m_alignment;
                clear();
                m_allocator = other.m_allocator;
                if (other.m_data != nullptr) {
                    reserve(other.m_size);
                    for (size_t i = 0; i < other.m_size; i++) {
                        new (&m_data[i]) T(other[i]);
                    }
                    m_size = other.m_size;
                } 
            } 
            return *this;
        }

        ~vector() {
            if (m_data != nullptr) {
                std::destroy(&m_data[0], &m_data[m_size]);
                m_allocator->free(m_data);
            }
        }

        T & operator [](size_t index) {
            assert(index < m_size);
            return m_data[index];
        }

        const T & operator [](size_t index) const {
            assert(index < m_size);
            return m_data[index];
        }

        void push_back(T const & t) {
            if (m_size >= m_capacity) {
                size_t newCapacity = m_capacity * CAPACITY_INCREASE_CONSTANT;
                newCapacity = newCapacity == 0 ? 1 : newCapacity;
                reserve(newCapacity);
            }

            new (&m_data[m_size]) T(t);
            m_size++;
        }

        template <typename... Args>
        void emplace_back(Args&&... args) {
            if (m_size >= m_capacity) {
                size_t newCapacity = m_capacity * CAPACITY_INCREASE_CONSTANT;
                newCapacity = newCapacity == 0 ? 1 : newCapacity;
                reserve(newCapacity);
            }

            new (&m_data[m_size]) T(std::forward<Args>(args)...);
            m_size++;
        }

        void pop_back() {
            if (m_size > 0) {
                back().~T();
                m_size--;
            }
        }

        void remove(size_t index) {
            assert(index < m_size);
            while (index + 1 < m_size) {
                m_data[index] =  m_data[index+1];
                ++index;
            }
            --m_size;
        }

        void remove(size_t index, size_t n) {
            assert(index + n <= m_size);
            while (index + n < m_size) {
                m_data[index] = m_data[index+n];
                ++index;
            }
            m_size -= n;
        }

        void resize(size_t size) {
            if (size > m_size) {
                reserve(size);
                for (size_t i = m_size; i < size; i++){
                    new (&m_data[i]) T();
                }
            }
            m_size = size;
        }


        void resize(size_t size, T const & value) {
            if (size > m_size) {
                reserve(size);
                for (size_t i = m_size; i < size; i++){
                    new (&m_data[i]) T(value);
                }
            }
            m_size = size;
        }

        void clear() {
            if (m_data != nullptr) {
                std::destroy(&m_data[0], &m_data[m_size]);
                m_allocator->free(m_data);
            }
            m_data = nullptr;
            m_size = 0;
            m_capacity = 0;
        }

        void reserve(size_t capacity) {
            if (capacity <= m_capacity) {
                return;
            }

            T* newPointer = static_cast<T*>(m_allocator->allocate(capacity * sizeof(T),
                                            m_alignment));

            if (m_data != nullptr) {
                for (size_t i = 0; i < m_size; i++){
                    new (&newPointer[i]) T(m_data[i]);
                }

                m_allocator->free(m_data);
            }

            m_capacity = capacity;
            m_data = newPointer;
        }

        inline bool empty() const { return m_size == 0; }

        inline T& front() { assert(!empty()); return m_data[0]; }
        inline T const & front() const { assert(!empty()); return m_data[0]; }
        inline T& back() { assert(!empty()); return m_data[m_size - 1]; }
        inline T const & back() const { assert(!empty()); return m_data[m_size - 1]; }

        inline size_t size() const { return m_size; }
        inline size_t capacity() const { return m_capacity; }
        inline T* data() const { return m_data; }

        inline T* begin() const { return &m_data[0]; }
        inline T* end() const { return &m_data[m_size]; }

    private:
        // Capacity increase when size exceeds capacity
        static constexpr size_t CAPACITY_INCREASE_CONSTANT = 2;
        // Start of vector.
        T* m_data;
        // Alignment
        size_t m_alignment;
        // Number of elements currently in vector
        size_t m_size;
        // Buffer size in bytes.
        size_t m_capacity;
        // Allocator
        ContainerAllocator* m_allocator;
    };
}

#endif