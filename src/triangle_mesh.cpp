#include "triangle_mesh.hpp"

#include <chrono>
#include <fstream>
#include <sstream>

#include "custom_assert.hpp"
#include "log.hpp"

std::shared_ptr<TriangleMesh> TriangleMesh::fromObj(const std::string &file) {
  using namespace std::chrono;

  auto mesh{
      std::make_shared<TriangleMesh>(file.substr(file.find_last_of('/') + 1))};

  std::ifstream is{file};
  if (!is) {
    log("[ERROR] Could not open file %s, terminating", file.c_str());
    std::terminate();
  }

  std::vector<vec3> vertices, normals;

  log("[INFO] Reading OBJ file \"%s\"...\n", file.c_str());
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
      mesh->addVertex(vertices[size_t(v1) - 1]);
      mesh->addVertex(vertices[size_t(v2) - 1]);
      mesh->addVertex(vertices[size_t(v3) - 1]);
      mesh->addNormal(normals[size_t(n1) - 1]);
      mesh->addNormal(normals[size_t(n2) - 1]);
      mesh->addNormal(normals[size_t(n3) - 1]);
      mesh->addTriangle({i, i + 1, i + 2});
      i += 3;
    }
  }

  auto end{steady_clock::now()};
  log("[INFO] Reading took %g s and used %zu B of memory\n",
      duration_cast<microseconds>(end - start).count() / 1e6f,
      (vertices.size() + normals.size()) * sizeof(vec3));

  auto mtlFile{file.substr(0, file.size() - 4) + ".mtl"};
  is = std::ifstream{mtlFile};

  if (!is)
    return mesh;

  log("[INFO] Reading MTL file \"%s\"...\n", mtlFile.c_str());
  Material material;
  while (std::getline(is, line)) {
    std::istringstream iss{line};
    std::string prop;
    iss >> prop;
    if (prop == "Ka")
      iss >> material.Ka.x >> material.Ka.y >> material.Ka.z;
    else if (prop == "Kd")
      iss >> material.Kd.x >> material.Kd.y >> material.Kd.z;
    else if (prop == "Ks")
      iss >> material.Ks.x >> material.Ks.y >> material.Ks.z;
    else if (prop == "Ns")
      iss >> material.Ns;
    else if (prop == "Ni")
      iss >> material.Ni;
    else if (prop == "d")
      iss >> material.d;
    else if (prop == "map_Kd")
      iss >> material.map_Kd;
  }
  mesh->material = material;

  log("[INFO] Reading done");

  return mesh;
}

TriangleMesh::TriangleMesh(std::string name) : Actor{std::move(name)} {}

// TriangleMesh::TriangleMesh(const TriangleMesh &other)
//     : TransformableObject{other}, _vertices{other._vertices},
//       _normals{other._normals}, _triangles{other._triangles} {}

TriangleMesh::TriangleMesh(TriangleMesh &&other) noexcept
    : Actor{std::move(other)}, _vertices{std::move(other._vertices)},
      _normals{std::move(other._normals)},
      _triangles{std::move(other._triangles)} {}

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
  _name = std::move(other._name);
  _transform = std::move(other._transform);
  material = std::move(other.material);
  _vertices = std::move(other._vertices);
  _normals = std::move(other._normals);
  _triangles = std::move(other._triangles);
skip:
  return *this;
}

void TriangleMesh::addVertex(vec3 v) { _vertices.push_back(v); }

void TriangleMesh::addNormal(vec3 n) { _normals.push_back(n); }

void TriangleMesh::addTriangle(Triangle t) { _triangles.push_back(t); }

void TriangleMesh::addUV(vec2 uv) { _uv.push_back(uv); }

const std::vector<vec3> &TriangleMesh::vertices() const { return _vertices; }

const std::vector<vec3> &TriangleMesh::normals() const { return _normals; }

const std::vector<TriangleMesh::Triangle> &TriangleMesh::triangles() const {
  return _triangles;
}

const std::vector<vec2> &TriangleMesh::uv() const { return _uv; }
