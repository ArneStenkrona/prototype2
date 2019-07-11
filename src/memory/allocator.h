#include <cstddef>

class Allocator {
public:
    virtual void* allocate(size_t size, size_t alignment) = 0;
    virtual void free(void* pointer) = 0;
};