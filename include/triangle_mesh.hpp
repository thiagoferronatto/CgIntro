#ifndef TRIANGLE_MESH_HPP
#define TRIANGLE_MESH_HPP

#include <memory>
#include <string>
#include <vector>

#include "glm.hpp"
#include "SharedObject.h"

using namespace glm;

/// @brief General triangle structure. Holds arbitrary vertex data.
/// @tparam Vertex data type.
template <typename T> struct Triangle {
  T v1, v2, v3;
};

using IndexedTriangle = Triangle<unsigned>;

struct TriangleMeshData {
  static TriangleMeshData fromObj(const std::string &fileName);
  static TriangleMeshData cube();
  static TriangleMeshData plane();

  void addVertex(vec3 v);
  void addNormal(vec3 n);
  void addTriangle(IndexedTriangle t);
  void addUV(vec2 uv);

  std::vector<vec3> vertices, normals;
  std::vector<IndexedTriangle> triangles;
  std::vector<vec2> uvs{};
};

class TriangleMesh : public cg::SharedObject {
public:
  TriangleMesh(TriangleMeshData &&data);
  TriangleMesh(const TriangleMesh &other) = delete;
  TriangleMesh(TriangleMesh &&other) noexcept;

  TriangleMesh &operator=(const TriangleMesh &other) = delete;
  TriangleMesh &operator=(TriangleMesh &&other) noexcept;

  const std::vector<vec3> &vertices() const;
  const std::vector<vec3> &normals() const;
  const std::vector<IndexedTriangle> &triangles() const;
  const std::vector<vec2> &uv() const;

  const auto &data() const { return _data; }

private:
  inline static size_t _cubes{}, _planes{}, _customMeshes{};

  TriangleMeshData _data;
};

#endif // TRIANGLE_MESH_HPP