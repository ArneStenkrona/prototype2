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
     * @param tags vector to store collider tags of nodes
     */
    void query(ColliderTag caller, AABB const & aabb, prt::vector<ColliderTag> & tags);
    /**
     * Finds all intersecting nodes for aabb
     * @param aabb aabb of query object
     * @param meshIndices vector to store mesh indices
     * @param capsuleIndices vector to store capsule indices
     */
    void query(ColliderTag caller, AABB const & aabb, 
               prt::vector<uint16_t> & meshIndices,
               prt::vector<uint16_t> & capsuleIndices,
               ColliderType type);

    /**
     * Finds all intersecting nodes for raycast
     * @param origin origin of the ray
     * @param direction direction of the ray
     * @param maxDistance maximum length of the ray
     * @param tags vector to store collider tags of nodes
     */
    void queryRaycast(glm::vec3 const& origin,
                      glm::vec3 const& direction,
                      float maxDistance,
                      prt::vector<ColliderTag> & tags);
    
    /**
     * Inserts aabbs along with their collider tags into the tree
     * @param tags address of the start of the range of collider tags
     * @param aabbs address of the start of the range of aabbs
     * @param n number of aabbs to be inserted
     * @param treeIndices address to the start of the range that will 
     *                    recieve the resulting indices in the tree
     */
    void insert(ColliderTag const * tags, AABB const * aabbs, size_t n,
                int32_t * treeIndices);
                
    /**
     * Updates the nodes given by a range of tree indices which is then
     * made up-to-date
     * @param treeIndices address of the start of the range of tree indices
     *                    that will be updated
     * @param aabbs address of the start of the range of aabbs
     * @param n number of aabbs to be inserted
     */
    void update(int32_t * treeIndices, AABB const * aabbs, size_t n);

    /**
     * Updates the tags of nodes given by a range of tree indices
     * @param treeIndices address of the start of the range of tree indices
     *                    that will be updated
     * @param colliderTags new tags
     * @param n number of tags to be updated
     */
    void updateTags(int32_t * treeIndices, ColliderTag const * colliderTags, size_t n);

    /**
     * @param treeIndices address of the start of the range of tree indices
     *                    that will be removed
     * @param n number of nodes to be removed
     */
    void remove(int32_t * treeIndices, size_t n);

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
    int32_t rootIndex = Node::NULL_INDEX;

    int32_t freeHead = Node::NULL_INDEX; // free list
    int32_t m_size = 0;
    prt::vector<Node> m_nodes;

    int32_t insertLeaf(ColliderTag tag, AABB const & aabb);
    void remove(int32_t index);

    void freeNode(int32_t index);

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

        static constexpr int32_t NULL_INDEX = -1;

        bool isLeaf() const { return right == NULL_INDEX; }

        union {
            int32_t parent = NULL_INDEX;
            int32_t next; // free list
        };

        // union {
            // struct {
                int32_t left = NULL_INDEX;
                int32_t right = NULL_INDEX;
            // };
            // struct {
                ColliderTag colliderTag;
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
