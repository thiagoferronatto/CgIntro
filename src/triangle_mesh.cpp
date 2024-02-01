#include "triangle_mesh.hpp"

#include <chrono>
#include <fstream>
#include <sstream>

#include "custom_assert.hpp"
#include "log.hpp"

std::shared_ptr<TriangleMesh> TriangleMesh::fromObj(const std::string &file) {
  using namespace std::chrono;

  auto mesh{
      std::make_shared<TriangleMesh>(file.substr(file.find_last_of('/') + 1) +
                                     std::to_string(_customMeshes++))};

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
  logMsg("[INFO] Reading took %g s and used %zu B of memory\n",
         duration_cast<microseconds>(end - start).count() / 1e6f,
         (vertices.size() + normals.size()) * sizeof(vec3));

  auto mtlFile{file.substr(0, file.size() - 4) + ".mtl"};
  is = std::ifstream{mtlFile};

  if (!is)
    return mesh;

  logMsg("[INFO] Reading MTL file \"%s\"...\n", mtlFile.c_str());
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

  logMsg("[INFO] Reading done");

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

void TriangleMesh::translate(vec3 xyz) {
  this->TransformableObject::translate(xyz);
  // no need to rebound when simply translating
  _boundingBox.a += xyz;
  _boundingBox.b += xyz;
  // bound();
}

void TriangleMesh::rotate(vec3 euler) {
  this->TransformableObject::rotate(euler);
  bound();
}

void TriangleMesh::scale(vec3 xyz) {
  this->TransformableObject::scale(xyz);
  bound();
}

void TriangleMesh::bound() {
  _boundingBox.a = vec3{std::numeric_limits<float>::max()};
  _boundingBox.b = vec3{std::numeric_limits<float>::lowest()};
  for (auto &local_v : _vertices) {
    vec3 v = _transform * vec4{local_v, 1};
    _boundingBox.a = min(_boundingBox.a, v);
    _boundingBox.b = max(_boundingBox.b, v);
  }
  _isBound = true;
}

void TriangleMesh::initializeRigidBody(float mass) {
  this->RigidBody::initializeRigidBody(mass);
  float vertexMass = 1.0f / (float(_vertices.size()) * _inverseMass);
  vec3 centerOfMass{};
  for (auto &local_v : _vertices) {
    vec3 v{_transform * vec4{local_v, 1}};
    _inertiaTensor.x += vertexMass * (v.y * v.y + v.z * v.z);
    _inertiaTensor.y += vertexMass * (v.x * v.x + v.z * v.z);
    _inertiaTensor.z += vertexMass * (v.x * v.x + v.y * v.y);
    centerOfMass += v;
  }
  centerOfMass /= float(_vertices.size());
}

std::shared_ptr<TriangleMesh> TriangleMesh::cube() {
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

std::shared_ptr<TriangleMesh> TriangleMesh::plane() {
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
