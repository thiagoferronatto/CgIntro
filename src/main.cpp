#include <algorithm>
#include <chrono>
#include <stack>
#include <utility>
#include <vector>

#include "bezier_patch.hpp"
#include "gl_util.hpp"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "triangle_mesh.hpp"
#include "types.hpp"
#include "window.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

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

      const float ambient = 0.01;

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
        float invd = 1 / length(l);
        l *= invd;
        vec3 h = normalize(v + l);


        float NdotL = clamp(dot(normal, l), 0.0, 1.0);
        float NdotV = clamp(dot(normal, v), 0.0, 1.0); // move outside of loop
        float NdotH = clamp(dot(normal, h), 0.0, 1.0);
        float LdotH = clamp(dot(l, h), 0.0, 1.0);
        float NdotHSqr = NdotH * NdotH;

        const float _Roughness = 0.1;
        const float _Metallic = 0.5;
        const float pi = 3.1415926535;

        float roughness = _Roughness * _Roughness;
        float roughnessSqr = roughness * roughness;

        float A = 1.0 - 0.5 * (roughnessSqr / (roughnessSqr + 0.33));
        float B = 0.45 * (roughnessSqr / (roughnessSqr + 0.09));
        float C = clamp(dot(normalize(v - normal * NdotV), normalize(l - normal * NdotL)), 0.0, 1.0);
        float angleL = acos(NdotL);
        float angleV = acos(NdotV);
        float alpha = max(angleL, angleV);
        float beta = min(angleL, angleV);
        vec3 d = diffuse * (A + B * C * sin(alpha) * tan(beta)) * NdotL;

        float D = roughnessSqr / (pi * pow(NdotHSqr * (roughnessSqr - 1.0) + 1.0, 2.0));
        float k  = roughness * 0.5;
        float gl = NdotL / (NdotL * (1.0 - k) + k);
        float gv = NdotV / (NdotV * (1.0 - k) + k);
        float G = gl * gv;
        float F = _Metallic + (1.0 - _Metallic) * pow(1.0 - LdotH, 5.0);
        vec3 specular = vec3(clamp((D * G * F) / (4.0 * NdotV), 0.0, 1.0) * pi);

        const float intensity = 25;

        color += invd * invd * intensity * clamp(mix(d, specular, _Metallic), 0.0, 1.0);


        // float invd = 1.0 /  lightDist;
        // float d = dot(normal, l);
        // d = 0.5 * d + 0.5;
        // d *= d;
        // const float alpha = 400;
        // float s = pow(max(dot(normal, h), 0.0), alpha);

        // color += min(invd * invd, 1.0) * intensity * (d * diffuse + s);
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

constexpr int maxRecDepth = 3;

using MeshArray = std::vector<TriangleMesh>;

struct RecSubdArgs {
  BezierPatch &p0; // ref to patch0
  BezierPatch &p1; // ref to patch1
  interval u0;     // u interval of patch 0's colliding box
  interval v0;     // v interval of patch 0's colliding box
  interval u1;     // u interval of patch 1's colliding box
  interval v1;     // v interval of patch 1's colliding box
  MeshArray *b0;   // for displaying the boxes
  MeshArray *b1;   // for displaying the boxes
  int depth = 0;   // to stop at a reasonable level
};

