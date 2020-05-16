#ifndef AABB_TREE_H
#define AABB_TREE_H

#include "aabb.h"
#include "src/container/vector.h"

#include "colliders.h"

#include <cstdint>

// thank you, Andy Gaul: https://www.randygaul.net/2013/08/06/dynamic-aabb-tree/
class DynamicAABBTree {
public:
    /**
     * Finds all intersecting nodes for aabb
     * @param aabb aabb of query object
     * @param objectIndices vector to store object indices of nodes
     */
    void query(AABB const & aabb, prt::vector<uint32_t> & objectIndices);
    
    /**
     * Inserts aabbs along with their object indices into the tree
     * @param objectIndices address of the start of the range of object indices
     * @param aabbs address of the start of the range of aabbs
     * @param n number of aabbs to be inserted
     * @param treeIndices address to the start of the range that will 
     *                    recieve the resulting indices in the tree
     */
    void insert(uint32_t const * objectIndices, AABB const * aabbs, size_t n,
                int32_t * treeIndices);

    /**
     * Updates the nodes given by a range of tree indices which is then
     * made up-to-date
     * @param treeIndices address of the start of the range of tree indices
     *                    these will be updated
     * @param aabbs address of the start of the range of aabbs
     * @param n number of aabbs to be inserted
     */
    void update(int32_t * treeIndices, AABB const * aabbs, size_t n);

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

    int32_t freeHead = Node::nullIndex; // free list
    int32_t m_size;
    prt::vector<Node> m_nodes;

    int32_t insertLeaf(uint32_t objectIndex, AABB const & aabb);
    void remove(int32_t index);

    int32_t allocateNode();

    void synchHierarchy(int32_t index);
    void balance(int32_t index);

    int32_t findBestSibling(int32_t leafIndex) const;

    struct NodeCost;
    NodeCost siblingCost(Node const & leaf, int32_t siblingIndex, float inheritedCost) const;

    struct Node {
        AABB aabb;

        // leaf = 0, free nodes = -1
        int32_t height = 0;

        static constexpr int32_t nullIndex = -1;

        bool isLeaf() const { return right == nullIndex; }

        union {
            int32_t parent = nullIndex;
            int32_t next; // free list
        };

        // union {
            // struct {
                int32_t left = nullIndex;
                int32_t right = nullIndex;
            // };
            // struct {
                uint32_t objectIndex = -1;
                // int32_t notUsed;
            // };
            // void *userData;
        // };

    };

    struct NodeCost {
        int32_t index;
        float directCost;
        float inheritedCost;
        friend bool operator<(NodeCost const & lhs, NodeCost const & rhs) 
                            { return lhs.directCost + lhs.inheritedCost < rhs.directCost + rhs.inheritedCost; }
    };
};

#endif
