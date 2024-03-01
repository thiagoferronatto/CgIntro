#ifndef COLLISION_PROPS_HPP
#define COLLISION_PROPS_HPP

#include "glm.hpp"

template <typename T> struct CollisionProps {
  glm::vec3 p, n;
  T &body1, &body2;
};

#endif // COLLISION_PROPS_HPP