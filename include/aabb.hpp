#ifndef AABB_HPP
#define AABB_HPP

#include "glm/vec3.hpp"
#include "triangle_mesh.hpp"

class AxisAlignedBox {
public:
  TriangleMesh getMesh() {
    TriangleMesh mesh;

    glm::vec3 v0{min.x, min.y, min.z};
    glm::vec3 v1{min.x, min.y, max.z};
    glm::vec3 v2{max.x, min.y, max.z};
    glm::vec3 v3{max.x, min.y, min.z};
    glm::vec3 v4{min.x, max.y, min.z};
    glm::vec3 v5{min.x, max.y, max.z};
    glm::vec3 v6{max.x, max.y, max.z};
    glm::vec3 v7{max.x, max.y, min.z};

    mesh.vertices = {
        v0, v1, v2, v3, // Bottom
        v4, v5, v6, v7, // Top
        v1, v2, v6, v5, // Front
        v0, v4, v7, v3, // Back
        v0, v1, v5, v4, // Left
        v2, v3, v7, v6  // Right
    };

    glm::vec3 nBottom = {0, -1, 0};
    glm::vec3 nTop = {0, 1, 0};
    glm::vec3 nFront = {0, 0, 1};
    glm::vec3 nBack = {0, 0, -1};
    glm::vec3 nLeft = {-1, 0, 0};
    glm::vec3 nRight = {1, 0, 0};

    mesh.normals = {
        nBottom, nBottom, nBottom, nBottom, // Bottom
        nTop,    nTop,    nTop,    nTop,    // Top
        nFront,  nFront,  nFront,  nFront,  // Front
        nBack,   nBack,   nBack,   nBack,   // Back
        nLeft,   nLeft,   nLeft,   nLeft,   // Left
        nRight,  nRight,  nRight,  nRight   // Right
    };

    mesh.triangles = {
        {0, 1, 2},    {2, 3, 0},    // Bottom
        {4, 5, 6},    {6, 7, 4},    // Top
        {8, 9, 10},   {8, 10, 11},  // Front
        {14, 13, 12}, {15, 14, 12}, // Back
        {16, 17, 18}, {16, 18, 19}, // Left
        {20, 21, 22}, {20, 22, 23}  // Right
    };

    return mesh;
  }

  bool overlapsWith(const AxisAlignedBox &other) const {
    return (min.x <= other.max.x && max.x >= other.min.x) &&
           (min.y <= other.max.y && max.y >= other.min.y) &&
           (min.z <= other.max.z && max.z >= other.min.z);
  }

  glm::vec3 min;
  glm::vec3 max;
};

using AAB = AxisAlignedBox;

#endif // AABB_HPP