#ifndef TRIANGLE_MESH_HPP
#define TRIANGLE_MESH_HPP

#include <vector>

#include "actor.hpp"

class TriangleMesh : public Actor {
public:
  struct Triangle {
    unsigned v1, v2, v3;
  };

  static std::shared_ptr<TriangleMesh> fromObj(const std::string &fileName);

  TriangleMesh(std::string name);
  TriangleMesh(const TriangleMesh &other) = delete;
  TriangleMesh(TriangleMesh &&other) noexcept;

  TriangleMesh &operator=(const TriangleMesh &other) = delete;
  TriangleMesh &operator=(TriangleMesh &&other) noexcept;

  void addVertex(vec3 v);
  void addNormal(vec3 n);
  void addTriangle(Triangle t);
  void addUV(vec2 uv);

  const std::vector<vec3> &vertices() const;
  const std::vector<vec3> &normals() const;
  const std::vector<Triangle> &triangles() const;
  const std::vector<vec2> &uv() const;

  void translate(vec3 xyz) override;
  void rotate(vec3 euler) override;
  void scale(vec3 xyz) override;

  void bound() override;

  void initializeRigidBody(float mass) override;

  static std::shared_ptr<TriangleMesh> cube();
  static std::shared_ptr<TriangleMesh> plane();

private:
  inline static size_t _cubes{}, _planes{}, _customMeshes{};

  std::vector<vec3> _vertices, _normals;
  std::vector<Triangle> _triangles;
  std::vector<vec2> _uv{};
};

#endif // TRIANGLE_MESH_HPP