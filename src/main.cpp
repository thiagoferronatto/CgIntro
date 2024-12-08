#include <algorithm>
#include <numeric>
#include <print>
#include <random>
#include <ranges>
#include <vector>

#include "gl_util.hpp"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "libaffa/aa.h"
#include "window.hpp"

#define USE_AA_INSTEAD_OF_IA

#ifdef USE_AA_INSTEAD_OF_IA
using Type = AAF;
#else
using Type = interval;
#endif

struct TriangleMesh {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<glm::uvec3> triangles;

  // OpenGL stuff
  GLuint vbos[2]{};
  GLuint ebo{};
};

struct AxisAlignedBox {
  glm::vec3 min;
  glm::vec3 max;
};

using AAB = AxisAlignedBox;

auto getBoxFromBezierPatch(auto&& ctrlPts, const Type& u, const Type& v) {
  Type B[4];
  B[0] = B[3] = 1;
  B[1] = B[2] = 3;

  auto u2 = u * u, u3 = u2 * u;
  auto uc = Type{1} - u, uc2 = uc * uc, uc3 = uc2 * uc;
  auto v2 = v * v, v3 = v2 * v;
  auto vc = Type{1} - v, vc2 = vc * vc, vc3 = vc2 * vc;
  Type U[4]{1, u, u2, u3};
  Type UC[4]{1, uc, uc2, uc3};
  Type V[4]{1, v, v2, v3};
  Type VC[4]{1, vc, vc2, vc3};

  Type xhat;
  Type yhat;
  Type zhat;
  for (int i = 0; i < 4; ++i) {
    auto BU = B[i] * U[i] * UC[3 - i];
    for (int j = 0; j < 4; ++j) {
      auto BV = B[j] * V[j] * VC[3 - j];
      auto p = ctrlPts[4 * i + j];
      auto BUBV = BU * BV;
      xhat = xhat + BUBV * p.x;
      yhat = yhat + BUBV * p.y;
      zhat = zhat + BUBV * p.z;
    }
  }

#ifdef USE_AA_INSTEAD_OF_IA
  auto xi{xhat.convert()};
  auto yi{yhat.convert()};
  auto zi{zhat.convert()};
#else
  auto xi{xhat};
  auto yi{yhat};
  auto zi{zhat};
#endif

  return AAB{{xi.left(), yi.left(), zi.left()},
             {xi.right(), yi.right(), zi.right()}};
}

auto tessellateBezierPatch(auto&& controlPoints, unsigned level) {
  TriangleMesh mesh;
  auto stepSize{1.0f / level};
  float u{};
  float v{};

  for (unsigned i{}; i < level; ++i, u += stepSize) {
    for (unsigned j{}; j < level; ++j, v += stepSize) {
      float B[4];
      B[0] = B[3] = 1;
      B[1] = B[2] = 3;

      float u2 = u * u, u3 = u2 * u;
      float uc = 1 - u, uc2 = uc * uc, uc3 = uc2 * uc;
      float v2 = v * v, v3 = v2 * v;
      float vc = 1 - v, vc2 = vc * vc, vc3 = vc2 * vc;
      float U[4]{1, u, u2, u3};
      float DU[4]{0, 1, 2 * u, 3 * u2};
      float UC[4]{1, uc, uc2, uc3};
      float DUC[4]{0, -1, 2 * u - 2, -3 * uc2};
      float V[4]{1, v, v2, v3};
      float DV[4]{0, 1, 2 * v, 3 * v2};
      float VC[4]{1, vc, vc2, vc3};
      float DVC[4]{0, -1, 2 * v - 2, -3 * vc2};

      glm::vec3 s{};
      glm::vec3 du{};
      glm::vec3 dv{};
      for (int i = 0; i < 4; ++i) {
        float BU = B[i] * U[i] * UC[3 - i];
        float DBU = B[i] * (DU[i] * UC[3 - i] + U[i] * DUC[3 - i]);
        for (int j = 0; j < 4; ++j) {
          float BV = B[j] * V[j] * VC[3 - j];
          float DBV = B[j] * (DV[j] * VC[3 - j] + V[j] * DVC[3 - j]);
          auto p = controlPoints[4 * i + j];
          s += BU * BV * p;
          du += DBU * BV * p;
          dv += BU * DBV * p;
        }
      }
      mesh.vertices.push_back(std::move(s));
      auto vertexNormal{glm::normalize(glm::cross(dv, du))};
      mesh.normals.push_back(std::move(vertexNormal));
    }
    v = 0;
  }
  for (unsigned i{level}, end{level * level}; i < end; ++i) {
    if (i % level == level - 1) continue;
    mesh.triangles.push_back({i - level, i, i + 1});
    mesh.triangles.push_back({i + 1, i - level + 1, i - level});
  }
  return mesh;
}

