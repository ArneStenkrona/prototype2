#include "aabb_tree.h"

#include "src/container/priority_queue.h"

#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <cassert>

void DynamicAABBTree::query(ColliderTag caller, AABB const & aabb, prt::vector<ColliderTag> & tags) {
    if (m_size == 0) {
        return;
    }
    
    prt::vector<int32_t> nodeStack;
    nodeStack.push_back(rootIndex);
    while (!nodeStack.empty()) {
        int32_t index = nodeStack.back();
        nodeStack.pop_back();
        Node const & node = m_nodes[index];
        if (AABB::intersect(node.aabb, aabb)) {
            if (node.isLeaf()) {
                if (caller != node.colliderTag) {
                    tags.push_back(node.colliderTag);
                }
            } else {
                nodeStack.push_back(node.left);
                nodeStack.push_back(node.right);
            }
        }
    }
}

void DynamicAABBTree::query(ColliderTag caller, AABB const & aabb, 
                            prt::vector<uint16_t> & meshIndices,
                            prt::vector<uint16_t> & capsuleIndices,
                            ColliderType type) {
    if (m_size == 0) {
        return;
    }

    prt::vector<int32_t> nodeStack;
    nodeStack.push_back(rootIndex);
    while (!nodeStack.empty()) {
        int32_t index = nodeStack.back();
        nodeStack.pop_back();
        Node const & node = m_nodes[index];
        if (AABB::intersect(node.aabb, aabb)) {
            if (node.isLeaf()) {
                if (caller != node.colliderTag && type == node.colliderTag.type) {
                    switch (node.colliderTag.shape) {
                        case ColliderShape::COLLIDER_SHAPE_MESH: 
                            meshIndices.push_back(node.colliderTag.index);
                            break;
                        case ColliderShape::COLLIDER_SHAPE_CAPSULE:
                            capsuleIndices.push_back(node.colliderTag.index);
                            break;
                        default:
                            break;
                    }
                }
            } else {
                nodeStack.push_back(node.left);
                nodeStack.push_back(node.right);
            }
        }
    }
}

void DynamicAABBTree::queryRaycast(glm::vec3 const& origin,
                                   glm::vec3 const& direction,
                                   float maxDistance,
                                   prt::vector<ColliderTag> & tags) {
    if (m_size == 0) {
        return;
    }
    
    prt::vector<int32_t> nodeStack;
    nodeStack.push_back(rootIndex);
    while (!nodeStack.empty()) {
        int32_t index = nodeStack.back();
        nodeStack.pop_back();
        Node const & node = m_nodes[index];
        if (AABB::intersectRay(node.aabb, origin, direction, maxDistance)) {
            if (node.isLeaf() && node.colliderTag.type == COLLIDER_TYPE_COLLIDE) {
                tags.push_back(node.colliderTag);
            } else {
                nodeStack.push_back(node.left);
                nodeStack.push_back(node.right);
            }
        }
    }
}

void DynamicAABBTree::insert(ColliderTag const * tags, AABB const * aabbs, size_t n,
                             int32_t * treeIndices) {
    for (size_t i = 0; i < n; ++i) {
        treeIndices[i] = insertLeaf(tags[i], aabbs[i]);
    }
}

void DynamicAABBTree::update(int32_t * treeIndices, AABB const * aabbs, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        Node & node = m_nodes[treeIndices[i]];
        if (!node.aabb.contains(aabbs[i])) {
            ColliderTag tag = node.colliderTag;
            remove(treeIndices[i]);
            treeIndices[i] = insertLeaf(tag, aabbs[i]);
        }
    }
}

void DynamicAABBTree::updateTags(int32_t * treeIndices, ColliderTag const * colliderTags, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        Node & node = m_nodes[treeIndices[i]];
        node.colliderTag = colliderTags[i];
    }
}

void DynamicAABBTree::remove(int32_t * treeIndices, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        remove(treeIndices[i]);
    }
}

float DynamicAABBTree::cost() const {
    float cost = 0.0f;
    for (auto const & node : m_nodes) {
        cost += node.aabb.area();
    }
    return cost;
}

int32_t DynamicAABBTree::insertLeaf(ColliderTag tag, AABB const & aabb) {
    // insert new node into vector
    int32_t leafIndex = allocateNode();

    m_nodes[leafIndex].colliderTag = tag;
    // add buffer to aabb 
    m_nodes[leafIndex].aabb.lowerBound = aabb.lowerBound - buffer;
    m_nodes[leafIndex].aabb.upperBound = aabb.upperBound + buffer;
    
    if (rootIndex == Node::NULL_INDEX) {
        rootIndex = leafIndex;
        return leafIndex;
    }
    // traverse tree to find suitable place of insertion
    // stage 1: find the best sibling for the new leaf
    int32_t siblingIndex = findBestSibling(leafIndex);

    // stage 2: create a new parent
    int32_t oldParentIndex = m_nodes[siblingIndex].parent;
    int32_t newParentIndex = allocateNode(); // warning, this may invalidate references
    Node & newParent = m_nodes[newParentIndex];
    newParent.parent = oldParentIndex;
    newParent.aabb = m_nodes[leafIndex].aabb + m_nodes[siblingIndex].aabb;
    newParent.height = 1 + m_nodes[siblingIndex].height;

    if (oldParentIndex != Node::NULL_INDEX) {
        Node & oldParent = m_nodes[oldParentIndex];
        // The sibling was not the root
        if (oldParent.left == siblingIndex) {
            oldParent.left = newParentIndex;
        } else {
            oldParent.right = newParentIndex;
        }
    } else {
        // The sibling was the root
        rootIndex = newParentIndex;
    }
    newParent.left = siblingIndex;
    newParent.right = leafIndex;
    m_nodes[siblingIndex].parent = newParentIndex;
    m_nodes[leafIndex].parent = newParentIndex;

    // stage 3: walk back up the tree refitting AABBs and applying rotations
    synchHierarchy(m_nodes[leafIndex].parent);

    return leafIndex;
}

