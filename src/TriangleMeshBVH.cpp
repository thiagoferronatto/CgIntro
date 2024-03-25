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
// OVERVIEW: TriangleMeshBVH.cpp
// ========
// Source file for triangle mesh BVH.
//
// Author: Paulo Pagliosa
// Last revision: 07/02/2022

#include "TriangleMeshBVH.h"

namespace cg { // begin namespace cg

/////////////////////////////////////////////////////////////////////
//
// TriangleMeshBVH implementation
// ===============
TriangleMeshBVH::TriangleMeshBVH(Actor *actor, PrimitiveArray &&primitives,
                                 uint32_t maxt)
    : BVH{std::move(primitives), maxt}, _actor{actor} {
  const auto &m = _actor->mesh->data();
  auto nt = (uint32_t)m.triangles.size();

  assert(nt > 0);
  _primitiveIds.resize(nt);

  PrimitiveInfoArray primitiveInfo(nt);

  for (uint32_t i = 0; i < nt; ++i) {
    _primitiveIds[i] = i;

    auto t = m.triangles.data() + i;
    Bounds3f b;

    b.inflate(m.vertices[t->v1]);
    b.inflate(m.vertices[t->v2]);
    b.inflate(m.vertices[t->v3]);
    primitiveInfo[i] = {i, b};
  }
  build(primitiveInfo);
#ifdef _DEBUG
  if (true) {
    printf("Mesh bounds: (%g, %g, %g), (%g, %g, %g)\n", _actor->bounds().a.x,
           _actor->bounds().a.y, _actor->bounds().a.z, _actor->bounds().b.x,
           _actor->bounds().b.y, _actor->bounds().b.z);
    printf("Mesh triangles: %d\n", nt);
    printf("BVH bounds: (%g, %g, %g), (%g, %g, %g)\n", bounds().a.x,
           bounds().a.y, bounds().a.z, bounds().b.x, bounds().b.y,
           bounds().b.z);
    printf("BVH nodes: %zd\n", size());
    /*
    iterate([this](const BVHNodeInfo& node)
    {
      if (!node.isLeaf)
        return;
      node.bounds.print("Leaf bounds:");
      printf("Leaf triangles: %d\n", node.count);
    });
    */
    putchar('\n');
  }
#endif // _DEBUG
}

} // end namespace cg
