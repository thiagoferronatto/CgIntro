#include "custom_assert.hpp"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "scene.hpp"
#include "triangle_intersection.hpp"
#include "window.hpp"

#include <chrono>

int main() {
  constexpr size_t w{1600}, h{900};
  Window window{w, h, "VBAG 2.0"};

  Scene scene;

  auto mazeBall = TriangleMesh::fromObj("assets/quadball.obj");
  mazeBall->material.Kd = {1, 0, 0};
  mazeBall->scale({0.5, 0.5, 0.5});
  mazeBall->translate({0, 8, 0});
  mazeBall->initializeRigidBody(1);
  scene.addActor(mazeBall);

  auto tiltedPlane1 = TriangleMesh::plane();
  tiltedPlane1->rotate({0, 0, glm::radians(30.0f)});
  tiltedPlane1->translate({-1, 6, 0});
  tiltedPlane1->initializeRigidBody(1); // temporary mass
  tiltedPlane1->_inverseMass = 0;       // setting infinite mass
  scene.addActor(tiltedPlane1);

  auto tiltedPlane2 = TriangleMesh::plane();
  tiltedPlane2->rotate({0, 0, glm::radians(-30.0f)});
  tiltedPlane2->translate({1, 4, 0});
  tiltedPlane2->initializeRigidBody(1); // temporary mass
  tiltedPlane2->_inverseMass = 0;       // setting infinite mass
  scene.addActor(tiltedPlane2);

  auto tiltedPlane3 = TriangleMesh::plane();
  tiltedPlane3->rotate({0, 0, glm::radians(30.0f)});
  tiltedPlane3->translate({-1, 2, 0});
  tiltedPlane3->initializeRigidBody(1); // temporary mass
  tiltedPlane3->_inverseMass = 0;       // setting infinite mass
  scene.addActor(tiltedPlane3);

  auto rightWallPlane = TriangleMesh::plane();
  rightWallPlane->scale({7, 1, 1});
  rightWallPlane->rotate({0, 0, glm::radians(-90.0f)});
  rightWallPlane->translate({1.9, 4, 0});
  rightWallPlane->initializeRigidBody(1); // temporary mass
  rightWallPlane->_inverseMass = 0;       // setting infinite mass
  scene.addActor(rightWallPlane);

  auto leftWallPlane = TriangleMesh::plane();
  leftWallPlane->scale({7, 1, 1});
  leftWallPlane->rotate({0, 0, glm::radians(90.0f)});
  leftWallPlane->translate({-1.9, 4, 0});
  leftWallPlane->initializeRigidBody(1); // temporary mass
  leftWallPlane->_inverseMass = 0;       // setting infinite mass
  scene.addActor(leftWallPlane);

  auto bouncingBall1 = TriangleMesh::fromObj("assets/quadball.obj");
  bouncingBall1->material.Kd = {0, 1, 0};
  bouncingBall1->scale({0.5, 0.5, 0.5});
  bouncingBall1->translate({3, 4, -0.01});
  bouncingBall1->_velocity = {0.01, 0.005, 0};
  bouncingBall1->initializeRigidBody(1);
  scene.addActor(bouncingBall1);

  auto bouncingBall2 = TriangleMesh::fromObj("assets/quadball.obj");
  bouncingBall2->material.Kd = {0, 0, 1};
  bouncingBall2->scale({0.5, 0.5, 0.5});
  bouncingBall2->translate({15, 4, 0.01});
  bouncingBall2->_velocity = {-0.01, 0.005, 0};
  bouncingBall2->initializeRigidBody(1);
  scene.addActor(bouncingBall2);

  auto ground = TriangleMesh::fromObj("assets/sandbox.obj");
  ground->scale({15, 2, 15});
  ground->translate({0, -3, 0});
  ground->initializeRigidBody(1); // temporary mass
  ground->_inverseMass = 0;       // setting infinite mass
  scene.addActor(ground);

  auto light{std::make_shared<Light>(vec3{1})};
  light->intensity = 50;
  light->translate({-10, 5, -10});
  scene.addLight(light);
  light = std::make_shared<Light>(vec3{1});
  light->intensity = 50;
  light->translate({-10, 5, 10});
  scene.addLight(light);
  light = std::make_shared<Light>(vec3{1});
  light->intensity = 50;
  light->translate({10, 5, -10});
  scene.addLight(light);
  light = std::make_shared<Light>(vec3{1});
  light->intensity = 50;
  light->translate({10, 5, 10});
  scene.addLight(light);

  auto cam{std::make_shared<Camera>("cam_1", glm::radians(74.0f),
                                    float(w) / float(h), 0.1f, 1000.0f)};
  cam->translate({0, 10, 20});
  cam->rotate({glm::radians(45.0f), 0, 0});
  scene.addCamera(cam);

  scene.ambient = {};

  constexpr vec3 gravity{0, -0.000025, 0};

  float deltaTime = 0;

  scene.render(window, [&] {
    using namespace std::chrono;

    using ActorPtr = std::shared_ptr<Actor>;
    using ActorPair = std::pair<ActorPtr, ActorPtr>;

    auto before = std::chrono::steady_clock::now();

    // COLLISION DETECTION
    std::vector<ActorPair> donePairs;
    for (auto &actor1 : scene.actors()) {
      if (!actor1->isBound())
        actor1->bound();
      for (auto &actor2 : scene.actors()) {
        if (actor2 == actor1)
          continue;
        // this seems excessive, try using a hash table with XORed ptrs as keys,
        // that way it'd have one entry per pair since A ^ B == B ^ A
        if (std::find(donePairs.begin(), donePairs.end(),
                      ActorPair{actor1, actor2}) != donePairs.end())
          continue;
        donePairs.push_back({actor1, actor2});
        donePairs.push_back({actor2, actor1});
        if (!actor2->isBound())
          actor2->bound();
        // broad check
        if (!actor1->boundsIntersect(*actor2))
          continue;
        // narrow check
        auto &trs1 = actor1->transform(), trs2 = actor2->transform();
        auto mesh1 = std::dynamic_pointer_cast<TriangleMesh>(actor1),
             mesh2 = std::dynamic_pointer_cast<TriangleMesh>(actor2);
        if (!mesh1 || !mesh2) {
          // this doesn't happen, but if more actor types are added, then it may
          std::terminate();
        }
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
                (float *)&t2v2, (float *)&t2v3, &coplanar, (float *)&a,
                (float *)&b, (float *)&n1, (float *)&n2);
            if (overlap) {
              // COLLISION RESPONSE

              if (coplanar) {
                // TODO: figure out if EPSILON CHECKS need to be on in
                // triangle_intersection.cpp for coplanarity to be detected

                // doing the best I can for the rare coplanar case
                // (can happen often with regular polyhedra, and
                // this fix works perfectly for specifically that)
                a = mesh1->_centerOfMass;
                b = mesh2->_centerOfMass;
              }
              auto p = 0.5f * (a + b); // point of collision

              constexpr float restitution{.9825};

              // TODO: currently using the normal of the second mesh, but
              //       should figure out which one (n1 or n2) to use based on
              //       some reasonable criteria (maybe largest mass?)
              n2 = normalize(n2);
              auto vr = mesh1->_velocity - mesh2->_velocity;
              auto vj = -(1 + restitution) * dot(vr, n2);
              auto j = vj / (mesh1->_inverseMass + mesh2->_inverseMass);

              // updating velocities
              mesh1->_velocity += mesh1->_inverseMass * j * n2;
              mesh2->_velocity -= mesh2->_inverseMass * j * n2;

              auto invI1 = 1.0f / mesh1->_inertiaTensor;
              auto invI2 = 1.0f / mesh2->_inertiaTensor;

              // TODO: fix angular motion

              // mesh1->_angularVelocity +=
              //     0.1f * mesh1->_inverseMass *
              //     cross(p - mesh1->_centerOfMass, j * n2);
              // mesh2->_angularVelocity -=
              //     0.1f * mesh2->_inverseMass *
              //     cross(p - mesh2->_centerOfMass, j * n2);

              // temporary fix for continued overlap
              mesh1->translate(0.001f * mesh1->_inverseMass * n2);
              mesh2->translate(-0.001f * mesh2->_inverseMass * n2);

              // an attempt to add drag. this should reduce the velocity in the
              // direction orthogonal to the collision normal
              constexpr float dragCoefficient = 0.01f;
              mesh1->_velocity -=
                  dragCoefficient *
                  (mesh1->_velocity - dot(mesh1->_velocity, n2) * n2);

              // TODO: figure out how to add damping, the balls keep bouncing (I
              // believe the restitution coefficient should already fix this)
            }
          }
        }
      }
    }

    for (auto &actor : scene.actors()) {
      // TODO: maybe limit top speed to avoid tunneling
      //
      // constexpr auto maxSpd = 0.025f;
      // auto currSpd = length(actor->_velocity);
      // if (currSpd > maxSpd)
      //   actor->_velocity *= maxSpd / currSpd;
      actor->translate(actor->_velocity);
      actor->rotate(actor->_angularVelocity);
    }

    // maybe add a flag to objects that are not affected by gravity (like the
    // ground plane or immovable structures)
    mazeBall->_velocity += gravity;
    bouncingBall1->_velocity += gravity;
    bouncingBall2->_velocity += gravity;

    auto now = steady_clock::now();
    deltaTime = duration_cast<microseconds>(now - before).count() / 1e6f;
  });

  return 0;
}