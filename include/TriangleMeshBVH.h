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
// OVERVIEW: TriangleMeshBVH.h
// ========
// Class definition for triangle mesh BVH.
//
// Author: Paulo Pagliosa
// Last revision: 21/01/2022

#ifndef __TriangleMeshBVH_h
#define __TriangleMeshBVH_h

#include <memory>

#include "BVH.h"
#include "actor.hpp"

namespace cg { // begin namespace cg

/////////////////////////////////////////////////////////////////////
//
// TriangleMeshBVH: triangle mesh BVH class
// ===============
class TriangleMeshBVH final : public BVH<Triangle<vec3>> {
public:
  TriangleMeshBVH(Actor *actor, PrimitiveArray &&, uint32_t = 64);

  const cg::Reference<TriangleMesh> mesh() const { return _actor->mesh; }

  cg::Reference<TriangleMesh> mesh() { return _actor->mesh; }

  auto actor() { return _actor; }

  auto root() const { return _root; }

  auto getNode(void *ptr) const { return (Node *)ptr; }

private:
  Actor *_actor;

}; // TriangleMeshBVH

} // end namespace cg

#endif // __TriangleMeshBVH_h