void subdivide(const RecSubdArgs &args) {
  using IntervalPair = std::pair<interval, interval>;

  std::stack<RecSubdArgs> stack;
  stack.push(args);

  while (!stack.empty()) {
    const auto args = stack.top(); // this is a 100 byte copy. can we avoid it?
    stack.pop();

    if (args.depth > maxRecDepth)
      continue;

    interval p0i[4], p1i[4];
    p0i[0] = {args.u0.left(), args.u0.mid()};
    p0i[1] = {args.u0.mid(), args.u0.right()};
    p0i[2] = {args.v0.left(), args.v0.mid()};
    p0i[3] = {args.v0.mid(), args.v0.right()};

    // the intervals corresponding to each new box from patch0
    IntervalPair bi0[4];
    bi0[0] = {p0i[0], p0i[2]};
    bi0[1] = {p0i[0], p0i[3]};
    bi0[2] = {p0i[1], p0i[2]};
    bi0[3] = {p0i[1], p0i[3]};

    p1i[0] = {args.u1.left(), args.u1.mid()};
    p1i[1] = {args.u1.mid(), args.u1.right()};
    p1i[2] = {args.v1.left(), args.v1.mid()};
    p1i[3] = {args.v1.mid(), args.v1.right()};

    // the intervals corresponding to each new box from patch1
    IntervalPair bi1[4];
    bi1[0] = {p1i[0], p1i[2]};
    bi1[1] = {p1i[0], p1i[3]};
    bi1[2] = {p1i[1], p1i[2]};
    bi1[3] = {p1i[1], p1i[3]};

    AAB p0b[4], p1b[4];
    for (int i = 0; i < 4; ++i) {
      p0b[i] = args.p0.getSubpatchAabb(bi0[i].first, bi0[i].second);
      for (int j = 0; j < 4; ++j) {
        p1b[j] = args.p1.getSubpatchAabb(bi1[j].first, bi1[j].second);
        if (p0b[i].overlapsWith(p1b[j])) {
          RecSubdArgs nextCallArgs{.p0 = args.p0,
                                   .p1 = args.p1,
                                   .u0 = bi0[i].first,
                                   .v0 = bi0[i].second,
                                   .u1 = bi1[j].first,
                                   .v1 = bi1[j].second,
                                   .b0 = args.b0,
                                   .b1 = args.b1,
                                   .depth = args.depth + 1};

          stack.push(nextCallArgs);
        }
      }
    }

    if (args.b0)
      for (int i = 0; i < 4; ++i)
        args.b0[args.depth].push_back(p0b[i].getMesh());

    if (args.b1)
      for (int i = 0; i < 4; ++i)
        args.b1[args.depth].push_back(p1b[i].getMesh());
  }
}

