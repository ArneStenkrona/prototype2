#include "src/memory/allocator.h"

#include <cstdint>
#include <cassert>

namespace prt {
    template <typename T>
    class array {
    public:
    array(): _arrayPointer(nullptr), _size(0) {}

    array(Allocator& allocator, size_t size, size_t alignment = sizeof(T)) {
        _arrayPointer = static_cast<T*> 
                        (allocator
                        .allocate(size * sizeof(T), 
                                alignment));
        for (size_t i = 0; i < size; i++) {
            _arrayPointer[i] = T();
        }

        _size = size;
    }

        T operator [](size_t index) const { 
            assert(index < _size);
            return _arrayPointer[index];
        }

        T & operator [](size_t index) {
            assert(index < _size);
            return _arrayPointer[index];
        }

        inline size_t size() const { return _size; }
    private:
        T* _arrayPointer;
        size_t _size; 
    };
}