#include "aabb_tree.h"

#include "src/container/priority_queue.h"

#include <algorithm>
#include <cassert>

float DynamicAABBTree::cost() const {
    float cost = 0.0f;
    for (auto const & node : m_nodes) {
        cost += node.aabb.area();
    }
    return cost;
}

void DynamicAABBTree::insertLeaf(int32_t objectIndex, AABB const & aabb) {
    // insert new node into vector
    int32_t leafIndex = m_nodes.size();
    if (m_nodes.size() == 0) {
        rootIndex = leafIndex;
        return;
    }

    m_nodes.push_back({});
    Node & leaf = m_nodes.back();
    // add objectIndex to user data
    *reinterpret_cast<int32_t*>(leaf.userData) = objectIndex;
    // add buffer to aabb 
    leaf.aabb.lowerBound = aabb.lowerBound - buffer;
    leaf.aabb.upperBound = aabb.upperBound + buffer;
    // traverse tree to find suitable place of insertion
    // stage 1: find the best sibling for the new leaf
    int32_t siblingIndex = findBestSibling(leaf);
    Node & sibling = m_nodes[siblingIndex];

    // stage 2: create a new parent
    int32_t oldParentIndex = m_nodes[siblingIndex].parent;
    int32_t newParentIndex = m_nodes.size();
    m_nodes.push_back({});
    Node & newParent = m_nodes.back();
    Node & oldParent = m_nodes[oldParentIndex];
    newParent.parent = oldParentIndex;
    newParent.aabb = leaf.aabb + sibling.aabb;

    if (oldParentIndex != Node::nullIndex) {
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
    sibling.parent = newParentIndex;
    leaf.parent = newParentIndex;

    newParent.height = std::max(m_nodes[newParent.left].left, m_nodes[newParent.right].height) + 1;
    oldParent.height = std::max(m_nodes[oldParent.left].left, m_nodes[oldParent.right].height) + 1;

    // stage 3: walk back up the tree refitting AABBs and applying rotations
    synchHierarchy(leafIndex);
}


void DynamicAABBTree::synchHierarchy(int32_t index) {
    while (index != Node::nullIndex) {
        int32_t left = m_nodes[index].left;
        int32_t right = m_nodes[index].right;

        m_nodes[index].height = 1 + std::max(m_nodes[left].height, m_nodes[right].height);
        m_nodes[index].aabb = m_nodes[left].aabb + m_nodes[right].aabb;

        rotate(index);

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

        Node const & n = m_nodes[index];
        Node const & left = m_nodes[n.left];
        Node const & right = m_nodes[n.right];
        int32_t difference = left.height - right.height;
        assert(difference > -2 && difference < 2);
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

void DynamicAABBTree::swap(int32_t indexA, int32_t indexB) {
    Node & a = m_nodes[indexA];
    Node & b = m_nodes[indexB];
    Node & parentA = m_nodes[a.parent];
    Node & parentB = m_nodes[b.parent];

    // parents be notified of switch
    if (parentA.left == indexA) {
        parentA.left = indexB;
    } else {
        parentA.right = indexB;
    }
    if (parentB.left == indexB) {
        parentB.left = indexA;
    } else {
        parentB.right = indexA;
    }
    // set new parents
    int32_t tempIndex = a.parent;
    a.parent = b.parent;
    b.parent = tempIndex;
}

int32_t DynamicAABBTree::findBestSibling(Node const & leaf) const {
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
    // int32_t parentIndex = m_nodes[siblingIndex].parent;
    // inheritedCost += (m_nodes[parentIndex].aabb + leaf.aabb).area() - m_nodes[parentIndex].aabb.area();

    cost.directCost = directCost;
    cost.inheritedCost = inheritedCost;
    return cost;
}

