#include "aabb_tree.h"

#include "src/container/priority_queue.h"

#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <cassert>

void DynamicAABBTree::query(AABB const & aabb, prt::vector<uint32_t> & objectIndices) {
    prt::vector<int32_t> nodeStack;
    nodeStack.push_back(rootIndex);
    while (!nodeStack.empty()) {
        int32_t index = nodeStack.back();
        nodeStack.pop_back();
        Node const & node = m_nodes[index];
        if (AABB::intersect(node.aabb, aabb)) {
            if (node.isLeaf()) {
                objectIndices.push_back(node.objectIndex);
            } else {
                nodeStack.push_back(node.left);
                nodeStack.push_back(node.right);
            }
        }
    }
}

void DynamicAABBTree::insert(uint32_t const * objectIndices, AABB const * aabbs, size_t n,
                             int32_t * treeIndices) {
    for (size_t i = 0; i < n; ++i) {
        treeIndices[i] = insertLeaf(objectIndices[i], aabbs[i]);
        ++numberOfLeaves;
    }
}

void DynamicAABBTree::update(int32_t * treeIndices, AABB const * aabbs, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        Node & node = m_nodes[treeIndices[i]];
        if (!node.aabb.contains(aabbs[i])) {
            int32_t objectIndex = node.objectIndex;
            remove(treeIndices[i]);
            treeIndices[i] = insertLeaf(objectIndex, aabbs[i]);
        }
    }
}

float DynamicAABBTree::cost() const {
    float cost = 0.0f;
    for (auto const & node : m_nodes) {
        cost += node.aabb.area();
    }
    return cost;
}

int32_t DynamicAABBTree::insertLeaf(uint32_t objectIndex, AABB const & aabb) {
    // insert new node into vector
    int32_t leafIndex = allocateNode();
    ++m_size;

    // Node & leaf = m_nodes[leafIndex];
    // add objectIndex to user data
    // *reinterpret_cast<int32_t*>(leaf.userData) = objectIndex;
    m_nodes[leafIndex].objectIndex = objectIndex;
    // add buffer to aabb 
    m_nodes[leafIndex].aabb.lowerBound = aabb.lowerBound - buffer;
    m_nodes[leafIndex].aabb.upperBound = aabb.upperBound + buffer;
    
    if (m_size == 1) {
        rootIndex = leafIndex;
        return leafIndex;
    }
    // traverse tree to find suitable place of insertion
    // stage 1: find the best sibling for the new leaf
    int32_t siblingIndex = findBestSibling(leafIndex);
    // Node & sibling = m_nodes[siblingIndex];

    // stage 2: create a new parent
    int32_t oldParentIndex = m_nodes[siblingIndex].parent;
    int32_t newParentIndex = allocateNode(); // warning, this may invalidate references
    Node & newParent = m_nodes[newParentIndex];
    newParent.parent = oldParentIndex;
    newParent.aabb = m_nodes[leafIndex].aabb + m_nodes[siblingIndex].aabb;

    if (oldParentIndex != Node::nullIndex) {
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

    newParent.height = std::max(m_nodes[newParent.left].left, m_nodes[newParent.right].height) + 1;
    if (oldParentIndex != Node::nullIndex) {
        Node & oldParent = m_nodes[oldParentIndex];
        oldParent.height = std::max(m_nodes[oldParent.left].left, m_nodes[oldParent.right].height) + 1;
    }

    // stage 3: walk back up the tree refitting AABBs and applying rotations
    synchHierarchy(m_nodes[leafIndex].parent);

    return leafIndex;
}

void DynamicAABBTree::remove(int32_t index) {
    Node & n = m_nodes[index];
    assert(n.isLeaf());
    Node & parent = m_nodes[n.parent];
    int32_t siblingIndex;
    if (parent.left == index) {
        siblingIndex = parent.right;
    } else {
        siblingIndex = parent.left;
    }
    
    Node & grandParent = m_nodes[parent.parent];
    if (grandParent.left == n.parent) {
        grandParent.left = siblingIndex;
    } else {
        grandParent.right = siblingIndex;
    }
    m_nodes[siblingIndex].parent = parent.parent;

    n.height = -1;
    parent.height = -1;

    parent.next = freeHead;
    n.next = n.parent;
    freeHead = index;

    --m_size;
}

int32_t DynamicAABBTree::allocateNode() {
    int32_t index;
    if (freeHead == Node::nullIndex) {
        index = m_nodes.size();
        m_nodes.push_back({});
    } else {
        index = freeHead;
        m_nodes[freeHead] = {};
        freeHead = m_nodes[freeHead].next;
    }
    // m_nodes[index].height = 0;
    return index;
}

void DynamicAABBTree::synchHierarchy(int32_t index) {
    while (index != Node::nullIndex) {
        int32_t left = m_nodes[index].left;
        int32_t right = m_nodes[index].right;
        
        m_nodes[index].height = 1 + std::max(m_nodes[left].height, m_nodes[right].height);
        m_nodes[index].aabb = m_nodes[left].aabb + m_nodes[right].aabb;

        // rotate(index);

        index = m_nodes[index].parent;
    }
}

void DynamicAABBTree::rotate(int32_t index) {
    int32_t childIndex = Node::nullIndex;
    int32_t grandChildIndex = Node::nullIndex;
    // find first unbalanced node
    while (index != rootIndex) {  
        grandChildIndex = childIndex;
        childIndex = index;
        index = m_nodes[index].parent;

        if (grandChildIndex == Node::nullIndex) {
            continue;
        }

        Node const & n = m_nodes[index];
        Node const & left = m_nodes[n.left];
        Node const & right = m_nodes[n.right];
        int32_t difference = left.height - right.height;
        assert(difference >= -2 && difference <= 2);

        // unbalanced tree
        if (difference > 1) {
            // right is higher than left
            swap(n.left, grandChildIndex);
            return;
        } else if (difference < -1) {
            // left is higher than right
            swap(n.right, grandChildIndex);
            return;
        }
    }
}

void DynamicAABBTree::swap(int32_t shorter, int32_t higher) {
    Node & a = m_nodes[shorter];
    Node & b = m_nodes[higher];
    Node & parentA = m_nodes[a.parent];
    Node & parentB = m_nodes[b.parent];

    // parents be notified of switch
    if (parentA.left == shorter) {
        parentA.left = higher;
    } else {
        parentA.right = higher;
    }
    if (parentB.left == higher) {
        parentB.left = shorter;
    } else {
        parentB.right = shorter;
    }
    // set new parents
    int32_t tempIndex = a.parent;
    a.parent = b.parent;
    b.parent = tempIndex;
    
    parentB.aabb = m_nodes[parentB.left].aabb + m_nodes[parentB.right].aabb;
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
            float inheritedCost = nc.inheritedCost + (leaf.aabb +curr.aabb).area() - curr.aabb.area();
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
