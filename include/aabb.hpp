#ifndef AABB_HPP
#define AABB_HPP

#include "glm/glm.hpp"

struct AxisAlignedBoundingBox {
  void inflate(const AxisAlignedBoundingBox &other) {
    a = min(a, other.a);
    b = max(b, other.b);
  }

  void inflate(glm::vec3 v) {
    a = min(a, v);
    b = max(b, v);
  }

  glm::vec3 size() const { return b - a; }

  glm::vec3 center() const { return 0.5f * (a + b); }

  glm::vec3 a{std::numeric_limits<float>::max()},
      b{std::numeric_limits<float>::lowest()};
};

using Aabb = AxisAlignedBoundingBox;

#endif // AABB_HPP