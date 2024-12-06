#include <algorithm>
#include <numeric>
#include <random>
#include <ranges>
#include <vector>

#include "gl_util.hpp"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "window.hpp"

struct TriangleMesh {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<glm::uvec3> triangles;
};

auto tessellate(auto&& controlPoints, unsigned level) {
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

struct QuadMesh {
  std::vector<glm::vec3> vertices;
  std::vector<GLuint> indices;
};

int main() {
  constexpr size_t w{1600}, h{900};
  Window window{w, h, "Computer Graphics Intro"};

  TriangleMesh mesh;
  // mesh.vertices.push_back({-0.5, 0, 0.5});
  // mesh.vertices.push_back({0.5, 0, 0.5});
  // mesh.vertices.push_back({0, 0, -0.5});

  // mesh.normals.push_back({0, 1, 0});
  // mesh.normals.push_back({0, 1, 0});
  // mesh.normals.push_back({0, 1, 0});

  // mesh.triangles.push_back({0, 1, 2});

  std::vector<glm::vec3> ctrlPts;
  ctrlPts.push_back({0, 0, 0});
  ctrlPts.push_back({1, 0, 0});
  ctrlPts.push_back({2, 0, 0});
  ctrlPts.push_back({3, 0, 0});

  ctrlPts.push_back({0, 0, -1});
  ctrlPts.push_back({1, 0, -1});
  ctrlPts.push_back({2, 5, -1});
  ctrlPts.push_back({3, 0, -1});

  ctrlPts.push_back({0, 0, -2});
  ctrlPts.push_back({1, 0, -2});
  ctrlPts.push_back({2, 0, -2});
  ctrlPts.push_back({3, 0, -2});

  ctrlPts.push_back({0, 0, -3});
  ctrlPts.push_back({1, 0, -3});
  ctrlPts.push_back({2, 0, -3});
  ctrlPts.push_back({3, 0, -3});

  mesh = tessellate(ctrlPts, 2048);

  GLuint vbos[2];
  glGenBuffers(2, vbos);
  glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
  glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(glm::vec3),
               mesh.vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
  glBufferData(GL_ARRAY_BUFFER, mesh.normals.size() * sizeof(glm::vec3),
               mesh.normals.data(), GL_STATIC_DRAW);

  GLuint vao;
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(1);

  GLuint ebo;
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               mesh.triangles.size() * sizeof(glm::uvec3),
               mesh.triangles.data(), GL_STATIC_DRAW);

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

    in vec3 fragNormal;
    in vec3 fragPosition;

    out vec4 fragColor;

    void main(void) {
      vec3 normal = normalize(fragNormal);

      const float ambient = 0.1;

      vec3 lights[6];
      lights[0] = vec3( 0, 3,  0);
      
      vec3 v = camPos - fragPosition;
      float camDist = length(v);
      v /= camDist;

      vec3 diffuse = vec3(1, 0, 0);

      vec3 color = ambient * diffuse;

      for (int i = 0; i < 1; ++i) {
        vec3 lPos = lights[i];
        
        vec3 l = lPos - fragPosition;
        float lightDist = length(l);
        l /= lightDist;
        float invd = 1.0 / (camDist + lightDist);
        float d = 0.5 * dot(normal, l) + 0.5;
        d *= d;

        vec3 h = normalize(v + l);
        const float alpha = 50.0;
        float s = pow(max(dot(normal, h), 0.0), alpha);

        const float intensity = 20;

        color += min(invd * invd, 1.0) * intensity * (d * diffuse + s);
      }

      fragColor = vec4(color, 1);
    }
  )"};

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

  auto viewLoc = glGetUniformLocation(program, "view");
  auto projLoc = glGetUniformLocation(program, "proj");
  auto camPosLoc = glGetUniformLocation(program, "camPos");

  auto camTrs = glm::inverse(glm::lookAt(glm::vec3{0, 0.5, 1}, {}, {0, 1, 0}));

  {
    auto view = glm::inverse(camTrs);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0].x);
    glUniform3fv(camPosLoc, 1, &camTrs[3].x);
  }
  auto proj =
      glm::perspective(glm::radians(74.0f), 16.0f / 9.0f, 0.01f, 100.0f);
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0].x);

  glEnable(GL_DEPTH_TEST);
  glPointSize(10);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glfwSwapInterval(1);

  glClearColor(0.1, 0.1, 0.1, 1);

  glCullFace(GL_FRONT);
  glEnable(GL_CULL_FACE);

  bool wtcUpdated = false;

  float dt = 0;

  window.show();

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

    glDrawElements(GL_TRIANGLES, 3 * mesh.triangles.size(), GL_UNSIGNED_INT,
                   nullptr);

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