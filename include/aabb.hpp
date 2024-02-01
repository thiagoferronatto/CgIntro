#ifndef AABB_HPP
#define AABB_HPP

#include "glm/vec3.hpp"

struct AxisAlignedBoundingBox {
  glm::vec3 a{}, b{};
};

using AABB = AxisAlignedBoundingBox;

#endif // AABB_HPP