constexpr auto vsSrc{R"(
    #version 460

    layout (location = 0) in vec3 position;
    layout (location = 1) in vec3 normal;

    uniform mat4 view;
    uniform mat4 proj;

    out vec3 fragPosition;
    out vec3 fragNormal;

    void main(void) {
      fragPosition = position;
      fragNormal = normal;
      gl_Position = proj * view * vec4(position, 1);
    }
  )"};

constexpr auto fsSrc{R"(
    #version 460

    uniform mat4 view;
    uniform vec3 camPos;
    uniform vec3 diffuse;
    uniform int shade;

    in vec3 fragNormal;
    in vec3 fragPosition;

    out vec4 fragColor;

    void main(void) {
      if (shade == 0) {
        fragColor = vec4(diffuse, 0.1);
        return;
      }

      vec3 normal = normalize(fragNormal);

      const float ambient = 0.1;

      vec3 lights[6];
      lights[0] = vec3( 1, 3,  -4);
      
      vec3 v = camPos - fragPosition;
      float camDist = length(v);
      v /= camDist;

      vec3 color = ambient * diffuse;

      for (int i = 0; i < 1; ++i) {
        vec3 lPos = lights[i];
        
        vec3 l = lPos - fragPosition;
        float lightDist = length(l);
        l /= lightDist;
        float invd = 1.0 /  lightDist;
        float d = max(dot(normal, l), 0);

        vec3 h = normalize(v + l);
        const float alpha = 50;
        float s = pow(max(dot(normal, h), 0.0), alpha);

        const float intensity = 5;

        color += min(invd * invd, 1.0) * intensity * (d * diffuse + s);
      }

      fragColor = vec4(color, 1);
    }
  )"};

auto setupProgram() {
  auto vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vsSrc, nullptr);
  glCompileShader(vs);
  glCheckShaderCompilation(vs);

  auto fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fsSrc, nullptr);
  glCompileShader(fs);
  glCheckShaderCompilation(fs);

  auto program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glCheckProgramLinkage(program);

  glUseProgram(program);

  glEnable(GL_DEPTH_TEST);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glClearColor(0.1, 0.1, 0.1, 1);

  return program;
}

GLuint vao{};
void drawMesh(TriangleMesh& mesh) {
  if (!mesh.vbos[0] || !mesh.vbos[1] || !mesh.ebo) {
    glDeleteBuffers(2, mesh.vbos);
    glDeleteBuffers(1, &mesh.ebo);
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(glm::vec3),
                 mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, mesh.normals.size() * sizeof(glm::vec3),
                 mesh.normals.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 mesh.triangles.size() * sizeof(glm::uvec3),
                 mesh.triangles.data(), GL_STATIC_DRAW);
  }

  if (!vao) glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[1]);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);

  glDrawElements(GL_TRIANGLES, 3 * mesh.triangles.size(), GL_UNSIGNED_INT,
                 nullptr);
}

