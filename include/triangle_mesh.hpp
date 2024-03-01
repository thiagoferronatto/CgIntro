#ifndef TRIANGLE_MESH_HPP
#define TRIANGLE_MESH_HPP

#include <vector>

#include "actor.hpp"

struct Triangle {
  unsigned v1, v2, v3;
};

struct TriangleMeshData {
  static std::shared_ptr<TriangleMeshData> fromObj(const std::string &fileName);
  static std::shared_ptr<TriangleMeshData> cube();
  static std::shared_ptr<TriangleMeshData> plane();

  void addVertex(vec3 v);
  void addNormal(vec3 n);
  void addTriangle(Triangle t);
  void addUV(vec2 uv);

  std::vector<vec3> vertices, normals;
  std::vector<Triangle> triangles;
  std::vector<vec2> uvs{};
};

class TriangleMesh : public Actor {
public:
  TriangleMesh(std::string name, std::shared_ptr<TriangleMeshData> data);
  TriangleMesh(const TriangleMesh &other) = delete;
  TriangleMesh(TriangleMesh &&other) noexcept;

  TriangleMesh &operator=(const TriangleMesh &other) = delete;
  TriangleMesh &operator=(TriangleMesh &&other) noexcept;

  const std::vector<vec3> &vertices() const;
  const std::vector<vec3> &normals() const;
  const std::vector<Triangle> &triangles() const;
  const std::vector<vec2> &uv() const;

  void bound() override;

  void initializeRigidBody(float mass) override;

  auto data() const { return _data; }

private:
  inline static size_t _cubes{}, _planes{}, _customMeshes{};

  std::shared_ptr<TriangleMeshData> _data;
};

#endif // TRIANGLE_MESH_HPP