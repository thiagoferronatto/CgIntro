#ifndef DBVT_BROADPHASE_HPP
#define DBVT_BROADPHASE_HPP

#include "bvt_collision.hpp"
#include "colliders.hpp"
#include "scene.hpp"

#include "DynamicTree.h"

struct DbvtBroadphase {
  DbvtBroadphase(Scene &scene) : scene{scene}, indices(scene.actors().size()) {
    // populating the DBVT
    // TODO: create a TriangleMeshBVH for each actor and use that instead
    int i{};
    for (auto &actor : scene.actors())
      indices[i++] = tree.add(actor->bounds(), actor.get());
  }

  void collide() {
    collideTT(tree, tree, TriangleMeshCollider{nullptr, nullptr});
    // collideTT(tree, tree, TriangleMeshBvhCollider{});
    int i{};
    constexpr vec3 gravity{0, -0.00001, 0};
    for (auto &actor : scene.actors()) {
      if (actor->_inverseMass > 0)
        actor->_velocity += gravity;
      actor->translate(actor->_velocity);
      actor->rotate(actor->_angularVelocity);
      // TODO: not refit every frame
      tree.remove(indices[i]);
      indices[i++] = tree.add(actor->bounds(), actor.get());
    }
  }

  Scene &scene;
  cg::DynamicTree tree;
  std::vector<int> indices;
};

#endif // DBVT_BROADPHASE_HPP