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
            Size = N,
            DataSize = sizeof(T) * N
        };
        
        T m_data[N];    

        T operator [](size_t index) const { 
            assert(index < Size);
            return m_data[index];
        }

        T & operator [](size_t index) {
            assert(index < Size);
            return m_data[index];
        }

        constexpr inline size_t size() const { return Size; }
        constexpr inline T* data() { return &m_data[0]; }
        constexpr inline const T* data() const { return const_cast<const T*>(&m_data[0]); }

        constexpr inline T* begin() { return &m_data[0]; }
        constexpr inline const T* begin() const { return const_cast<const T*>(&m_data[0]); }
        constexpr inline T* end() { return &m_data[Size]; }
        constexpr inline const T* end() const { return const_cast<const T*>(&m_data[Size]); }

        constexpr inline size_t data_size() const { return DataSize; }
    };
}

#endif