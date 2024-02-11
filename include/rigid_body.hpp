#ifndef RIGID_BODY_HPP
#define RIGID_BODY_HPP

#include "boundable.hpp"
#include "glm/ext.hpp"
#include "glm/glm.hpp"

using namespace glm;

class RigidBody : public Boundable {
public:
  virtual void initializeRigidBody(float mass);

  virtual void collide(RigidBody &other, vec3 p, vec3 n);

public:
  // https://en.wikipedia.org/wiki/Moment_of_inertia#Inertia_tensor
  vec3 _invInertiaTensor{};
  vec3 _velocity{}, _angularVelocity{}, _centerOfMass;
  float _inverseMass;
};

#endif // RIGID_BODY_HPP