void DynamicAABBTree::remove(int32_t index) {
    Node & n = m_nodes[index];
    assert(n.isLeaf());

    if (index == rootIndex) {
        rootIndex = Node::NULL_INDEX;
        freeNode(index);
        return;
    }

    int32_t parent = n.parent;

    int32_t siblingIndex;
    if (m_nodes[parent].left == index) {
        siblingIndex = m_nodes[parent].right;
    } else {
        siblingIndex = m_nodes[parent].left;
    }

    if (m_nodes[parent].parent != Node::NULL_INDEX)  {
        int32_t grandParent = m_nodes[parent].parent;
        if (m_nodes[grandParent].left == n.parent) {
            m_nodes[grandParent].left = siblingIndex;
        } else {
            m_nodes[grandParent].right = siblingIndex;
        }

        m_nodes[siblingIndex].parent = grandParent;
        freeNode(n.parent);

        synchHierarchy(grandParent);
    } else {
        rootIndex = siblingIndex;
        m_nodes[siblingIndex].parent = Node::NULL_INDEX;
        freeNode(n.parent);
    }

    freeNode(index);
}

void DynamicAABBTree::freeNode(int32_t index) {
    m_nodes[index].next = freeHead;
    m_nodes[index].height = -1;
    freeHead = index;
    --m_size;
}

int32_t DynamicAABBTree::allocateNode() {
    int32_t index;
    if (freeHead == Node::NULL_INDEX) {
        index = m_nodes.size();
        m_nodes.push_back({});
    } else {
        index = freeHead;
        freeHead = m_nodes[freeHead].next;
        m_nodes[index] = {};
    }
    ++m_size;
    return index;
}

void DynamicAABBTree::synchHierarchy(int32_t index) {
    while (index != Node::NULL_INDEX) {
        balance(index);
        
        int32_t left = m_nodes[index].left;
        int32_t right = m_nodes[index].right;
        
        m_nodes[index].height = 1 + std::max(m_nodes[left].height, m_nodes[right].height);
        m_nodes[index].aabb = m_nodes[left].aabb + m_nodes[right].aabb;

        // balance(index);
        index = m_nodes[index].parent;
    }
}

void DynamicAABBTree::balance(int32_t index) {
    Node & node = m_nodes[index];
    if (node.height < 2) {
        return;
    }

    int32_t ileft = node.left;
    int32_t iright = node.right;
    Node & left = m_nodes[ileft];
    Node & right = m_nodes[iright];

    int32_t difference = left.height - right.height;
    if (difference > 1) {
        // left is higher than right, rotate left up
        int32_t * ia = m_nodes[left.left].height > m_nodes[left.right].height ? &left.left : &left.right;
        Node & a = m_nodes[*ia];
        right.parent = ileft;
        a.parent = index;

        node.right = *ia;
        *ia = iright;

        left.aabb = m_nodes[left.left].aabb + m_nodes[left.right].aabb;
        left.height = std::max(m_nodes[left.left].height, m_nodes[left.right].height) + 1;
    } else if (difference < -1) {
        // right is higher than left, rotate right up
        int32_t * ia = m_nodes[right.left].height > m_nodes[right.right].height ? &right.left : &right.right;
        Node & a = m_nodes[*ia];
        left.parent = iright;
        a.parent = index;

        node.left = *ia;
        *ia = ileft;

        right.aabb = m_nodes[right.left].aabb + m_nodes[right.right].aabb;
        right.height = std::max(m_nodes[right.left].height, m_nodes[right.right].height) + 1;
    }
    node.height = std::max(m_nodes[node.left].height, m_nodes[node.right].height) + 1;
}

int32_t DynamicAABBTree::findBestSibling(int32_t leafIndex) const {
    Node const & leaf = m_nodes[leafIndex];
    int32_t bestSibling = rootIndex;
    float bestCost = (leaf.aabb + m_nodes[rootIndex].aabb).area();
    prt::priority_queue<NodeCost> q;
    
    q.push(siblingCost(leaf, rootIndex, 0.0f));
    while (!q.empty()) {
        NodeCost nc = q.top();
        q.pop();
        float currentCost = nc.directCost + nc.inheritedCost;
        if (currentCost < bestCost) {
            bestCost = currentCost;
            bestSibling = nc.index;
        }

        Node const & curr = m_nodes[nc.index];
        if (!curr.isLeaf()) {
            float inheritedCost = nc.inheritedCost + (leaf.aabb + curr.aabb).area() - curr.aabb.area();
            float cLow = leaf.aabb.area() + inheritedCost;
            if (cLow < bestCost) {
                NodeCost left = siblingCost(leaf, curr.left, inheritedCost);
                NodeCost right = siblingCost(leaf, curr.right, inheritedCost);
                q.push(left);
                q.push(right);
            }
        }
    }

    return bestSibling;
}

DynamicAABBTree::NodeCost DynamicAABBTree::siblingCost(Node const & leaf, int32_t siblingIndex, float inheritedCost) const {
    NodeCost cost;
    cost.index = siblingIndex;

    float directCost = (leaf.aabb + m_nodes[siblingIndex].aabb).area();

    cost.directCost = directCost;
    cost.inheritedCost = inheritedCost;
    return cost;
}