TriangleMesh generateAABBTriangleMesh(const glm::vec3& min,
                                      const glm::vec3& max) {
  TriangleMesh mesh;

  // Define the 8 corners of the AABB
  glm::vec3 v0 = {min.x, min.y, min.z};
  glm::vec3 v1 = {min.x, min.y, max.z};
  glm::vec3 v2 = {max.x, min.y, max.z};
  glm::vec3 v3 = {max.x, min.y, min.z};
  glm::vec3 v4 = {min.x, max.y, min.z};
  glm::vec3 v5 = {min.x, max.y, max.z};
  glm::vec3 v6 = {max.x, max.y, max.z};
  glm::vec3 v7 = {max.x, max.y, min.z};

  // Add vertices for each face (flat shading requires duplication)
  mesh.vertices = {
      v0, v1, v2, v3,  // Bottom face
      v4, v5, v6, v7,  // Top face
      v1, v2, v6, v5,  // Front face
      v0, v4, v7, v3,  // Back face
      v0, v1, v5, v4,  // Left face
      v2, v3, v7, v6   // Right face
  };

  // Add normals for each face (flat shading requires one normal per vertex)
  glm::vec3 nBottom = {0, -1, 0};
  glm::vec3 nTop = {0, 1, 0};
  glm::vec3 nFront = {0, 0, 1};
  glm::vec3 nBack = {0, 0, -1};
  glm::vec3 nLeft = {-1, 0, 0};
  glm::vec3 nRight = {1, 0, 0};

  mesh.normals = {
      nBottom, nBottom, nBottom, nBottom,  // Bottom face
      nTop,    nTop,    nTop,    nTop,     // Top face
      nFront,  nFront,  nFront,  nFront,   // Front face
      nBack,   nBack,   nBack,   nBack,    // Back face
      nLeft,   nLeft,   nLeft,   nLeft,    // Left face
      nRight,  nRight,  nRight,  nRight    // Right face
  };

  // Add triangles (indices)
  mesh.triangles = {
      {0, 1, 2},    {2, 3, 0},     // Bottom face
      {4, 5, 6},    {6, 7, 4},     // Top face
      {8, 9, 10},   {8, 10, 11},   // Front face
      {14, 13, 12}, {15, 14, 12},  // Back face
      {16, 17, 18}, {16, 18, 19},  // Left face
      {20, 21, 22}, {20, 22, 23}   // Right face
  };

  return mesh;
}

