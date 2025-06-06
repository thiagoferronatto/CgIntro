#include <algorithm>
#include <numeric>
#include <print>
#include <random>
#include <ranges>
#include <vector>

#include "bezier_patch.hpp"
#include "gl_util.hpp"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "libaffa/aa.h"
#include "types.hpp"
#include "window.hpp"

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
      lights[0] = vec3(0, 5, 0);
      lights[1] = vec3(0, -5, 0);
      
      vec3 v = camPos - fragPosition;
      float camDist = length(v);
      v /= camDist;

      vec3 color = ambient * diffuse;

      for (int i = 0; i < 2; ++i) {
        vec3 lPos = lights[i];
        
        vec3 l = lPos - fragPosition;
        //l = vec3(0, 1, 0);
        float lightDist = length(l);
        l /= lightDist;
        float invd = 1.0 /  lightDist;
        float d = dot(normal, l);
        d = 0.5 * d + 0.5;
        d *= d;

        vec3 h = normalize(v + l);
        const float alpha = 100;
        float s = pow(max(dot(normal, h), 0.0), alpha);

        const float intensity = 10;

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

  glClearColor(0, 0, 0, 1);

  return program;
}

int main() {
  constexpr u64 w{1600}, h{900};
  Window window{w, h, "Surface tinkering"};

  BezierPatch::PointCloud controlPoints1 = {
      {1.4, 0, 0.5}, {0, 0, 3}, {3, 0, 3}, {1.6, 0, 0.5},
      {1.4, 1, 0.5}, {0, 1, 3}, {3, 1, 3}, {1.6, 1, 0.5},
      {1.4, 2, 0.5}, {0, 2, 3}, {3, 2, 3}, {1.6, 2, 0.5},
      {1.4, 3, 0.5}, {0, 3, 3}, {3, 3, 3}, {1.6, 3, 0.5}};

  BezierPatch::PointCloud controlPoints = {
      {0, 0, 0}, {0, 3, 0}, {3, 3, 0}, {3, 0, 0}, {1, 0, 1}, {0, 2, 1},
      {3, 2, 1}, {2, 0, 1}, {1, 0, 2}, {0, 2, 2}, {3, 2, 2}, {2, 0, 2},
      {0, 0, 3}, {0, 3, 3}, {3, 3, 3}, {3, 0, 3}};

  {
    controlPoints.clear();

    controlPoints.push_back({0, 1, 0});
    controlPoints.push_back({1, -1, 0});
    controlPoints.push_back({2, -1, 0});
    controlPoints.push_back({3, 1, 0});

    controlPoints.push_back({0, -1, -1});
    controlPoints.push_back({1, 2, -1});
    controlPoints.push_back({2, 2, -1});
    controlPoints.push_back({3, -1, -1});

    controlPoints.push_back({0, -1, -2});
    controlPoints.push_back({1, 2, -2});
    controlPoints.push_back({2, 2, -2});
    controlPoints.push_back({3, -1, -2});

    controlPoints.push_back({0, 1, -3});
    controlPoints.push_back({1, -1, -3});
    controlPoints.push_back({2, -1, -3});
    controlPoints.push_back({3, 1, -3});
  }

  for (auto &ctrlPt : controlPoints) {
    static constexpr glm::mat4 m4id = glm::identity<glm::mat4>();
    static constexpr f32 pi = glm::pi<f32>();
    static constexpr glm::vec3 up = {0, 1, 0};
    // auto matrix{glm::rotate(m4id, 0.125f * pi, up)};
    // ctrlPt = matrix * glm::vec4{ctrlPt, 1};
    ctrlPt += glm::vec3{-1.5, 0, -1.5};
  }

  BezierPatch patch = std::move(controlPoints);
  TriangleMesh patchMesh = patch.tessellate(1024);
  // for (glm::vec3 &n : patchMesh.normals)
  //   n *= -1;
  auto getBoundMeshes = &BezierPatch::getHullMeshes;

  i32 subdCount = 32;
  u64 noiseTerms = 5;
  auto boxMeshes{(patch.*getBoundMeshes)(subdCount)};

  auto program{setupProgram()};

  auto viewLoc{glGetUniformLocation(program, "view")};
  auto projLoc{glGetUniformLocation(program, "proj")};
  auto camPosLoc{glGetUniformLocation(program, "camPos")};
  auto diffuseLoc{glGetUniformLocation(program, "diffuse")};
  auto shadeLoc{glGetUniformLocation(program, "shade")};

  auto camTrs{
      glm::inverse(glm::lookAt(glm::vec3{0, 3, 2}, {0, 2, 0}, {0, 1, 0}))};

  {
    auto view{glm::inverse(camTrs)};
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0].x);
    glUniform3fv(camPosLoc, 1, &camTrs[3].x);
  }
  auto proj{glm::perspective(glm::radians(74.0f), 16.0f / 9.0f, 0.01f, 100.0f)};
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0].x);

  bool wtcUpdated{};
  bool shouldShowAabbs{};
  bool pWasPressedLastFrame{};
  bool upWasPressedLastFrame{};
  bool downWasPressedLastFrame{};
  bool leftWasPressedLastFrame{};
  bool rightWasPressedLastFrame{};
  float dt{};

  glm::vec3 diffuse;
  glUniform1i(shadeLoc, 1);

  window.show();

  while (!window.shouldClose()) {
    auto start{std::chrono::steady_clock::now()};

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* User input */ {
      constexpr f32 factor{4};

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

      if (window.keyIsPressed('P')) {
        if (!pWasPressedLastFrame)
          shouldShowAabbs = !shouldShowAabbs;
        pWasPressedLastFrame = true;
      } else {
        pWasPressedLastFrame = false;
      }

      if (window.keyIsPressed(GLFW_KEY_UP)) {
        if (!upWasPressedLastFrame) {
          std::cout << "[INFO] Resolution increased from " << subdCount;
          subdCount += 1;
          if (subdCount > 128)
            subdCount = 128;
          std::cout << " to " << subdCount << '\n';
          boxMeshes = std::move((patch.*getBoundMeshes)(subdCount));
        }
        upWasPressedLastFrame = true;
      } else {
        upWasPressedLastFrame = false;
      }

      if (window.keyIsPressed(GLFW_KEY_DOWN)) {
        if (!downWasPressedLastFrame) {
          std::cout << "[INFO] Resolution decreased from " << subdCount;
          subdCount -= 1;
          if (subdCount < 1)
            subdCount = 2;
          std::cout << " to " << subdCount << '\n';
          boxMeshes = std::move((patch.*getBoundMeshes)(subdCount));
        }
        downWasPressedLastFrame = true;
      } else {
        downWasPressedLastFrame = false;
      }

      if (window.keyIsPressed(GLFW_KEY_LEFT)) {
        if (!leftWasPressedLastFrame) {
          noiseTerms -= 1;
          if (noiseTerms < 4)
            noiseTerms = 4;
          boxMeshes = std::move((patch.*getBoundMeshes)(subdCount));
          std::cout << "[INFO] Noise-term count decreased from " << noiseTerms
                    << " to " << (noiseTerms - 1) << " ("
                    << (1 << (noiseTerms - 1)) << " vertices)\n";
        }
        leftWasPressedLastFrame = true;
      } else {
        leftWasPressedLastFrame = false;
      }

      if (window.keyIsPressed(GLFW_KEY_RIGHT)) {
        if (!rightWasPressedLastFrame) {
          noiseTerms += 1;
          if (noiseTerms > 30)
            noiseTerms = 30;
          boxMeshes = std::move((patch.*getBoundMeshes)(subdCount));
          std::cout << "[INFO] Noise-term count increased from "
                    << (noiseTerms - 2) << " to " << (noiseTerms - 1) << " ("
                    << (1 << (noiseTerms - 1)) << " vertices)\n";
        }
        rightWasPressedLastFrame = true;
      } else {
        rightWasPressedLastFrame = false;
      }

      if (window.keyIsPressed(GLFW_KEY_ESCAPE))
        break;

      if (wtcUpdated) {
        auto view{glm::inverse(camTrs)};
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0].x);
        glUniform3fv(camPosLoc, 1, &camTrs[3].x);
        wtcUpdated = false;
      }
    }

    // Patch visualization
    diffuse = {0, 1, 1};
    glUniform3fv(diffuseLoc, 1, &diffuse.x);
    patchMesh.draw();

    // AABB visualization
    if (shouldShowAabbs) {
      diffuse = {1, 0, 0};
      glUniform3fv(diffuseLoc, 1, &diffuse.x);

      // This is heavily unoptimized and should not be used for purposes other
      // than visualization. Currently, each box has its own separate mesh,
      // when they could all reference the same mesh with different
      // transformation matrices. Also, batch drawing would improve
      // performance, maybe by using something like glMultiDrawElements.
      for (auto &boxMesh : boxMeshes)
        boxMesh.draw();
    }

    window.swapBuffers();
    window.pollEvents();

    // Frame time calculation
    auto end{std::chrono::steady_clock::now()};
    dt = 1e-9f * (end - start).count();
  }

  return 0;
}