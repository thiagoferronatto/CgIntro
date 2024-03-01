#ifndef COLLIDERS_HPP
#define COLLIDERS_HPP

#include "simulation_step.hpp"
#include "triangle_intersection.hpp"
#include "triangle_mesh.hpp"

#include "TriangleMeshBVH.h"

struct TriangleMeshCollider {
  TriangleMeshCollider(TriangleMesh *mesh1, TriangleMesh *mesh2)
      : mesh1{mesh1}, mesh2{mesh2} {}

  // narrow phase
  void process(void *ptr1, void *ptr2) {
    auto actor1{(Actor *)ptr1}, actor2{(Actor *)ptr2};
    auto mesh1{dynamic_cast<TriangleMesh *>(actor1)},
        mesh2{dynamic_cast<TriangleMesh *>(actor2)};
    auto &trs1 = actor1->transform(), trs2 = actor2->transform();
    for (auto &t1 : mesh1->triangles()) {
      vec3 t1v1 = trs1 * vec4{mesh1->vertices()[t1.v1], 1},
           t1v2 = trs1 * vec4{mesh1->vertices()[t1.v2], 1},
           t1v3 = trs1 * vec4{mesh1->vertices()[t1.v3], 1};
      for (auto &t2 : mesh2->triangles()) {
        vec3 t2v1 = trs2 * vec4{mesh2->vertices()[t2.v1], 1},
             t2v2 = trs2 * vec4{mesh2->vertices()[t2.v2], 1},
             t2v3 = trs2 * vec4{mesh2->vertices()[t2.v3], 1};
        vec3 a,       // start point of intersecting segment
            b,        // end point of intersecting segment
            n1,       // normal of the first triangle of the collision
            n2;       // normal of the second triangle of the collision
        int coplanar; // 1 if triangles were coplanar, 0 otherwise
        bool overlap = devillers::tri_tri_intersection_test_3d(
            (float *)&t1v1, (float *)&t1v2, (float *)&t1v3, (float *)&t2v1,
            (float *)&t2v2, (float *)&t2v3, &coplanar, (float *)&a, (float *)&b,
            (float *)&n1, (float *)&n2);
        if (overlap) {
          // collision detected

          if (coplanar) {
            a = mesh1->_centerOfMass;
            b = mesh2->_centerOfMass;
          }
          auto p = 0.5f * (a + b);
          auto n{normalize(n2)};

          CollisionProps<TriangleMesh> props{p, n, *mesh1, *mesh2};
          simulateStep(props);
        }
      }
    }
  }

  TriangleMesh *mesh1, *mesh2;
};

// struct TriangleMeshBvhCollider {
//   TriangleMeshBvhCollider() {}
//
//   // called when two DBVT leaves (which are BVTs) collide
//   void process(void *ptr1, void *ptr2) {
//     using Bvt = cg::TriangleMeshBVH;
//     auto bvt1{(Bvt *)ptr1}, bvt2{(Bvt *)ptr2};
//     collideTT(*bvt1, *bvt2, TriangleMeshCollider{bvt1->mesh(),
//     bvt2->mesh()});
//   }
// };

#endif // COLLIDERS_HPP