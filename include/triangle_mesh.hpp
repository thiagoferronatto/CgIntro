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

  static std::shared_ptr<TriangleMesh> cube() {
    auto mesh{std::make_shared<TriangleMesh>(std::string{"cube_"} +
                                             std::to_string(_cubes++))};
    // front face
    mesh->addVertex({-1, -1, 1}); // 0
    mesh->addNormal({0, 0, 1});   // 0
    mesh->addVertex({1, -1, 1});  // 1
    mesh->addNormal({0, 0, 1});   // 1
    mesh->addVertex({1, 1, 1});   // 2
    mesh->addNormal({0, 0, 1});   // 2
    mesh->addVertex({-1, 1, 1});  // 3
    mesh->addNormal({0, 0, 1});   // 3
    mesh->addTriangle({0, 1, 2});
    mesh->addTriangle({2, 3, 0});

    // back face
    mesh->addVertex({1, -1, -1});  // 4
    mesh->addNormal({0, 0, -1});   // 4
    mesh->addVertex({-1, -1, -1}); // 5
    mesh->addNormal({0, 0, -1});   // 5
    mesh->addVertex({-1, 1, -1});  // 6
    mesh->addNormal({0, 0, -1});   // 6
    mesh->addVertex({1, 1, -1});   // 7
    mesh->addNormal({0, 0, -1});   // 7
    mesh->addTriangle({4, 5, 6});
    mesh->addTriangle({6, 7, 4});

    // left face
    mesh->addVertex({-1, -1, -1}); // 8
    mesh->addNormal({-1, 0, 0});   // 8
    mesh->addVertex({-1, -1, 1});  // 9
    mesh->addNormal({-1, 0, 0});   // 9
    mesh->addVertex({-1, 1, 1});   // 10
    mesh->addNormal({-1, 0, 0});   // 10
    mesh->addVertex({-1, 1, -1});  // 11
    mesh->addNormal({-1, 0, 0});   // 11
    mesh->addTriangle({8, 9, 10});
    mesh->addTriangle({10, 11, 8});

    // right face
    mesh->addVertex({1, -1, 1});  // 12
    mesh->addNormal({1, 0, 0});   // 12
    mesh->addVertex({1, -1, -1}); // 13
    mesh->addNormal({1, 0, 0});   // 13
    mesh->addVertex({1, 1, -1});  // 14
    mesh->addNormal({1, 0, 0});   // 14
    mesh->addVertex({1, 1, 1});   // 15
    mesh->addNormal({1, 0, 0});   // 15
    mesh->addTriangle({12, 13, 14});
    mesh->addTriangle({14, 15, 12});

    // bottom face
    mesh->addVertex({-1, -1, -1}); // 16
    mesh->addNormal({0, -1, 0});   // 16
    mesh->addVertex({1, -1, -1});  // 17
    mesh->addNormal({0, -1, 0});   // 17
    mesh->addVertex({1, -1, 1});   // 18
    mesh->addNormal({0, -1, 0});   // 18
    mesh->addVertex({-1, -1, 1});  // 19
    mesh->addNormal({0, -1, 0});   // 19
    mesh->addTriangle({16, 17, 18});
    mesh->addTriangle({18, 19, 16});

    // top face
    mesh->addVertex({-1, 1, 1});  // 20
    mesh->addNormal({0, 1, 0});   // 20
    mesh->addVertex({1, 1, 1});   // 21
    mesh->addNormal({0, 1, 0});   // 21
    mesh->addVertex({1, 1, -1});  // 22
    mesh->addNormal({0, 1, 0});   // 22
    mesh->addVertex({-1, 1, -1}); // 23
    mesh->addNormal({0, 1, 0});   // 23
    mesh->addTriangle({20, 21, 22});
    mesh->addTriangle({22, 23, 20});

    return mesh;
  }

  static std::shared_ptr<TriangleMesh> plane() {
    auto mesh{std::make_shared<TriangleMesh>(std::string{"plane_"} +
                                             std::to_string(_planes++))};

    mesh->addVertex({-1, 0, 1});
    mesh->addVertex({1, 0, 1});
    mesh->addVertex({1, 0, -1});
    mesh->addVertex({-1, 0, -1});

    mesh->addNormal({0, 1, 0});
    mesh->addNormal({0, 1, 0});
    mesh->addNormal({0, 1, 0});
    mesh->addNormal({0, 1, 0});

     mesh->addUV({0, 0});
     mesh->addUV({1, 0});
     mesh->addUV({1, 1});
     mesh->addUV({0, 1});

    mesh->addTriangle({0, 1, 2});
    mesh->addTriangle({2, 3, 0});

    return mesh;
  }

private:
  inline static size_t _cubes{}, _planes{};

  std::vector<vec3> _vertices, _normals;
  std::vector<Triangle> _triangles;
  std::vector<vec2> _uv{};
};

#endif // TRIANGLE_MESH_HPP