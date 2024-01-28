#include "scene.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>

#include "glm/gtx/euler_angles.hpp"
#include "log.hpp"
#include "ppm.hpp"

// clang-format off
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
// clang-format on

void Scene::addCamera(std::shared_ptr<Camera> camera) {
  _cameras.push_back(camera);
  _addChildren(camera);
}

void Scene::addLight(std::shared_ptr<Light> light) {
  _lights.push_back(light);
  _addChildren(light);
}

const std::vector<std::shared_ptr<Actor>> &Scene::objects() const {
  return _actors;
}

void Scene::_addChildren(std::shared_ptr<Object> object) {
  for (auto child : object->children()) {
    if (auto actor{std::dynamic_pointer_cast<Actor>(child)})
      addActor<decltype(actor)::element_type>(actor);
    else if (auto camera{std::dynamic_pointer_cast<Camera>(child)})
      addCamera(camera);
    else if (auto light{std::dynamic_pointer_cast<Light>(child)})
      addLight(light);
  }
}

static void transferObjects(GLuint *&buffers, GLuint *&textures,
                            size_t &prevObjAmt, auto &objects) {
  using namespace std::chrono;

  auto start{steady_clock::now()};

  auto objAmt{objects.size()};

  // vertex positions, normals, indices, and uv
  constexpr size_t buffersPerObject{4};

  if (buffers) {
    glCheck(glDeleteBuffers(buffersPerObject * GLsizei(prevObjAmt), buffers));
    delete[] buffers;
  }
  buffers = new GLuint[buffersPerObject * objAmt];

  if (textures) {
    glCheck(glDeleteTextures(GLsizei(prevObjAmt), textures));
    delete[] textures;
  }
  textures = new GLuint[objAmt];

  // generating buffers for each object
  glCheck(glGenBuffers(buffersPerObject * GLsizei(objAmt), buffers));

  // generating 1 texture for each object (even if it remains unused)
  glCheck(glGenTextures(GLsizei(objAmt), textures));

  size_t i{};
  log("[INFO] Transferring scene data to GPU...\n");
  for (auto obj : objects) {
    if (auto mesh{std::dynamic_pointer_cast<TriangleMesh>(obj)}) {
      auto &v{mesh->vertices()}, &n{mesh->normals()};
      auto &uv{mesh->uv()};
      auto &t{mesh->triangles()};

      // transferring vertex positions to VRAM
      glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i]));
      glCheck(glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(vec3), v.data(),
                           GL_STATIC_DRAW));
      //{
      //  auto p = (vec3 *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
      //  for (int k = 0; k < v.size(); ++k)
      //    printf_s("v %.3f %.3f %.3f\n", p[k].x, p[k].y, p[k].z);
      //  glUnmapBuffer(GL_ARRAY_BUFFER);
      //}

      // transferring vertex normals to VRAM
      glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i + objAmt]));
      glCheck(glBufferData(GL_ARRAY_BUFFER, n.size() * sizeof(vec3), n.data(),
                           GL_STATIC_DRAW));
      //{
      //  auto p = (vec3 *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
      //  for (int k = 0; k < n.size(); ++k)
      //    printf_s("vn %.3f %.3f %.3f\n", p[k].x, p[k].y, p[k].z);
      //  glUnmapBuffer(GL_ARRAY_BUFFER);
      //}

      // transferring vertex UV coords to VRAM
      glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i + 2 * objAmt]));
      glCheck(glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(vec2), uv.data(),
                           GL_STATIC_DRAW));

      //{
      //  auto p = (vec2 *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
      //  for (int k = 0; k < uv.size(); ++k)
      //    printf_s("uv %.3f %.3f\n", p[k].x, p[k].y);
      //  glUnmapBuffer(GL_ARRAY_BUFFER);
      //}

      // transferring triangle vertex indices to VRAM
      glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[i + 3 * objAmt]));
      glCheck(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                           t.size() * sizeof(TriangleMesh::Triangle), t.data(),
                           GL_STATIC_DRAW));

      if (auto textureFile{mesh->material.map_Kd}; !textureFile.empty()) {
        PPM bah{textureFile};
        glCheck(glBindTexture(GL_TEXTURE_2D, textures[i]));
        glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                GL_MIRRORED_REPEAT));
        glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                GL_MIRRORED_REPEAT));
        glCheck(
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        glCheck(
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        glCheck(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
        glCheck(glTexImage2D( //
            GL_TEXTURE_2D,    // target
            0,                // level
            GL_RGB,           // internalformat
            bah.width(),      // width
            bah.height(),     // height
            0,                // border
            GL_RGB,           // format
            GL_UNSIGNED_BYTE, // type
            bah.pixels()      // data
            ));
        glCheck(glGenerateMipmap(GL_TEXTURE_2D));
      }
    }
    ++i;
  }
  prevObjAmt = objects.size();
  auto end{steady_clock::now()};
  log("[INFO] Data transfer complete, took %g ms\n",
      duration_cast<microseconds>(end - start).count() / 1e3f);
}

