#ifndef SIMULATION_STEP_HPP
#define SIMULATION_STEP_HPP

#include "collision_props.hpp"

template <typename T>
void simulatePhysicsStep(const CollisionProps<T> &props, float dt) {
  using namespace glm;

  constexpr float restitution{0.9f};
  auto &p{props.p}, &n{props.n};
  auto &body1{props.body1}, &body2{props.body2};
  auto va{body1._velocity}, vb{body2._velocity};
  auto vab{va - vb};
  auto invma{body1._inverseMass}, invmb{body2._inverseMass};
  auto ra{body1._centerOfMass - p}, rb{body2._centerOfMass - p};
  auto j{
      dot(-(1 + restitution) * vab, n) /
      (dot(n, n * (invma + invmb)) + dot(cross((invma * cross(ra, n)), ra) +
                                             cross((invmb * cross(rb, n)), rb),
                                         n))};
  body1._velocity += invma * j * n * dt;
  body2._velocity -= invmb * j * n * dt;
  body1._angularVelocity += invma * cross(ra, j * n) * dt;
  body2._angularVelocity += invmb * cross(rb, j * n) * dt;

  // temporary fix for continued overlap
  if (invma != 0)
    body1.translate(0.0001f * n);
  if (invmb != 0)
    body2.translate(-0.0001f * n);
}

#endif // SIMULATION_STEP_HPP