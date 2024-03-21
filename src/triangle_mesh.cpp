#include "triangle_mesh.hpp"

#include <chrono>
#include <fstream>
#include <sstream>

#include "custom_assert.hpp"
#include "log.hpp"

using namespace glm;

void TriangleMeshData::addVertex(vec3 v) { vertices.push_back(v); }

void TriangleMeshData::addNormal(vec3 n) { normals.push_back(n); }

void TriangleMeshData::addTriangle(IndexedTriangle t) {
  triangles.push_back(t);
}

void TriangleMeshData::addUV(vec2 uv) { uvs.push_back(uv); }

TriangleMeshData TriangleMeshData::fromObj(const std::string &file) {
  using namespace std::chrono;

  TriangleMeshData data;

  std::ifstream is{file};
  if (!is) {
    logMsg("[ERROR] Could not open file %s, terminating", file.c_str());
    std::terminate();
  }

  std::vector<vec3> vertices, normals;

  logMsg("[INFO] Reading OBJ file \"%s\"...\n", file.c_str());
  auto start{steady_clock::now()};

  std::string line;
  unsigned i{};
  while (std::getline(is, line)) {
    std::istringstream iss{line};
    std::string type;
    iss >> type;
    if (type == "v") {
      vec3 v;
      iss >> v.x >> v.y >> v.z;
      vertices.push_back(v);
    } else if (type == "vn") {
      vec3 n;
      iss >> n.x >> n.y >> n.z;
      normals.push_back(n);
    } else if (type == "f") {
      unsigned v1, v2, v3, n1, n2, n3;
      iss >> v1;
      iss.ignore(2);
      iss >> n1;
      iss >> v2;
      iss.ignore(2);
      iss >> n2;
      iss >> v3;
      iss.ignore(2);
      iss >> n3;
      data.addVertex(vertices[size_t(v1) - 1]);
      data.addVertex(vertices[size_t(v2) - 1]);
      data.addVertex(vertices[size_t(v3) - 1]);
      data.addNormal(normals[size_t(n1) - 1]);
      data.addNormal(normals[size_t(n2) - 1]);
      data.addNormal(normals[size_t(n3) - 1]);
      data.addTriangle({i, i + 1, i + 2});
      i += 3;
    }
  }

  auto end{steady_clock::now()};
  logMsg("[INFO] Reading took %g s and used %zu B of memory\n",
         duration_cast<microseconds>(end - start).count() / 1e6f,
         (vertices.size() + normals.size()) * sizeof(vec3));

  return data;
}

