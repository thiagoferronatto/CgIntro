/*
 * Copyright (c) 2009 Erin Catto http://www.box2d.org
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef __DynamicTree_h
#define __DynamicTree_h

#include <algorithm>
#include <cassert>
#include <memory>
#include <stack>

#include "aabb.hpp"
#include "glm.hpp"

namespace cg { // begin namespace cg

//
// Auxiliary functions
//
inline auto boundsArea(const AABB &bounds) {
  auto ab{bounds.b - bounds.a};
  return 2.0f * (ab.x * ab.y + ab.x * ab.z + ab.y * ab.z);
}

inline AABB operator+(const AABB &a, const AABB &b) {
  return {glm::min(a.a, b.a), glm::max(a.b, b.b)};
}

inline auto testOverlap(const AABB &a, const AABB &b) {
  const auto &a_min = a.a;
  const auto &a_max = a.b;
  const auto &b_min = b.a;
  const auto &b_max = b.b;

  if (a_max.x < b_min.x || a_min.x > b_max.x)
    return false;
  if (a_max.y < b_min.y || a_min.y > b_max.y)
    return false;
  if (a_max.z < b_min.z || a_min.z > b_max.z)
    return false;
  return true;
}

//
// Forward definitions
//
class DynamicTree;
class DynamicTreeIterator;

/////////////////////////////////////////////////////////////////////
//
// DynamicTreeNode: dynamic tree node class
// ===============
struct DynamicTreeNode {
  // private:
  static constexpr int null = -1;

  AABB _bounds;
  void *_userData;
  union {
    int _parent;
    int _next;
  };
  int _children[2];
  int _height;

  auto isLeaf() const { return _children[0] == null; }

  friend DynamicTree;
  friend DynamicTreeIterator;

}; // DynamicTreeNode

/////////////////////////////////////////////////////////////////////
//
// DynamicTreeIterator: dynamic tree iterator class
// ===================
class DynamicTreeIterator {
public:
  using Node = DynamicTreeNode;
  using iterator = DynamicTreeIterator;

  DynamicTreeIterator(const Node *nodes, int index)
      : _nodes{nodes}, _index{index} {
    // do nothing
  }

  iterator &operator++();

  iterator operator++(int) {
    iterator temp{*this};

    operator++();
    return temp;
  }

  bool operator==(const iterator &other) const {
    return _nodes == other._nodes && _index == other._index;
  }

  bool operator!=(const iterator &other) const { return !operator==(other); }

  auto index() const { return _index; }

  bool isLeaf() const {
    assert(_index != Node::null);
    return _nodes[_index].isLeaf();
  }

  const auto &bounds() const {
    assert(_index != Node::null);
    return _nodes[_index]._bounds;
  }

  auto userData() const {
    assert(_index != Node::null);
    return _nodes[_index]._userData;
  }

  auto operator*() const { return _nodes[_index]._bounds; }

private:
  const Node *_nodes;
  int _index;

}; // DynamicTreeIterator

DynamicTreeIterator &DynamicTreeIterator::operator++() {
  if (_index != Node::null) {
    const auto node = _nodes + _index;
    auto nextIndex = node->_children[0];

    if (nextIndex == Node::null) {
      auto temp = _index;
      auto parentIndex = node->_parent;

      if (parentIndex != Node::null)
        for (;;) {
          const auto parent = _nodes + parentIndex;

          nextIndex = parent->_children[1];
          if (nextIndex != temp)
            break;
          nextIndex = Node::null;
          temp = parentIndex;
          parentIndex = parent->_parent;
          if (parentIndex == Node::null)
            break;
        }
    }
    _index = nextIndex;
  }
  return *this;
}

/////////////////////////////////////////////////////////////////////
//
// DynamicTree: dynamic tree class
// ===========
class DynamicTree {
public:
  using bounds_type = AABB;
  using iterator = DynamicTreeIterator;

  ~DynamicTree() { delete[] _nodes; }

  DynamicTree();

  int add(const bounds_type &bounds, void *userData = nullptr);
  void remove(int index);

  auto nodeCount() const { return _nodeCount; }

  auto get(int index) const {
    assert(0 <= index && index < _nodeCapacity);
    assert(_nodes[index].isLeaf());

    return iterator{_nodes, index};
  }

  auto begin() const { return iterator{_nodes, _root}; }

  auto end() const { return iterator{_nodes, Node::null}; }

  auto bounds() const {
    return _root == Node::null ? bounds_type{} : _nodes[_root]._bounds;
  }

  template <typename T> void query(T *handler, const bounds_type &bounds) const;

  int root() const { return _root; }

  auto getNode(int index) const { return _nodes + index; }

private:
  using Node = DynamicTreeNode;

  Node *_nodes;
  int _root;
  int _nodeCount;
  int _nodeCapacity;
  int _freeList;

  void resize();
  int allocateNode();
  void freeNode(int);
  void addLeafNode(int);
  void removeLeafNode(int);
  int balance(int);

  static auto allocateNodes(int n) {
    return static_cast<Node *>(::operator new(n * sizeof(Node)));
  }

  friend iterator;

}; // DynamicTree

DynamicTree::DynamicTree() : _nodes{nullptr} {
  _root = _freeList = Node::null;
  _nodeCount = _nodeCapacity = 0;
}

inline void DynamicTree::resize() {
  {
    auto temp = _nodes;

    _nodeCapacity = std::max(64, _nodeCapacity * 2);
    _nodes = allocateNodes(_nodeCapacity);
    if (temp != nullptr) {
      memcpy(_nodes, temp, _nodeCount * sizeof(Node));
      delete[] temp;
    }
  }

  auto n = _nodeCapacity - 1;

  for (int i = _nodeCount; i < n; ++i) {
    _nodes[i]._next = i + 1;
    _nodes[i]._height = -1;
  }
  _nodes[n]._next = Node::null;
  _nodes[n]._height = -1;
  _freeList = _nodeCount;
}

int DynamicTree::allocateNode() {
  if (_freeList == Node::null)
    resize();

  auto index = _freeList;
  auto node = _nodes + index;

  _freeList = node->_next;
  node->_parent = node->_children[0] = node->_children[1] = Node::null;
  node->_height = 0;
  ++_nodeCount;
  return index;
}

void DynamicTree::freeNode(int index) {
  _nodes[index]._next = _freeList;
  _nodes[index]._height = -1;
  _freeList = index;
  --_nodeCount;
}

int DynamicTree::add(const bounds_type &bounds, void *userData) {
  auto leaf = allocateNode();

  _nodes[leaf]._bounds = bounds;
  _nodes[leaf]._userData = userData;
  addLeafNode(leaf);
  return leaf;
}

void DynamicTree::remove(int index) {
  assert(0 <= index && index < _nodeCapacity);
  assert(_nodes[index].isLeaf());

  removeLeafNode(index);
  freeNode(index);
}

void DynamicTree::addLeafNode(int leaf) {
  if (_root == Node::null) {
    _root = leaf;
    return;
  }

  // Find the best sibling for this node
  const auto &leafBounds = _nodes[leaf]._bounds;
  auto index = _root;

  for (Node *node; !(node = _nodes + index)->isLeaf();) {
    auto c1 = node->_children[0];
    auto c2 = node->_children[1];
    auto area = boundsArea(node->_bounds);
    auto combinedArea = boundsArea(node->_bounds + leafBounds);
    // Cost of creating a new parent for this node and the new leaf
    auto cost = float(2) * combinedArea;
    // Minimum cost of pushing the leaf further down the tree
    auto inheritanceCost = float(2) * (combinedArea - area);
    float cost1, cost2;

    // Cost of descending into child 1
    if ((node = _nodes + c1)->isLeaf())
      cost1 = boundsArea(leafBounds + node->_bounds) + inheritanceCost;
    else {
      auto oldArea = boundsArea(node->_bounds);
      auto newArea = boundsArea(leafBounds + node->_bounds);

      cost1 = (newArea - oldArea) + inheritanceCost;
    }
    // Cost of descending into child 2
    if ((node = _nodes + c2)->isLeaf())
      cost2 = boundsArea(leafBounds + node->_bounds) + inheritanceCost;
    else {
      auto oldArea = boundsArea(node->_bounds);
      auto newArea = boundsArea(leafBounds + node->_bounds);

      cost2 = (newArea - oldArea) + inheritanceCost;
    }
    // Descend according to the minimum cost
    if (cost < cost1 && cost < cost2)
      break;
    index = cost1 < cost2 ? c1 : c2;
  }

  // Create a new parent
  auto oldParent = _nodes[index]._parent;
  auto newParent = allocateNode();
  auto newParentNode = _nodes + newParent;

  newParentNode->_parent = oldParent;
  newParentNode->_userData = nullptr;
  newParentNode->_bounds = leafBounds + _nodes[index]._bounds;
  newParentNode->_height = _nodes[index]._height + 1;
  if (oldParent == Node::null)
    // The sibling was the root
    _root = newParent;
  else {
    auto oldParentNode = _nodes + oldParent;

    // The sibling was not the root
    if (oldParentNode->_children[0] == index)
      oldParentNode->_children[0] = newParent;
    else
      oldParentNode->_children[1] = newParent;
  }
  newParentNode->_children[0] = index;
  newParentNode->_children[1] = leaf;
  _nodes[index]._parent = _nodes[leaf]._parent = newParent;
  // Walk back up the tree fixing heights and bounds
  index = _nodes[leaf]._parent;
  while (index != Node::null) {
    auto node = _nodes + balance(index);
    auto c1 = node->_children[0];
    auto c2 = node->_children[1];

    assert(c1 != Node::null);
    assert(c2 != Node::null);
    node->_bounds = _nodes[c1]._bounds + _nodes[c2]._bounds;
    node->_height = 1 + std::max(_nodes[c1]._height, _nodes[c2]._height);
    index = node->_parent;
  }
}

void DynamicTree::removeLeafNode(int leaf) {
  if (_root == leaf) {
    _root = Node::null;
    return;
  }

  auto parent = _nodes[leaf]._parent;
  auto grandParent = _nodes[parent]._parent;
  int sibling;

  if (_nodes[parent]._children[0] == leaf)
    sibling = _nodes[parent]._children[1];
  else
    sibling = _nodes[parent]._children[0];
  if (grandParent != Node::null) {
    // Destroy parent and connect sibling to grand parent
    if (_nodes[grandParent]._children[0] == parent)
      _nodes[grandParent]._children[0] = sibling;
    else
      _nodes[grandParent]._children[1] = sibling;
    _nodes[sibling]._parent = grandParent;
    freeNode(parent);

    // Adjust ancestor bounds
    auto index = grandParent;

    while (index != Node::null) {
      auto node = _nodes + balance(index);

      auto c1 = node->_children[0];
      auto c2 = node->_children[1];

      node->_bounds = _nodes[c1]._bounds + _nodes[c2]._bounds;
      node->_height = 1 + std::max(_nodes[c1]._height, _nodes[c2]._height);
      index = node->_parent;
    }
  } else {
    _root = sibling;
    _nodes[sibling]._parent = Node::null;
    freeNode(parent);
  }
}

// Perform a left or right rotation if node A is imbalanced.
// Return the new root index.
int DynamicTree::balance(int iA) {
  assert(iA != Node::null);

  auto a = _nodes + iA;

  if (a->isLeaf() || a->_height < 2)
    return iA;

  auto iB = a->_children[0];
  auto iC = a->_children[1];

  assert(0 <= iB && iB < _nodeCapacity);
  assert(0 <= iC && iC < _nodeCapacity);

  auto b = _nodes + iB;
  auto c = _nodes + iC;
  int balance = c->_height - b->_height;

  // Rotate C up
  if (balance > 1) {
    auto iF = c->_children[0];
    auto iG = c->_children[1];

    assert(0 <= iF && iF < _nodeCapacity);
    assert(0 <= iG && iG < _nodeCapacity);

    auto f = _nodes + iF;
    auto g = _nodes + iG;

    // Swap A and C
    c->_children[0] = iA;
    c->_parent = a->_parent;
    a->_parent = iC;
    // A's old parent should point to C
    if (c->_parent != Node::null) {
      if (_nodes[c->_parent]._children[0] == iA)
        _nodes[c->_parent]._children[0] = iC;
      else {
        assert(_nodes[c->_parent]._children[1] == iA);
        _nodes[c->_parent]._children[1] = iC;
      }
    } else
      _root = iC;
    // Rotate
    if (f->_height > g->_height) {
      c->_children[1] = iF;
      a->_children[1] = iG;
      g->_parent = iA;
      a->_bounds = b->_bounds + g->_bounds;
      c->_bounds = a->_bounds + f->_bounds;
      a->_height = 1 + std::max(b->_height, g->_height);
      c->_height = 1 + std::max(a->_height, f->_height);
    } else {
      c->_children[1] = iG;
      a->_children[1] = iF;
      f->_parent = iA;
      a->_bounds = b->_bounds + f->_bounds;
      c->_bounds = a->_bounds + g->_bounds;
      a->_height = 1 + std::max(b->_height, f->_height);
      c->_height = 1 + std::max(a->_height, g->_height);
    }
    return iC;
  }
  // Rotate B up
  if (balance < -1) {
    auto iD = b->_children[0];
    auto iE = b->_children[1];

    assert(0 <= iD && iD < _nodeCapacity);
    assert(0 <= iE && iE < _nodeCapacity);

    auto d = _nodes + iD;
    auto e = _nodes + iE;

    // Swap A and B
    b->_children[0] = iA;
    b->_parent = a->_parent;
    a->_parent = iB;
    // A's old parent should point to B
    if (b->_parent != Node::null) {
      if (_nodes[b->_parent]._children[0] == iA)
        _nodes[b->_parent]._children[0] = iB;
      else {
        assert(_nodes[b->_parent]._children[1] == iA);
        _nodes[b->_parent]._children[1] = iB;
      }
    } else
      _root = iB;
    // Rotate
    if (d->_height > e->_height) {
      b->_children[1] = iD;
      a->_children[0] = iE;
      e->_parent = iA;
      a->_bounds = c->_bounds + e->_bounds;
      b->_bounds = a->_bounds + d->_bounds;
      a->_height = 1 + std::max(c->_height, e->_height);
      b->_height = 1 + std::max(a->_height, d->_height);
    } else {
      b->_children[1] = iE;
      a->_children[0] = iD;
      d->_parent = iA;
      a->_bounds = c->_bounds + d->_bounds;
      b->_bounds = a->_bounds + e->_bounds;
      a->_height = 1 + std::max(c->_height, d->_height);
      b->_height = 1 + std::max(a->_height, e->_height);
    }
    return iB;
  }
  return iA;
}

template <typename T>
void DynamicTree::query(T *handler, const bounds_type &bounds) const {
  std::stack<int> stack;

  stack.push(_root);
  while (!stack.empty()) {
    auto nodeIndex = stack.top();
    auto node = _nodes + nodeIndex;

    stack.pop();
    if (!testOverlap(node->_bounds, bounds))
      continue;
    if (!node->isLeaf()) {
      stack.push(node->_children[0]);
      stack.push(node->_children[1]);
    } else if (!handler->queryCallback(nodeIndex))
      return;
  }
}

} // end namespace cg

#endif // __DynamicTree_h
