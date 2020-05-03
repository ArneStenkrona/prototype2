#ifndef AABB_TREE_H
#define AABB_TREE_H

#include "shapes.h"
#include "src/container/vector.h"

#include <cstdint>

// thank you, Andy Gaul: https://www.randygaul.net/2013/08/06/dynamic-aabb-tree/
class DynamicAABBTree {
public:
    /**
     * The cost heuristic is defined as
     * the total surface area of all nodes
     * excluding the leaf nodes
     * @return the cost of the tree
     */
    float cost() const;

private:
    struct Node;
    static constexpr float buffer = 0.05f; 
    int32_t rootIndex;

    void insertLeaf(int32_t objectIndex, AABB const & aabb);
    void remove(AABB const & aabb);
    void update();

    void synchHierarchy(int32_t index);
    void rotate(int32_t index);
    void swap(int32_t indexA, int32_t indexB);
    
    int32_t findBestSibling(Node const & leaf) const;

    struct NodeCost;
    NodeCost siblingCost(Node const & leaf, int32_t siblingIndex, float inheritedCost) const;

    struct Node {
        bool isLeaf() const { return right != nullIndex; }

        AABB aabb;

        union {
            int32_t parent;
            int32_t next; // free list
        };

        union {
            struct {
                int32_t left;
                int32_t right;
            };

            void *userData;
        };

        // leaf = 0, free nodes = -1
        int32_t height = 0;

        static constexpr int32_t nullIndex = -1;
    };

    struct NodeCost {
        int32_t index;
        float directCost;
        float inheritedCost;
        friend bool operator<(NodeCost const & lhs, NodeCost const & rhs) 
                            { return lhs.directCost + lhs.inheritedCost < rhs.directCost + rhs.inheritedCost; }
    };

    prt::vector<Node> m_nodes;
};

#endif
