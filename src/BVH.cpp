//[]---------------------------------------------------------------[]
//|                                                                 |
//| Copyright (C) 2019, 2023 Paulo Pagliosa.                        |
//|                                                                 |
//| This software is provided 'as-is', without any express or       |
//| implied warranty. In no event will the authors be held liable   |
//| for any damages arising from the use of this software.          |
//|                                                                 |
//| Permission is granted to anyone to use this software for any    |
//| purpose, including commercial applications, and to alter it and |
//| redistribute it freely, subject to the following restrictions:  |
//|                                                                 |
//| 1. The origin of this software must not be misrepresented; you  |
//| must not claim that you wrote the original software. If you use |
//| this software in a product, an acknowledgment in the product    |
//| documentation would be appreciated but is not required.         |
//|                                                                 |
//| 2. Altered source versions must be plainly marked as such, and  |
//| must not be misrepresented as being the original software.      |
//|                                                                 |
//| 3. This notice may not be removed or altered from any source    |
//| distribution.                                                   |
//|                                                                 |
//[]---------------------------------------------------------------[]
//
// OVERVIEW: BVH.cpp
// ========
// Source file for BVH.
//
// Author: Paulo Pagliosa
// Last revision: 22/06/2023

#include "BVH.h"
#include <algorithm>
#include <stack>

namespace cg { // begin namespace cg

/////////////////////////////////////////////////////////////////////
//
// BVHBase implementation
// =======

void BVHBase::Node::iterate(const Node *node, BVHNodeFunction f) {
  if (node == nullptr)
    return;

  auto isLeaf = node->isLeaf();

  f({node->bounds, isLeaf, node->first, node->count});
  if (!node->isLeaf()) {
    iterate(node->children[0], f);
    iterate(node->children[1], f);
  }
}

inline BVHBase::Node *BVHBase::makeLeaf(PrimitiveInfoArray &primitiveInfo,
                                        uint32_t start, uint32_t end,
                                        IndexArray &orderedPrimitiveIds) {
  Bounds3f bounds;
  auto first = uint32_t(orderedPrimitiveIds.size());

  for (uint32_t i = start; i < end; ++i) {
    bounds.inflate(primitiveInfo[i].bounds);
    orderedPrimitiveIds.push_back(_primitiveIds[primitiveInfo[i].index]);
  }
  return new Node{bounds, first, end - start};
}

inline auto maxDim(const Bounds3f &b) {
  auto s = b.size();
  return s.x > s.y && s.x > s.z ? 0 : (s.y > s.z ? 1 : 2);
}

BVHBase::Node *BVHBase::makeNode(PrimitiveInfoArray &primitiveInfo,
                                 uint32_t start, uint32_t end,
                                 IndexArray &orderedPrimitiveIds) {
  ++_nodeCount;
  if (end - start <= _maxPrimitivesPerNode)
    return makeLeaf(primitiveInfo, start, end, orderedPrimitiveIds);

  Bounds3f centroidBounds;

  for (auto i = start; i < end; ++i)
    centroidBounds.inflate(primitiveInfo[i].centroid);

  auto dim = maxDim(centroidBounds);

  if (centroidBounds.b[dim] == centroidBounds.a[dim])
    return makeLeaf(primitiveInfo, start, end, orderedPrimitiveIds);

  // Partition primitives into two sets and build children
  auto mid = (start + end) / 2;

  std::nth_element(&primitiveInfo[start], &primitiveInfo[mid],
                   &primitiveInfo[end - 1] + 1,
                   [dim](const PrimitiveInfo &a, const PrimitiveInfo &b) {
                     return a.centroid[dim] < b.centroid[dim];
                   });
  return new Node{makeNode(primitiveInfo, start, mid, orderedPrimitiveIds),
                  makeNode(primitiveInfo, mid, end, orderedPrimitiveIds)};
}

BVHBase::~BVHBase() { delete _root; }

Bounds3f BVHBase::bounds() const {
  return _root == nullptr ? Bounds3f{} : _root->bounds;
}

void BVHBase::iterate(BVHNodeFunction f) const { Node::iterate(_root, f); }

} // end namespace cg
