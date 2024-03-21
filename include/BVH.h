//[]---------------------------------------------------------------[]
//|                                                                 |
//| Copyright (C) 2019, 2022 Paulo Pagliosa.                        |
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
// OVERVIEW: BVH.h
// ========
// Class definition for BVH.
//
// Author: Paulo Pagliosa
// Last revision: 10/02/2022

#ifndef __BVH_h
#define __BVH_h

#include "SharedObject.h"
#include "aabb.hpp"

#include <cassert>
#include <cinttypes>
#include <functional>
#include <vector>

namespace cg { // begin namespace cg

using namespace glm;
using Bounds3f = Aabb;

struct BVHNodeInfo {
  Bounds3f bounds;
  bool isLeaf;
  uint32_t first;
  uint32_t count;

}; // BVHNodeInfo

using BVHNodeFunction = std::function<void(const BVHNodeInfo &)>;

/////////////////////////////////////////////////////////////////////
//
// BVHBase: BVH base class
// =======
class BVHBase : public SharedObject {
public:
  ~BVHBase();

  auto size() const { return (size_t)_nodeCount; }

  Bounds3f bounds() const;
  void iterate(BVHNodeFunction) const;

protected:
  struct Node;
  struct PrimitiveInfo;

  using PrimitiveInfoArray = std::vector<PrimitiveInfo>;
  using IndexArray = std::vector<uint32_t>;

  IndexArray _primitiveIds;
  Node *_root{};

  BVHBase(uint32_t maxPrimitivesPerNode)
      : _maxPrimitivesPerNode{maxPrimitivesPerNode} {
    // do nothing
  }

  void build(PrimitiveInfoArray &primitiveInfo) {
    auto np = (uint32_t)primitiveInfo.size();
    IndexArray orderedPrimitiveIds(np);

    _root = makeNode(primitiveInfo, 0, np, orderedPrimitiveIds);
    _primitiveIds.swap(orderedPrimitiveIds);
  }

private:
  uint32_t _nodeCount{};
  uint32_t _maxPrimitivesPerNode;

  Node *makeNode(PrimitiveInfoArray &, uint32_t, uint32_t, IndexArray &);
  Node *makeLeaf(PrimitiveInfoArray &, uint32_t, uint32_t, IndexArray &);

}; // BVHBase

struct BVHBase::Node {
  Bounds3f bounds;
  Node *children[2];
  uint32_t first;
  uint32_t count;

  ~Node() {
    delete children[0];
    delete children[1];
  }

  Node(const Bounds3f &bounds, uint32_t first, uint32_t count)
      : bounds{bounds}, first{first}, count{count} {
    children[0] = children[1] = nullptr;
  }

  Node(Node *c0, Node *c1) : count{} {
    bounds.inflate(c0->bounds);
    bounds.inflate(c1->bounds);
    children[0] = c0;
    children[1] = c1;
  }

  bool isLeaf() const { return children[0] == nullptr; }

  static void iterate(const Node *, BVHNodeFunction);

}; // BVHBase::Node

struct BVHBase::PrimitiveInfo {
  uint32_t index;
  Bounds3f bounds;
  vec3 centroid;

  PrimitiveInfo() = default;

  PrimitiveInfo(uint32_t index, const Bounds3f &bounds)
      : index{index}, bounds{bounds}, centroid{0.5f * (bounds.a + bounds.b)} {
    // do nothing
  }

}; // BVHBase::PrimitiveInfo

/////////////////////////////////////////////////////////////////////
//
// BVH: BVH class
// ===
template <typename T> class BVH : public BVHBase { // changed from final
public:
  using PrimitiveArray = std::vector<T>; // changed from std::vector<T *>

  BVH(PrimitiveArray &&, uint32_t = 8);

  const auto &primitives() const { return _primitives; }
  auto &primitives() { return _primitives; }

protected: // changed from private
  PrimitiveArray _primitives;

}; // BVH

template <typename T>
BVH<T>::BVH(PrimitiveArray &&primitives, uint32_t maxPrimitivesPerNode)
    : BVHBase{maxPrimitivesPerNode}, _primitives{std::move(primitives)} {
  auto np = (uint32_t)_primitives.size();

  assert(np > 0);
  _primitiveIds.resize(np);

  PrimitiveInfoArray primitiveInfo(np);

  for (uint32_t i = 0; i < np; ++i) {
    Aabb bounds;
    bounds.inflate(_primitives[i].v1);
    bounds.inflate(_primitives[i].v2);
    bounds.inflate(_primitives[i].v3);
    primitiveInfo[i] = {_primitiveIds[i] = i, bounds};
  }
  build(primitiveInfo);
}

} // end namespace cg

#endif // __BVH_h
