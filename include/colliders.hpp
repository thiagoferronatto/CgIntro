#ifndef COLLIDERS_HPP
#define COLLIDERS_HPP

#include "actor.hpp"
#include "simulation_step.hpp"
#include "triangle_intersection.hpp"

#include "TriangleMeshBVH.h"

struct MeshCollider {
  using Bvt = cg::TriangleMeshBVH;
  using Triangles = Bvt::PrimitiveArray;

  MeshCollider(Actor *actor1, Actor *actor2, Triangles &mesh1trigs,
               Triangles &mesh2trigs, float dt)
      : actor1{actor1}, actor2{actor2}, mesh1trigs{mesh1trigs},
        mesh2trigs{mesh2trigs}, dt{dt} {}

  // narrow phase
  void process(int first1, int count1, int first2, int count2) {
    auto &trs1 = actor1->transform(), &trs2 = actor2->transform();
    auto begin1{mesh1trigs.data() + first1}, begin2{mesh2trigs.data() + first2};
    auto end1{begin1 + count1}, end2{begin2 + count2};
    for (auto p1{begin1}; p1 < end1; ++p1) {
      vec3 t1v1{trs1 * vec4{begin1->v1, 1}}, t1v2{trs1 * vec4{begin1->v2, 1}},
          t1v3{trs1 * vec4{begin1->v3, 1}};
      for (auto p2{begin2}; p2 < end2; ++p2) {
        vec3 t2v1{trs2 * vec4{begin2->v1, 1}}, t2v2{trs2 * vec4{begin2->v2, 1}},
            t2v3{trs2 * vec4{begin2->v3, 1}};
        vec3 a,       // start point of intersecting segment
            b,        // end point of intersecting segment
            n1,       // normal of the first triangle of the collision
            n2;       // normal of the second triangle of the collision
        int coplanar; // 1 if triangles were coplanar, 0 otherwise
        int overlap{devillers::tri_tri_intersection_test_3d(
            (float *)&t1v1, (float *)&t1v2, (float *)&t1v3, (float *)&t2v1,
            (float *)&t2v2, (float *)&t2v3, &coplanar, (float *)&a, (float *)&b,
            (float *)&n1, (float *)&n2)};
        if (overlap) {
          // collision detected

          if (coplanar) {
            a = actor1->_centerOfMass;
            b = actor2->_centerOfMass;
          }
          auto p{0.5f * (a + b)};
          auto n{normalize(n2)};

          CollisionProps<Actor> props{p, n, *actor1, *actor2};
          simulatePhysicsStep(props, dt);
        }
      }
    }
  }

  Actor *actor1, *actor2;
  Triangles &mesh1trigs, &mesh2trigs;
  float dt;
};

struct BvtCollider {
  using Bvt = cg::TriangleMeshBVH;

  // callback for when two DBVT leaves (which are BVTs) collide
  void process(Bvt *bvt1, Bvt *bvt2) {
    collideTT(*bvt1, *bvt2,
              MeshCollider{bvt1->actor(), bvt2->actor(), bvt1->primitives(),
                           bvt2->primitives(), dt});
  }

  float dt;
};

#endif // COLLIDERS_HPP