static void makeMainMenu(Scene *scene, const Window &window, GLuint *&buffers,
                         GLuint *&textures, size_t &prevObjAmt) {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::BeginMenu("Import")) {
        if (ImGui::BeginMenu("Wavefront (OBJ)")) {
          std::string path{"./assets/"};
          for (auto &entry : std::filesystem::directory_iterator{path}) {
            auto filePath{entry.path()};
            auto fileName{filePath.filename()};
            if (fileName.extension() == ".obj" &&
                ImGui::MenuItem(fileName.string().c_str())) {
              scene->addActor<TriangleMesh>(
                  TriangleMesh::fromObj(filePath.string()));
              transferObjects(buffers, textures, prevObjAmt, scene->objects());
            }
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::BeginMenu("Scene")) {
        if (ImGui::BeginMenu("Add premade actor")) {
          if (ImGui::MenuItem("Cube")) {
            scene->addActor(TriangleMesh::cube());
            transferObjects(buffers, textures, prevObjAmt, scene->objects());
          }
          if (ImGui::MenuItem("Plane")) {
            scene->addActor(TriangleMesh::plane());
            transferObjects(buffers, textures, prevObjAmt, scene->objects());
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      if (ImGui::BeginMenu("Render")) {
        ImGui::MenuItem("Wireframes", "", &scene->options.wireframe);
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void Scene::render(const Window &window, const std::function<void()> &f) {
  using namespace std::chrono;

  GLuint *buffers{}, *textures{};
  auto objAmt{_actors.size()};
  transferObjects(buffers, textures, objAmt, _actors);

  GLuint vao;
  glCheck(glGenVertexArrays(1, &vao));
  glCheck(glBindVertexArray(vao));

  log("[INFO] Starting rendering loop\n");
  window.show();
  while (!window.shouldClose()) {
    // GUI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    makeMainMenu(this, window, buffers, textures, objAmt);

    ImGui::SetNextWindowPos({0.75f * window.width(), 20});
    ImGui::SetNextWindowSize({0.25f * window.width(), 0.5f * window.height()});
    if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoResize)) {
      if (ImGui::BeginTabBar("scene_tabs")) {
        if (ImGui::BeginTabItem("Objects")) {
          if (ImGui::CollapsingHeader("Actors",
                                      ImGuiTreeNodeFlags_DefaultOpen)) {
            auto it{_actors.end()};
            for (auto &actor : _actors) {
              if (ImGui::MenuItem(actor->name().c_str()))
                _currentObject = actor;
              if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove")) {
                  if (_currentObject == actor)
                    _currentObject = nullptr;
                  it = std::find(_actors.begin(), _actors.end(), actor);
                  transferObjects(buffers, textures, objAmt, _actors);
                }
                ImGui::EndPopup();
              }
            }
            if (it != _actors.end()) {
              _actors.erase(it);
              transferObjects(buffers, textures, objAmt, _actors);
            }
          }
          if (ImGui::CollapsingHeader("Cameras",
                                      ImGuiTreeNodeFlags_DefaultOpen)) {
            auto it{_cameras.end()};
            for (auto &cam : _cameras) {
              if (ImGui::MenuItem(cam->name().c_str()))
                _currentObject = cam;
              if (_cameras.size() > 1) {
                if (ImGui::BeginPopupContextItem()) {
                  if (ImGui::MenuItem("Remove")) {
                    if (_currentObject == cam)
                      _currentObject = nullptr;
                    it = std::find(_cameras.begin(), _cameras.end(), cam);
                  }
                  ImGui::EndPopup();
                }
              }
            }
            if (it != _cameras.end())
              _cameras.erase(it);
          }
          if (ImGui::CollapsingHeader("Lights",
                                      ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Add light"))
              addLight(std::make_shared<Light>(vec3{1}));
            auto it{_lights.end()};
            for (auto light : _lights) {
              if (ImGui::MenuItem(light->name().c_str()))
                _currentObject = light;
              if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove")) {
                  if (_currentObject == light)
                    _currentObject = nullptr;
                  it = std::find(_lights.begin(), _lights.end(), light);
                }
                ImGui::EndPopup();
              }
            }
            if (it != _lights.end())
              _lights.erase(it);
          }
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Properties")) {
          if (ImGui::ColorEdit3("Ambient", &_ambient.x))
            glClearColor(_ambient.x, _ambient.y, _ambient.z, 1);
          ImGui::Checkbox("Desaturate bright colors", &options.desaturate);
          ImGui::Checkbox("Perform tone mapping", &options.toneMap);
          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }
    }
    ImGui::End();

    ImGui::SetNextWindowPos(
        {0.75f * window.width(), 0.5f * window.height() + 20});
    ImGui::SetNextWindowSize(
        {0.25f * window.width(), 0.5f * window.height() - 20});
    if (ImGui::Begin("Object properties", nullptr, ImGuiWindowFlags_NoResize)) {
      if (_currentObject) {
        ImGui::Text("Selected object: %s", _currentObject->name().c_str());
        if (ImGui::CollapsingHeader("Transform",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          auto pos{_currentObject->position()};
          if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
            auto invt{glm::inverse(_currentObject->transform())};
            _currentObject->translate(
                invt * vec4{pos - _currentObject->position(), 0});
            if (auto cam{std::dynamic_pointer_cast<Camera>(_currentObject)})
              cam->updateWorldToCamera();
          }
          vec3 rotation{_currentObject->rotation()};
          if (ImGui::DragFloat3("Rotation", &rotation.x, 0.01f, -1e3, 1e3)) {
            auto tmp{rotation - _currentObject->rotation()};
            auto scale{_currentObject->scale()};
            _currentObject->setScale({1, 1, 1});
            auto invt{glm::inverse(_currentObject->transform())};
            _currentObject->rotate(tmp.x, invt * vec4{1, 0, 0, 0});
            _currentObject->rotate(tmp.y, invt * vec4{0, 1, 0, 0});
            _currentObject->rotate(tmp.z, invt * vec4{0, 0, 1, 0});
            _currentObject->setScale(scale);
            if (auto cam{std::dynamic_pointer_cast<Camera>(_currentObject)})
              cam->updateWorldToCamera();
          }
          vec3 scale{_currentObject->scale()};
          if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.001f, 1e5f)) {
            _currentObject->setScale(scale);
          }
        }

        if (auto actor{std::dynamic_pointer_cast<Actor>(_currentObject)}) {
          if (ImGui::CollapsingHeader("Material",
                                      ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Ambient (Ka)", &actor->material.Ka.x);
            ImGui::ColorEdit3("Diffuse (Kd)", &actor->material.Kd.x);
            ImGui::ColorEdit3("Diffuse (Ks)", &actor->material.Ks.x);
            ImGui::DragFloat("Shininess (Ns)", &actor->material.Ns, 0.1, 0);
          }
        }
        if (auto light{std::dynamic_pointer_cast<Light>(_currentObject)}) {
          if (ImGui::CollapsingHeader("Light properties",
                                      ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Color", &light->color.x);
            ImGui::DragFloat("Intensity", &light->intensity, 0.1f, 0, 1e10);
          }
        }
        if (auto cam{std::dynamic_pointer_cast<Camera>(_currentObject)}) {
          if (ImGui::CollapsingHeader("Camera properties",
                                      ImGuiTreeNodeFlags_DefaultOpen)) {
            if (auto fov{cam->fov()};
                ImGui::SliderFloat("FOV", &fov, 1, 179, "%.3f deg")) {
              cam->setFov(fov);
              cam->updatePerspective();
            }
            if (auto aspect{cam->aspect()};
                ImGui::SliderFloat("Aspect ratio", &aspect, 0.001, 10)) {
              cam->setAspect(aspect);
              cam->updatePerspective();
            }
            if (auto near{cam->near()};
                ImGui::DragFloat("Near plane", &near, 0.1, 0, cam->far())) {
              cam->setNear(near);
              cam->updatePerspective();
            }
            if (auto far{cam->far()};
                ImGui::DragFloat("Far plane", &far, 0.1, cam->near(), 1e5)) {
              cam->setFar(far);
              cam->updatePerspective();
            }
          }
        }
      }
    }
    ImGui::End();

    ImGui::SetNextWindowPos({0, 0.75f * window.height()});
    ImGui::SetNextWindowSize({0.75f * window.width(), 0.25f * window.height()});
    if (ImGui::Begin("log", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_AlwaysVerticalScrollbar |
                         ImGuiWindowFlags_NoResize)) {
      ImGui::Text(globalLog.c_str());
    }
    ImGui::End();

    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                    GL_STENCIL_BUFFER_BIT));

    // DEBUG CONTROLS
    if (window.keyIsPressed(GLFW_KEY_ESCAPE))
      _cameras[0]->translate(-_cameras[0]->transform()[3]);
    if (window.keyIsPressed(GLFW_KEY_W))
      _cameras[0]->translate({0, 0, -0.1});
    if (window.keyIsPressed(GLFW_KEY_A))
      _cameras[0]->translate({-0.1, 0, 0});
    if (window.keyIsPressed(GLFW_KEY_S))
      _cameras[0]->translate({0, 0, 0.1});
    if (window.keyIsPressed(GLFW_KEY_D))
      _cameras[0]->translate({0.1, 0, 0});
    if (window.keyIsPressed(GLFW_KEY_SPACE))
      _cameras[0]->translate({0, 0.1, 0});
    if (window.keyIsPressed(GLFW_KEY_LEFT_CONTROL))
      _cameras[0]->translate({0, -0.1, 0});
    if (window.keyIsPressed(GLFW_KEY_Q))
      _cameras[0]->rotate(glm::radians(1.0f), {0, 1, 0});
    if (window.keyIsPressed(GLFW_KEY_E))
      _cameras[0]->rotate(glm::radians(-1.0f), {0, 1, 0});
    if (window.keyIsPressed(GLFW_KEY_R))
      _cameras[0]->rotate(glm::radians(1.0f), {1, 0, 0});
    if (window.keyIsPressed(GLFW_KEY_F))
      _cameras[0]->rotate(glm::radians(-1.0f), {1, 0, 0});
    if (window.keyIsPressed(GLFW_KEY_Z))
      _cameras[0]->setFov(fmaxf(_cameras[0]->fov() - 1.0f, 0.1));
    if (window.keyIsPressed(GLFW_KEY_C))
      _cameras[0]->setFov(fminf(_cameras[0]->fov() + 1.0f, 179));
    // END OF DEBUG CONTROLS

    unsigned i = 0;
    for (auto obj : _actors) {
      // adding VBOs to VAO
      glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i]));
      glCheck(glEnableVertexAttribArray(0));
      glCheck(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
      glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i + objAmt]));
      glCheck(glEnableVertexAttribArray(1));
      glCheck(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
      if (!obj->material.map_Kd.empty()) {
        glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i + 2 * objAmt]));
        glCheck(glEnableVertexAttribArray(2));
        glCheck(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr));
        glCheck(glBindTexture(GL_TEXTURE_2D, textures[i]));
      }

      // adding EBO to VAO
      glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[i + 3 * objAmt]));

      if (auto mesh{std::dynamic_pointer_cast<TriangleMesh>(obj)}) {
        // vertex shader
        auto vsLoc{glCreateShader(GL_VERTEX_SHADER)};
        glCheck(glShaderSource(vsLoc, 1, &triangleMeshVertexShader, nullptr));
        glCheck(glCompileShader(vsLoc));
        glCheckShaderCompilation(vsLoc);

        // fragment shader
        auto fsLoc{glCreateShader(GL_FRAGMENT_SHADER)};
        auto fragShader{triangleMeshFragShader(_lights.size())};
        auto bah = fragShader.c_str();
        glCheck(glShaderSource(fsLoc, 1, &bah, nullptr));
        glCheck(glCompileShader(fsLoc));
        glCheckShaderCompilation(fsLoc);

        // program
        auto program{glCreateProgram()};
        glCheck(glAttachShader(program, vsLoc));
        glCheck(glAttachShader(program, fsLoc));
        glCheck(glLinkProgram(program));
        glCheckProgramLinkage(program);
        glCheck(glUseProgram(program));

        // uniforms
        auto mLoc{glGetUniformLocation(program, "M")};
        glCheck(glUniformMatrix4fv(mLoc, 1, GL_FALSE, &mesh->transform()[0].x));
        auto vLoc{glGetUniformLocation(program, "V")};
        glCheck(glUniformMatrix4fv(vLoc, 1, GL_FALSE,
                                   &_cameras[0]->worldToCamera()[0].x));
        auto pLoc{glGetUniformLocation(program, "P")};
        glCheck(glUniformMatrix4fv(pLoc, 1, GL_FALSE,
                                   &_cameras[0]->perspective()[0].x));
        auto materialKaLoc{glGetUniformLocation(program, "material.Ka")};
        glCheck(glUniform3fv(materialKaLoc, 1, &mesh->material.Ka.x));
        auto materialKdLoc{glGetUniformLocation(program, "material.Kd")};
        glCheck(glUniform3fv(materialKdLoc, 1, &mesh->material.Kd.x));
        auto materialKsLoc{glGetUniformLocation(program, "material.Ks")};
        glCheck(glUniform3fv(materialKsLoc, 1, &mesh->material.Ks.x));
        auto materialNsLoc{glGetUniformLocation(program, "material.Ns")};
        glCheck(glUniform1f(materialNsLoc, mesh->material.Ns));
        auto materialNiLoc{glGetUniformLocation(program, "material.Ni")};
        glCheck(glUniform1f(materialNiLoc, mesh->material.Ni));
        auto materialDLoc{glGetUniformLocation(program, "material.d")};
        glCheck(glUniform1f(materialDLoc, mesh->material.d));
        auto ambientLoc{glGetUniformLocation(program, "ambient")};
        glCheck(glUniform3fv(ambientLoc, 1, &_ambient.x));

        // UI state and rendering options
        auto selectedLoc{glGetUniformLocation(program, "selected")};
        glUniform1i(selectedLoc, GLint(std::dynamic_pointer_cast<TriangleMesh>(
                                           _currentObject) == mesh));
        auto toneMapLoc{glGetUniformLocation(program, "toneMap")};
        glCheck(glUniform1i(toneMapLoc, GLint(options.toneMap)));
        auto wireframeLoc{glGetUniformLocation(program, "wireframe")};
        glCheck(glUniform1i(wireframeLoc, GLint(options.wireframe)));
        auto desaturateLoc{glGetUniformLocation(program, "desaturate")};
        glCheck(glUniform1i(desaturateLoc, GLint(options.desaturate)));
        auto texturedLoc{glGetUniformLocation(program, "textured")};
        glCheck(
            glUniform1i(texturedLoc, GLint(!mesh->material.map_Kd.empty())));

        unsigned l{};
        for (auto &light : _lights) {
          auto lightColorLoc{glGetUniformLocation(
              program, (std::string{"lights["} + std::to_string(l) + "].color")
                           .c_str())};
          auto trueLightColor{light->intensity * light->color};
          glCheck(glUniform3fv(lightColorLoc, 1, &trueLightColor.x));
          auto lightTransformLoc{glGetUniformLocation(
              program,
              (std::string{"lights["} + std::to_string(l) + "].transform")
                  .c_str())};
          glCheck(glUniformMatrix4fv(lightTransformLoc, 1, GL_FALSE,
                                     &light->transform()[0].x));
          ++l;
        }

        auto lightCountLoc{glGetUniformLocation(program, "lightCount")};
        glUniform1ui(lightCountLoc, l);

        // setting up for the draw call
        glCheck(glEnable(GL_DEPTH_TEST));
        glCheck(glPolygonMode(GL_FRONT_AND_BACK,
                              options.wireframe ? GL_LINE : GL_FILL));

        // drawing elements
        glCheck(glDrawElements(GL_TRIANGLES,
                               3 * GLsizei(mesh->triangles().size()),
                               GL_UNSIGNED_INT, nullptr));

        // calling custom loop function after drawing
        f();

      } else {
        log("[WARNING] Attempted to draw unsupported shape, ignoring "
            "request\n");
      }
      ++i;
    }

    // GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    window.swapBuffers();
    window.pollEvents();
  }
  log("[INFO] Rendering loop ended\n");
  log("[INFO] Freeing GPU memory\n");

  glCheck(glDeleteBuffers(3 * GLsizei(objAmt), buffers));
  log("[INFO] Done\n");
  log("[INFO] Freeing CPU memory\n");
  delete[] buffers;
  log("[INFO] Done\n");
}