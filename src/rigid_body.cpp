#include "rigid_body.hpp"

void RigidBody::initializeRigidBody(float mass) {
  _inverseMass =
      mass == std::numeric_limits<float>::infinity() ? 0.0f : 1.0f / mass;
}

void RigidBody::collide(RigidBody &other, vec3 p, vec3 n) {
  //
}