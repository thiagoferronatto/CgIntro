#include "rigid_body.hpp"

// if mass == 0, then assume infinite mass
void RigidBody::initializeRigidBody(float mass) {
  _inverseMass = mass == 0.0f ? 0.0f : 1.0f / mass;
}

void RigidBody::collide(RigidBody &other, vec3 p, vec3 n) {
  //
}