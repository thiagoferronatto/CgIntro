#ifndef DBVT_BROADPHASE_HPP
#define DBVT_BROADPHASE_HPP

#include "bvt_collision.hpp"
#include "colliders.hpp"
#include "scene.hpp"

#include "DynamicTree.h"

struct DbvtBroadphase {
  using Bvt = cg::TriangleMeshBVH;
  using Triangles = Bvt::PrimitiveArray;

  DbvtBroadphase(Scene &scene) : scene{scene}, indices(scene.actors().size()) {
    // populating the DBVT
    int i{};
    for (auto &actor : scene.actors()) {
      auto &verts{actor->mesh->vertices()};
      Triangles triangles;
      for (auto &idxt : actor->mesh->triangles())
        triangles.push_back({verts[idxt.v1], verts[idxt.v2], verts[idxt.v3]});
      auto bvt{new Bvt{actor, std::move(triangles)}}; // FIXME: memory leak
      indices[i++] = tree.add(bvt->bounds(), bvt);
    }
  }

  void collide() {
    collideTT(tree, tree, BvtCollider{scene.dt()});
    int i{};
    // TODO: make broadphase scene-independent
    for (auto &actor : scene.actors()) {
      // TODO: find a way to move this physics stuff into simulatePhysicsStep()
      static constexpr vec3 gravity{0, -0.0005, 0};
      if (actor->_inverseMass > 0)
        actor->_velocity += gravity * scene.dt();
      actor->translate(actor->_velocity);
      actor->rotate(actor->_angularVelocity);

      // TODO: not refit every frame
      tree.remove(indices[i]);

      // FIXME: this does not work, actors are no longer the leaves, BVTs are
      // use tree bounds instead (which means the tree needs its own transform?)
      indices[i++] = tree.add(actor->bounds(), actor);
    }
  }

  Scene &scene;
  cg::DynamicTree tree;
  std::vector<int> indices;
};

#endif // DBVT_BROADPHASE_HPP