int main() {
  constexpr size_t w{1600}, h{900};
  Window window{w, h, "Computer Graphics Intro"};

  TriangleMesh mesh;

  std::vector<glm::vec3> ctrlPts;
  ctrlPts.push_back({0, 1, 0});
  ctrlPts.push_back({1, 0, 0});
  ctrlPts.push_back({2, 0, 0});
  ctrlPts.push_back({3, 1, 0});

  ctrlPts.push_back({0, 0, -1});
  ctrlPts.push_back({1, 2, -1});
  ctrlPts.push_back({2, 2, -1});
  ctrlPts.push_back({3, 0, -1});

  ctrlPts.push_back({0, 0, -2});
  ctrlPts.push_back({1, 2, -2});
  ctrlPts.push_back({2, 2, -2});
  ctrlPts.push_back({3, 0, -2});

  ctrlPts.push_back({0, 1, -3});
  ctrlPts.push_back({1, 0, -3});
  ctrlPts.push_back({2, 0, -3});
  ctrlPts.push_back({3, 1, -3});

  for (auto& ctrlPt : ctrlPts) ctrlPt += glm::vec3{-1.5, 0, -1.5};

  mesh = tessellateBezierPatch(ctrlPts, 512);

  std::vector<TriangleMesh> boxMeshes;

  float u{};
  float v{};
  auto n{16};
  auto incr{1.0f / n};
  for (int i{}; i < n; ++i, u += incr, v = 0) {
    for (int j{}; j < n; ++j, v += incr) {
      auto box{getBoxFromBezierPatch(ctrlPts, interval{u, u + incr},
                                     interval{v, v + incr})};
      boxMeshes.push_back(generateAABBTriangleMesh(box.min, box.max));
    }
  }

  auto program = setupProgram();

  auto viewLoc = glGetUniformLocation(program, "view");
  auto projLoc = glGetUniformLocation(program, "proj");
  auto camPosLoc = glGetUniformLocation(program, "camPos");
  auto diffuseLoc = glGetUniformLocation(program, "diffuse");
  auto shadeLoc = glGetUniformLocation(program, "shade");

  auto camTrs =
      glm::inverse(glm::lookAt(glm::vec3{0, 3, 2}, {0, 2, 0}, {0, 1, 0}));

  {
    auto view = glm::inverse(camTrs);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0].x);
    glUniform3fv(camPosLoc, 1, &camTrs[3].x);
  }
  auto proj =
      glm::perspective(glm::radians(40.0f), 16.0f / 9.0f, 0.01f, 100.0f);
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0].x);

  bool wtcUpdated = false;

  float dt = 0;

  window.show();

  glm::vec3 diffuse;

  glUniform1i(shadeLoc, 1);

  while (!window.shouldClose()) {
    auto start = std::chrono::steady_clock::now();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    constexpr float factor{5};

    /* user input */ {
      glm::vec3 globalY{glm::inverse(camTrs) * glm::vec4{0, 1, 0, 0}};
      if (window.keyIsPressed('W')) {
        camTrs = glm::translate(camTrs, {0, 0, -factor * dt});
        wtcUpdated = true;
      }
      if (window.keyIsPressed('A')) {
        camTrs = glm::translate(camTrs, {-factor * dt, 0, 0});
        wtcUpdated = true;
      }
      if (window.keyIsPressed('S')) {
        camTrs = glm::translate(camTrs, {0, 0, factor * dt});
        wtcUpdated = true;
      }
      if (window.keyIsPressed('D')) {
        camTrs = glm::translate(camTrs, {factor * dt, 0, 0});
        wtcUpdated = true;
      }
      if (window.keyIsPressed(GLFW_KEY_SPACE)) {
        camTrs = glm::translate(camTrs, factor * dt * globalY);
        wtcUpdated = true;
      }
      if (window.keyIsPressed(GLFW_KEY_LEFT_CONTROL)) {
        camTrs = glm::translate(camTrs, -factor * dt * globalY);
        wtcUpdated = true;
      }
      if (window.keyIsPressed('Q')) {
        camTrs = glm::rotate(camTrs, factor * dt, globalY);
        wtcUpdated = true;
      }
      if (window.keyIsPressed('E')) {
        camTrs = glm::rotate(camTrs, -factor * dt, globalY);
        wtcUpdated = true;
      }
      if (window.keyIsPressed('R')) {
        camTrs = glm::rotate(camTrs, factor * dt, {1, 0, 0});
        wtcUpdated = true;
      }
      if (window.keyIsPressed('F')) {
        camTrs = glm::rotate(camTrs, -factor * dt, {1, 0, 0});
        wtcUpdated = true;
      }

      if (window.keyIsPressed(GLFW_KEY_ESCAPE)) break;

      if (wtcUpdated) {
        auto view = glm::inverse(camTrs);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0].x);
        glUniform3fv(camPosLoc, 1, &camTrs[3].x);
        wtcUpdated = false;
      }
    }

    diffuse = {1, 0, 0};
    glUniform3fv(diffuseLoc, 1, &diffuse.x);
    drawMesh(mesh);

    diffuse = {0, 1, 1};
    glUniform3fv(diffuseLoc, 1, &diffuse.x);
    for (auto& boxMesh : boxMeshes) drawMesh(boxMesh);

    window.swapBuffers();
    window.pollEvents();

    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    auto msDiff = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
    auto msDiffCount = msDiff.count();
    dt = 0.001f * msDiffCount;
  }

  return 0;
}