int main() {
  constexpr u64 w{1600}, h{900};
  Window window{w, h, "Surface tinkering"};

  BezierPatch::PointCloud controlPoints = {
      {1.4, 0, 0.5}, {0, 0, 3}, {3, 0, 3}, {1.6, 0, 0.5},
      {1.4, 1, 0.5}, {0, 1, 3}, {3, 1, 3}, {1.6, 1, 0.5},
      {1.4, 2, 0.5}, {0, 2, 3}, {3, 2, 3}, {1.6, 2, 0.5},
      {1.4, 3, 0.5}, {0, 3, 3}, {3, 3, 3}, {1.6, 3, 0.5}};

  BezierPatch::PointCloud controlPoints1 = {
      {0, 0, 0}, {0, 3, 0}, {3, 3, 0}, {3, 0, 0}, {1, 0, 1}, {0, 2, 1},
      {3, 2, 1}, {2, 0, 1}, {1, 0, 2}, {0, 2, 2}, {3, 2, 2}, {2, 0, 2},
      {0, 0, 3}, {0, 3, 3}, {3, 3, 3}, {3, 0, 3}};

  {
    controlPoints.clear();

    controlPoints.push_back({0, 1, 0});
    controlPoints.push_back({0, -1, -1});
    controlPoints.push_back({0, -1, -2});
    controlPoints.push_back({0, 1, -3});

    controlPoints.push_back({1, -1, 0});
    controlPoints.push_back({1, 2, -1});
    controlPoints.push_back({1, 2, -2});
    controlPoints.push_back({1, -1, -3});

    controlPoints.push_back({2, -1, 0});
    controlPoints.push_back({2, 2, -1});
    controlPoints.push_back({2, 2, -2});
    controlPoints.push_back({2, -1, -3});

    controlPoints.push_back({3, 1, 0});
    controlPoints.push_back({3, -1, -1});
    controlPoints.push_back({3, -1, -2});
    controlPoints.push_back({3, 1, -3});
  }

  // Positioning the patches
  for (auto &ctrlPt : controlPoints)
    ctrlPt += glm::vec3{-1.5, 0, 1.5};
  for (auto &ctrlPt : controlPoints1)
    ctrlPt = 0.9f * (ctrlPt + glm::vec3{-3.5, 0, -1.5});

  std::vector<BezierPatch> patches;

  // adding all patches to the patches vector
  patches.push_back(std::move(controlPoints));
  patches.push_back(std::move(controlPoints1));

  // Tessellation
  constexpr auto tessellationFactor = 1024;
  std::vector<TriangleMesh> patchMeshes(patches.size());
  std::transform(patches.begin(), patches.end(), patchMeshes.begin(),
                 [](const BezierPatch &p) { //
                   return p.tessellate(tessellationFactor);
                 });

  // this is awful, but it IS the cleanest way
#if 0 // use zonotopes
#define GET_BOUND_MESHES getHullMeshes
#else
#define GET_BOUND_MESHES getAabbMeshes
#endif

  u64 subdCount = 8;
  // u64 noiseTerms = 5;
  std::vector<std::vector<TriangleMesh>> allBoundsMeshes(patches.size());
  std::vector<std::vector<AAB>> allBoundsBoxes(patches.size());
  auto updateBoundMeshes = [&] {
    for (int i = 0; i < patches.size(); ++i) {
      allBoundsMeshes[i].clear(); // fixes memory leak
      std::vector<AAB> boxes;
      boxes.reserve(subdCount * subdCount);
      allBoundsMeshes[i] = patches[i].GET_BOUND_MESHES(subdCount, &boxes);
      allBoundsBoxes[i] = std::move(boxes);
    }
  };
  updateBoundMeshes();

  auto program = setupProgram();

  auto viewLoc = glGetUniformLocation(program, "view");
  auto projLoc = glGetUniformLocation(program, "proj");
  auto camPosLoc = glGetUniformLocation(program, "camPos");
  auto diffuseLoc = glGetUniformLocation(program, "diffuse");
  auto shadeLoc = glGetUniformLocation(program, "shade");

  glEnable(GL_FRAMEBUFFER_SRGB);

  auto camTrs =
      glm::inverse(glm::lookAt(glm::vec3{0, 3, 4}, {0, 0, 0}, {0, 1, 0}));

  {
    auto view{glm::inverse(camTrs)};
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0].x);
    glUniform3fv(camPosLoc, 1, &camTrs[3].x);
  }
  auto proj{glm::perspective(glm::radians(74.0f), 16.0f / 9.0f, 0.01f, 100.0f)};
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0].x);

  bool wtcUpdated{};
  bool shouldShowBounds{};
  bool pWasPressedLastFrame{};
  bool upWasPressedLastFrame{};
  bool downWasPressedLastFrame{};
  float dt{};

  glm::vec3 diffuse;
  glUniform1i(shadeLoc, 1);

  window.show();

  glm::vec3 globalY{glm::inverse(camTrs) * glm::vec4{0, 1, 0, 0}};

  glm::vec3 boundsColor{.5, .5, .5};

  auto updatePatch = [&](int index) {
    patchMeshes[index] = patches[index].tessellate(tessellationFactor);
    std::vector<AAB> boxes;
    allBoundsMeshes[index] = patches[index].GET_BOUND_MESHES(subdCount, &boxes);
    allBoundsBoxes[index] = std::move(boxes);
  };

  while (!window.shouldClose()) {
    auto start{std::chrono::steady_clock::now()};

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* User input */ {
      constexpr f32 factor{4};

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
          shouldShowBounds = !shouldShowBounds;
        pWasPressedLastFrame = true;
      } else {
        pWasPressedLastFrame = false;
      }

      if (window.keyIsPressed(GLFW_KEY_UP)) {
        for (auto &ctrlPt : patches[0].controlPoints()) {
          ctrlPt.y += 1 * dt;
        }
        updatePatch(0);
      }

      if (window.keyIsPressed(GLFW_KEY_DOWN)) {
        for (auto &ctrlPt : patches[0].controlPoints()) {
          ctrlPt.y -= 1 * dt;
        }
        updatePatch(0);
      }

      if (window.keyIsPressed(GLFW_KEY_LEFT)) {
        for (auto &ctrlPt : patches[0].controlPoints()) {
          ctrlPt.x -= 1 * dt;
        }
        updatePatch(0);
      }

      if (window.keyIsPressed(GLFW_KEY_RIGHT)) {
        for (auto &ctrlPt : patches[0].controlPoints()) {
          ctrlPt.x += 1 * dt;
        }
        updatePatch(0);
      }

      if (window.keyIsPressed(GLFW_KEY_ESCAPE))
        break;

      if (wtcUpdated) {
        auto view = glm::inverse(camTrs);
        globalY = view * glm::vec4{0, 1, 0, 0};
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0].x);
        glUniform3fv(camPosLoc, 1, &camTrs[3].x);
        wtcUpdated = false;
      }
    }

    constexpr glm::vec3 patchColors[] = {{224 / 255., 176 / 255., 74 / 255.},
                                         {74 / 255., 127 / 255., 224 / 255.},
                                         {224 / 255., 74 / 255., 150 / 255.},
                                         {74 / 255., 224 / 255., 74 / 255.}};
    constexpr auto patchColorCount = sizeof(patchColors) / sizeof(glm::vec3);

    MeshArray extraBoxes0[maxRecDepth + 1];
    MeshArray extraBoxes1[maxRecDepth + 1];

    // Collision
    bool didCollide = false;
    for (int i = 0, end = patches.size(); i < end; ++i) {
      for (int j = i; j < end; ++j) {
        auto &patch0 = patches[i], &patch1 = patches[j];
        auto &p0Boxes = allBoundsBoxes[i], &p1Boxes = allBoundsBoxes[j];

        for (int k = 0, boxCount0 = p0Boxes.size(); k < boxCount0; ++k) {
          auto kk = k / subdCount;
          auto k1 = k % subdCount;
          for (int l = 0, boxCount1 = p1Boxes.size(); l < boxCount1; ++l) {
            auto ll = l / subdCount;
            auto l1 = l % subdCount;
            if (p0Boxes[k].overlapsWith(p1Boxes[l])) {
              // collision detected

              if (i == j) {
                // self intersection, what to do? nothing for now
                // boundsColor = {0, .5, 1};
              } else {
                // TODO: refine each of the 2 boxes into 4 smaller ones up to
                // 32x32 (1024)
                //
                // maybe just split the AABBs themselves into 4 (cheaper)
                //
                // why not use a box for the patch and avoid AA? (IMPORTANT)
                // because AA guarantees that each box contains a part of the
                // surface. look up literature on bounds for patches

                // method parameters: maximum subdivision level (32x32), a least
                // reasonable subdivision level (8x8), maximum adaptive
                // subdivision level (32 / 4 = 8)

                // next steps:
                // [DONE] 1. implement on-the-fly adaptive subdivision process
                //           on the CPU, wasteful if necessary.
                //
                //        1.1. study this properly (refer to the paper).
                //
                //        2. make sure that generating boxes with AA is the best
                //           option (consider to be true for now).
                //
                //        3. maybe then implement an acceleration structure like
                //           a BVH.

                // change 1 to 2 if parametric space is [-1, 1] x [-1, 1]
                const f32 s = 1.0f / subdCount; // parametric square side length

                RecSubdArgs args{.p0 = patch0,
                                 .p1 = patch1,
                                 .u0 = {kk * s, (kk + 1) * s},
                                 .v0 = {k1 * s, (k1 + 1) * s},
                                 .u1 = {ll * s, (ll + 1) * s},
                                 .v1 = {l1 * s, (l1 + 1) * s},
                                 .b0 = extraBoxes0,
                                 .b1 = extraBoxes1,
                                 .depth = 0};

                subdivide(args);

                // boundsColor = {1, 0, 0};
                didCollide = true;
              }
            }
          }
        }
      }
      if (!didCollide)
        boundsColor = {.5, .5, .5};
    }

    // Visualization
    for (u64 i = 0, end = patchMeshes.size(); i < end; ++i) {
      // Patch visualization
      diffuse = patchColors[i % patchColorCount];
      glUniform3fv(diffuseLoc, 1, &diffuse.x);
      patchMeshes[i].draw();

      // AABB visualization
      if (shouldShowBounds) {
        diffuse = boundsColor;
        glUniform3fv(diffuseLoc, 1, &diffuse.x);

        // This is heavily unoptimized and should not be used for purposes
        // other than visualization. Currently, each box has its own separate
        // mesh, when they could all reference the same mesh with different
        // transformation matrices. Also, batch drawing would improve
        // performance, maybe by using something like glMultiDrawElements.
        for (auto &bvMesh : allBoundsMeshes[i])
          bvMesh.draw();
      }
    }

    for (int i = 0; i < maxRecDepth + 1; ++i) {
      float x = (1.0f / maxRecDepth) * i;
      diffuse = {x, 1 - x, 0};
      glUniform3fv(diffuseLoc, 1, &diffuse.x);

      glClear(GL_DEPTH_BUFFER_BIT);

      for (auto &boxMesh : extraBoxes0[i])
        boxMesh.draw();

      for (auto &boxMesh : extraBoxes1[i])
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