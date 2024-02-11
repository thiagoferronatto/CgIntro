#ifndef AABB_HPP
#define AABB_HPP

#include "glm/vec3.hpp"

struct AxisAlignedBoundingBox {
  glm::vec3 a{std::numeric_limits<float>::max()},
      b{std::numeric_limits<float>::lowest()};
};

using AABB = AxisAlignedBoundingBox;

#endif // AABB_HPP