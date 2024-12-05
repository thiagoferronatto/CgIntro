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
  std::vector<glm::tvec3<size_t>> triangles;
};

struct ControlPointData {
  std::function<float(float, float)> b;     // basis function b
  std::function<float(float, float)> dbdu;  // partial derivative of b wrt u
  std::function<float(float, float)> dbdv;  // partial derivative of b wrt v
  glm::vec3 p;                              // position of the control point
};

float bezierBasisFunction1d(float t) {
  auto tc{1 - t};
  return std::pow(tc, n - k - 1) * std::pow(t, k);
}

float bezierBasisFunction2d(float u, float v) {}

auto tessellate(auto&& controlPoints, auto&& level) {
  TriangleMesh mesh;
  auto stepSize{1.0f / level};
  float u{};
  float v{};
  for (size_t i{}; i <= level; ++i, u += stepSize) {
    for (size_t j{}; j <= level; ++j, v += stepSize) {
      glm::vec3 vertexPosition{};
      glm::vec3 uTangent{};
      glm::vec3 vTangent{};
      for (auto&& [b, dbdu, dbdv, p] : controlPoints) {
        vertexPosition += b(u, v) * p;
        uTangent += dbdu(u, v) * p;
        vTangent += dbdv(u, v) * p;
      }
      mesh.vertices.emplace_back(std::move(vertexPosition));
      auto vertexNormal{glm::normalize(glm::cross(uTangent, vTangent))};
      mesh.normals.emplace_back(std::move(vertexNormal));
    }
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

  auto mesh = QuadMesh{};
  mesh.vertices = {
      {0, 1, 0},  {1, 0, 0},  {2, 0, 0},  {3, 1, 0},   //
      {0, 0, -1}, {1, 2, -1}, {2, 2, -1}, {3, 0, -1},  //
      {0, 0, -2}, {1, 2, -2}, {2, 2, -2}, {3, 0, -2},  //
      {0, 1, -3}, {1, 0, -3}, {2, 0, -3}, {3, 1, -3},
  };
  glm::vec3 center{};
  for (const auto& vertex : mesh.vertices) center += vertex;
  center /= mesh.vertices.size();

  for (auto& vertex : mesh.vertices) vertex = 0.25f * (vertex - center);

  mesh.indices = {0,  1,  2,  3,   //
                  4,  5,  6,  7,   //
                  8,  9,  10, 11,  //
                  12, 13, 14, 15};

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(glm::vec3),
               mesh.vertices.data(), GL_STATIC_DRAW);

  GLuint vao;
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);

  GLuint ebo;
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(GLuint),
               mesh.indices.data(), GL_STATIC_DRAW);

  constexpr auto vsSrc{R"(
    #version 460

    layout (location = 0) in vec3 position;

    void main(void) {
      gl_Position = vec4(position, 1);
    }
  )"};

  constexpr auto tcsSrc{R"(
    #version 460

    layout (vertices = 16) out;

    uniform float tessLevel;

    void main(void) {
      gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

      gl_TessLevelOuter[0] = tessLevel;
      gl_TessLevelOuter[1] = tessLevel;
      gl_TessLevelOuter[2] = tessLevel;
      gl_TessLevelOuter[3] = tessLevel;

      gl_TessLevelInner[0] = tessLevel;
      gl_TessLevelInner[1] = tessLevel;
    }
  )"};

  constexpr auto tesSrc{R"(
    #version 460

    layout (quads) in;

    uniform mat4 view;
    uniform mat4 proj;
    uniform int selectedControlPoint;

    out vec3 tesNormal;
    out vec3 tesPosition;
    out vec2 fragUv;

    vec3 position(float u, float v, out vec3 normal) {
      float B[4];
      B[0] = B[3] = 1;
      B[1] = B[2] = 3;

      float u2 = u * u, u3 = u2 * u;
      float uc = 1 - u, uc2 = uc * uc, uc3 = uc2 * uc;
      float v2 = v * v, v3 = v2 * v;
      float vc = 1 - v, vc2 = vc * vc, vc3 = vc2 * vc;
      float U[4] = float[4](1, u, u2, u3);
      float DU[4] = float[4](0, 1, 2 * u, 3 * u2);
      float UC[4] = float[4](1, uc, uc2, uc3);
      float DUC[4] = float[4](0, -1, 2 * u - 2, -3 * uc2);
      float V[4] = float[4](1, v, v2, v3);
      float DV[4] = float[4](0, 1, 2 * v, 3 * v2);
      float VC[4] = float[4](1, vc, vc2, vc3);
      float DVC[4] = float[4](0, -1, 2 * v - 2, -3 * vc2);

      vec3 s = vec3(0);
      vec3 du = vec3(0);
      vec3 dv = vec3(0);
      for (int i = 0; i < 4; ++i) {
        float BU = B[i] * U[i] * UC[3 - i];
        float DBU = B[i] * (DU[i] * UC[3 - i] + U[i] * DUC[3 - i]);
        for (int j = 0; j < 4; ++j) {
          float BV = B[j] * V[j] * VC[3 - j];
          float DBV = B[j] * (DV[j] * VC[3 - j] + V[j] * DVC[3 - j]);
          vec3 p = gl_in[i * 4 + j].gl_Position.xyz;
          s += BU * BV * p;
          du += DBU * BV * p;
          dv += BU * DBV * p;
        }
      }

      normal = normalize(cross(dv, du));
      
      return s;
    }
    
    void main(void) {
      float u = gl_TessCoord.x, v = gl_TessCoord.y;
      
      vec3 p_uv = position(u, v, tesNormal);

      tesPosition = p_uv;

      ivec2 selectedIj;
      selectedIj.x = selectedControlPoint / 4;
      selectedIj.y = selectedControlPoint % 4;
      vec2 selectedUv = (1.0 / 3.0) * vec2(selectedIj);
      float d = distance(gl_TessCoord.xy, selectedUv) / sqrt(2);
      //diffuse = normalize(mix(vec3(1, 0.5, 0), vec3(0, 0.5, 1), pow(d, 0.25)));
      fragUv = vec2(u, v);

      

      gl_Position = proj * view * vec4(p_uv, 1);
      //const int gridSize = 64;
      //const float factor = 1.0 / float(gridSize);
      //gl_Position.xyz = factor * floor(gridSize * gl_Position.xyz);
      //tesNormal.xyz = factor * floor(gridSize * tesNormal.xyz);
    }
  )"};

  constexpr auto fsSrc{R"(
    #version 460

    uniform mat4 view;
    uniform vec3 camPos;
    uniform float aa;
    uniform float bb;
    uniform float cc;

    in vec3 tesNormal;
    in vec3 tesPosition;
    in vec2 fragUv;

    out vec4 fragColor;

    void main(void) {
      vec3 normal = normalize(tesNormal);

      const float ambient = 0.1;

      vec3 lights[6];
      lights[0] = vec3( 0, 3.16,  0);
      //lights[1] = vec3( 0,  7,  0);
      //lights[2] = vec3( 7,  0,  0);
      //lights[3] = vec3(-7,  0,  0);
      //lights[4] = vec3( 0,  0,  7);
      //lights[5] = vec3( 0,  0, -7);
      
      //vec3 camPos = vec3(inverse(view) * vec4(0, 0, 0, 1));
      vec3 v = camPos - tesPosition;
      float camDist = length(v);
      v /= camDist;

      vec3 diffuse = vec3(1);

      const float pi = 3.1415926535;
      const float twoPi = 2.0 * pi;
      const float invTwoPi = 1.0 / twoPi;
      const float r = 0.35355339059;

      const int n = 2000;
      const float dt = 1 / float(n);
      float t = 0;
      
      vec2 uv = fragUv - 0.5;

      float minD2 = 2;

      const float bbPi = bb * pi;

      float sinTwoPiAa = sin(twoPi * aa);
      float sinTwoPiCc = sin(twoPi * cc);

      for (int i = 0; i < n; ++i, t += dt) {
        float u_ = r * sinTwoPiCc * cos(twoPi * t); // u(t)
        float v_ = r * sinTwoPiAa * sin(bbPi * t);  // v(t)
        vec2 C = vec2(u_, v_); // C(t)
        vec2 tmp = uv - C;
        float d2 = dot(tmp, tmp);
        if (d2 < minD2)
          minD2 = d2;
      }

      bool isOnCurve = minD2 < 0.00002;

      if (isOnCurve) diffuse -= 0.1;

      vec3 color = ambient * diffuse;

      for (int i = 0; i < 1; ++i) {
        vec3 lPos = lights[i];
        
        vec3 l = lPos - tesPosition;
        float lightDist = length(l);
        l /= lightDist;
        float invd = 1.0 / (camDist + lightDist);
        float d = 0.5 * dot(normal, l) + 0.5;
        d *= d;

        vec3 h = normalize(v + l);
        const float alpha = 50.0;
        float s = !isOnCurve ? 0 : pow(max(dot(normal, h), 0.0), alpha);

        color += min(invd * invd, 1.0) * 10.0 * (d * diffuse + s);
      }

      fragColor = vec4(color, 1);
    }
  )"};

  auto vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vsSrc, nullptr);
  glCompileShader(vs);
  glCheckShaderCompilation(vs);

  auto tcs = glCreateShader(GL_TESS_CONTROL_SHADER);
  glShaderSource(tcs, 1, &tcsSrc, nullptr);
  glCompileShader(tcs);
  glCheckShaderCompilation(tcs);

  auto tes = glCreateShader(GL_TESS_EVALUATION_SHADER);
  glShaderSource(tes, 1, &tesSrc, nullptr);
  glCompileShader(tes);
  glCheckShaderCompilation(tes);

  auto fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fsSrc, nullptr);
  glCompileShader(fs);
  glCheckShaderCompilation(fs);

  auto program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, tcs);
  glAttachShader(program, tes);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glCheckProgramLinkage(program);

  glUseProgram(program);

  auto tessLevelLoc = glGetUniformLocation(program, "tessLevel");
  auto viewLoc = glGetUniformLocation(program, "view");
  auto projLoc = glGetUniformLocation(program, "proj");
  auto selectedControlPointLoc =
      glGetUniformLocation(program, "selectedControlPoint");
  auto camPosLoc = glGetUniformLocation(program, "camPos");
  auto aaLoc = glGetUniformLocation(program, "aa");
  auto bbLoc = glGetUniformLocation(program, "bb");
  auto ccLoc = glGetUniformLocation(program, "cc");

  glPatchParameteri(GL_PATCH_VERTICES, 16);

  // glEnable(GL_CULL_FACE);
  // glCullFace(GL_BACK);

  // auto camTrs = glm::identity<glm::mat4>();
  auto camTrs = glm::inverse(glm::lookAt(glm::vec3{0, 0.5, 1}, {}, {0, 1, 0}));
  // camTrs[3] = {0, 1, 1, 1};
  {
    auto view = glm::inverse(camTrs);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0].x);
    glUniform3fv(camPosLoc, 1, &camTrs[3].x);
  }
  auto proj =
      glm::perspective(glm::radians(74.0f), 16.0f / 9.0f, 0.01f, 100.0f);
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0].x);

  glEnable(GL_DEPTH_TEST);

  glClearColor(0.1, 0.1, 0.1, 1);

  glCullFace(GL_FRONT);
  glEnable(GL_CULL_FACE);

  bool wtcUpdated = false, vertexBufferNeedsUpdating = false;
  bool upWasPressedLastFrame = false;
  bool downWasPressedLastFrame = false;
  bool leftWasPressedLastFrame = false;
  bool rightWasPressedLastFrame = false;

  int selectedControlPoint = 0;

  float tessLevel = 64;
  glUniform1f(tessLevelLoc, tessLevel);

  float dt = 0;
  float time = 0;
  float aa = .25;
  float bb = 2;
  float cc = .25;

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
      if (window.keyIsPressed('I')) {
        mesh.vertices[selectedControlPoint].z -= factor * dt;
        vertexBufferNeedsUpdating = true;
      }
      if (window.keyIsPressed('J')) {
        mesh.vertices[selectedControlPoint].x -= factor * dt;
        vertexBufferNeedsUpdating = true;
      }
      if (window.keyIsPressed('K')) {
        mesh.vertices[selectedControlPoint].z += factor * dt;
        vertexBufferNeedsUpdating = true;
      }
      if (window.keyIsPressed('L')) {
        mesh.vertices[selectedControlPoint].x += factor * dt;
        vertexBufferNeedsUpdating = true;
      }
      if (window.keyIsPressed('N')) {
        aa -= factor * dt;
      }
      if (window.keyIsPressed('M')) {
        aa += factor * dt;
      }
      if (window.keyIsPressed('U')) {
        bb -= factor * dt;
      }
      if (window.keyIsPressed('O')) {
        bb += factor * dt;
      }
      if (window.keyIsPressed('Y')) {
        cc += factor * dt;
      }
      if (window.keyIsPressed('H')) {
        cc -= factor * dt;
      }

      bool utmp, dtmp, ltmp, rtmp;
      if ((utmp = window.keyIsPressed(GLFW_KEY_UP)) && !upWasPressedLastFrame) {
        auto next = selectedControlPoint + 4;
        selectedControlPoint = next >= 16 ? next - 16 : next;
      }
      upWasPressedLastFrame = utmp;

      if ((dtmp = window.keyIsPressed(GLFW_KEY_DOWN)) &&
          !downWasPressedLastFrame) {
        auto next = selectedControlPoint - 4;
        selectedControlPoint = next < 0 ? 16 + next : next;
      }
      downWasPressedLastFrame = dtmp;

      if ((ltmp = window.keyIsPressed(GLFW_KEY_LEFT)) &&
          !leftWasPressedLastFrame) {
        auto next = selectedControlPoint - 1;
        selectedControlPoint = next < 0 ? 16 + next : next;
      }
      leftWasPressedLastFrame = ltmp;

      if ((rtmp = window.keyIsPressed(GLFW_KEY_RIGHT)) &&
          !rightWasPressedLastFrame) {
        auto next = selectedControlPoint + 1;
        selectedControlPoint = next >= 16 ? next - 16 : next;
      }
      rightWasPressedLastFrame = rtmp;

      if (window.keyIsPressed(GLFW_KEY_ESCAPE)) break;

      if (wtcUpdated) {
        auto view = glm::inverse(camTrs);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0].x);
        glUniform3fv(camPosLoc, 1, &camTrs[3].x);
        wtcUpdated = false;
      }
      glUniform1i(selectedControlPointLoc, selectedControlPoint);
    }

    if (vertexBufferNeedsUpdating) {
      glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(glm::vec3),
                   mesh.vertices.data(), GL_STATIC_DRAW);
      vertexBufferNeedsUpdating = false;
    }

    glDrawElements(GL_PATCHES, mesh.indices.size(), GL_UNSIGNED_INT, nullptr);

    window.swapBuffers();
    window.pollEvents();

    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    auto msDiff = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
    auto msDiffCount = msDiff.count();
    dt = 0.001f * msDiffCount;
    // time = time + dt > 1 ? 0 : time + dt;
    time += dt;
    glUniform1f(aaLoc, aa);
    glUniform1f(bbLoc, bb);
    glUniform1f(ccLoc, cc);
  }

  return 0;
}