TriangleMeshData TriangleMeshData::cube() {
  TriangleMeshData data;

  // front face
  data.addVertex({-0.5, -0.5, 0.5}); // 0
  data.addNormal({0, 0, 1});         // 0
  data.addVertex({0.5, -0.5, 0.5});  // 1
  data.addNormal({0, 0, 1});         // 1
  data.addVertex({0.5, 0.5, 0.5});   // 2
  data.addNormal({0, 0, 1});         // 2
  data.addVertex({-0.5, 0.5, 0.5});  // 3
  data.addNormal({0, 0, 1});         // 3
  data.addTriangle({0, 1, 2});
  data.addTriangle({2, 3, 0});

  // back face
  data.addVertex({0.5, -0.5, -0.5});  // 4
  data.addNormal({0, 0, -1});         // 4
  data.addVertex({-0.5, -0.5, -0.5}); // 5
  data.addNormal({0, 0, -1});         // 5
  data.addVertex({-0.5, 0.5, -0.5});  // 6
  data.addNormal({0, 0, -1});         // 6
  data.addVertex({0.5, 0.5, -0.5});   // 7
  data.addNormal({0, 0, -1});         // 7
  data.addTriangle({4, 5, 6});
  data.addTriangle({6, 7, 4});

  // left face
  data.addVertex({-0.5, -0.5, -0.5}); // 8
  data.addNormal({-1, 0, 0});         // 8
  data.addVertex({-0.5, -0.5, 0.5});  // 9
  data.addNormal({-1, 0, 0});         // 9
  data.addVertex({-0.5, 0.5, 0.5});   // 10
  data.addNormal({-1, 0, 0});         // 10
  data.addVertex({-0.5, 0.5, -0.5});  // 11
  data.addNormal({-1, 0, 0});         // 11
  data.addTriangle({8, 9, 10});
  data.addTriangle({10, 11, 8});

  // right face
  data.addVertex({0.5, -0.5, 0.5});  // 12
  data.addNormal({1, 0, 0});         // 12
  data.addVertex({0.5, -0.5, -0.5}); // 13
  data.addNormal({1, 0, 0});         // 13
  data.addVertex({0.5, 0.5, -0.5});  // 14
  data.addNormal({1, 0, 0});         // 14
  data.addVertex({0.5, 0.5, 0.5});   // 15
  data.addNormal({1, 0, 0});         // 15
  data.addTriangle({12, 13, 14});
  data.addTriangle({14, 15, 12});

  // bottom face
  data.addVertex({-0.5, -0.5, -0.5}); // 16
  data.addNormal({0, -1, 0});         // 16
  data.addVertex({0.5, -0.5, -0.5});  // 17
  data.addNormal({0, -1, 0});         // 17
  data.addVertex({0.5, -0.5, 0.5});   // 18
  data.addNormal({0, -1, 0});         // 18
  data.addVertex({-0.5, -0.5, 0.5});  // 19
  data.addNormal({0, -1, 0});         // 19
  data.addTriangle({16, 17, 18});
  data.addTriangle({18, 19, 16});

  // top face
  data.addVertex({-0.5, 0.5, 0.5});  // 20
  data.addNormal({0, 1, 0});         // 20
  data.addVertex({0.5, 0.5, 0.5});   // 21
  data.addNormal({0, 1, 0});         // 21
  data.addVertex({0.5, 0.5, -0.5});  // 22
  data.addNormal({0, 1, 0});         // 22
  data.addVertex({-0.5, 0.5, -0.5}); // 23
  data.addNormal({0, 1, 0});         // 23
  data.addTriangle({20, 21, 22});
  data.addTriangle({22, 23, 20});

  return data;
}

TriangleMeshData TriangleMeshData::plane() {
  TriangleMeshData data;

  data.addVertex({-1, 0, 1});
  data.addVertex({1, 0, 1});
  data.addVertex({1, 0, -1});
  data.addVertex({-1, 0, -1});

  data.addNormal({0, 1, 0});
  data.addNormal({0, 1, 0});
  data.addNormal({0, 1, 0});
  data.addNormal({0, 1, 0});

  data.addUV({0, 0});
  data.addUV({1, 0});
  data.addUV({1, 1});
  data.addUV({0, 1});

  data.addTriangle({0, 1, 2});
  data.addTriangle({2, 3, 0});

  return data;
}

TriangleMesh::TriangleMesh(TriangleMeshData &&data) : _data{data} {}

// TriangleMesh::TriangleMesh(const TriangleMesh &other)
//     : TransformableObject{other}, _vertices{other._vertices},
//       _normals{other._normals}, _triangles{other._triangles} {}

TriangleMesh::TriangleMesh(TriangleMesh &&other) noexcept
    : _data{std::move(other._data)} {}

// TriangleMesh &TriangleMesh::operator=(const TriangleMesh &other) {
//   if (this == &other)
//     goto skip;
//   _vertices = other._vertices;
//   _normals = other._normals;
//   _triangles = other._triangles;
// skip:
//   return *this;
// }

TriangleMesh &TriangleMesh::operator=(TriangleMesh &&other) noexcept {
  if (this == &other)
    goto skip;
  _data = std::move(other._data);
skip:
  return *this;
}

const std::vector<vec3> &TriangleMesh::vertices() const {
  return _data.vertices;
}

const std::vector<vec3> &TriangleMesh::normals() const { return _data.normals; }

const std::vector<IndexedTriangle> &TriangleMesh::triangles() const {
  return _data.triangles;
}

const std::vector<vec2> &TriangleMesh::uv() const { return _data.uvs; }

// if mass == 0, then assume infinite mass
