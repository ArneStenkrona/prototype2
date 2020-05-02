#ifndef PRT_PRIORITY_QUEUE_H
#define PRT_PRIORITY_QUEUE_H

#include "src/container/vector.h"

#include <functional>

namespace prt {
    template<class T, class Compare = std::less<T> >
    class priority_queue {
    public:
        explicit priority_queue(ContainerAllocator& allocator)
        : m_container(allocator) {}

        priority_queue(): priority_queue(ContainerAllocator::getDefaultContainerAllocator()) {}

        T const & top() const { return m_container.front(); }

        void push(T const & t) {
            size_t index = m_container.size();
            m_container.push_back(t);

            while (index != 0) {
                size_t parentIndex = (index / 2 + (index % 2 != 0)) - 1;
                T elem = m_container[index];
                if (compare(t, m_container[parentIndex])) {
                    m_container[index] = m_container[parentIndex];
                    m_container[parentIndex] = elem;
                }
                index = parentIndex;
            }
        }

        void pop() {
            m_container.front() = m_container.back();
            m_container.pop_back();

            size_t index = 0;
            while (true) {
                size_t leftIndex = (index * 2) + 1;
                size_t rightIndex = leftIndex + 1;

                size_t childIndex;
                bool isLeft = leftIndex < m_container.size();
                bool isRight = rightIndex < m_container.size();
                if (isLeft && isRight) {
                    // both left and right children are present
                    childIndex = compare(m_container[leftIndex], m_container[rightIndex]) ? leftIndex :
                                                                                               rightIndex;
                } else if (isLeft) {
                    // only left child is persent
                    childIndex = leftIndex;
                } else if (isRight) {
                    // only right child is present
                    childIndex = rightIndex;
                } else {
                    // node has no children
                    return;
                }
                
                T child = m_container[childIndex];
                if (compare(child, m_container[index])) {
                    m_container[childIndex] = m_container[index];
                    m_container[index] = child;
                }
                index = childIndex;
            }
        }

        inline size_t size() const { return m_container.size(); }
        inline bool empty() const { return m_container.empty(); }

    private:
        static constexpr Compare compare = Compare();
        prt::vector<T> m_container;
    };
}

#endif