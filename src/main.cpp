#include "dbvt_broadphase.hpp"

static void addDefaultLights(Scene &scene) {
  auto light{std::make_shared<Light>(vec3{1})};
  light->intensity = 50;
  light->translate({-10, 5, -10});
  scene.addLight(light);
  light = std::make_shared<Light>(vec3{1});
  light->intensity = 50;
  light->translate({-10, 5, 10});
  scene.addLight(light);
  light = std::make_shared<Light>(vec3{1});
  light->intensity = 50;
  light->translate({10, 5, -10});
  scene.addLight(light);
  light = std::make_shared<Light>(vec3{1});
  light->intensity = 50;
  light->translate({10, 5, 10});
  scene.addLight(light);
}

static void addDefaultObjects(Scene &scene) {
  auto meshData{TriangleMeshData::cube()};

  constexpr int xdim{3}, ydim{3}, zdim{3};

  for (int i = 0; i < xdim; ++i) {
    for (int j = 0; j < ydim; ++j) {
      for (int k = 0; k < zdim; ++k) {
        auto mesh{std::make_shared<TriangleMesh>(
            "rigid_body_" + std::to_string(9 * i + 3 * j + k), meshData)};
        mesh->material.Kd = {1, 0, 0};
        mesh->scale(vec3{0.75});
        mesh->translate({i - 0.5f * xdim, 10 + j, k - 0.5f * zdim});
        mesh->initializeRigidBody(1);
        scene.addActor(mesh);
      }
    }
  }

  auto ground =
      std::make_shared<TriangleMesh>("ground", TriangleMeshData::plane());
  ground->scale({50, 1, 50});
  ground->translate({0, -3.5, 0});
  ground->initializeRigidBody(0);
  scene.addActor(ground);
}

static void addDefaultCamera(Scene &scene) {
  auto cam{std::make_shared<Camera>("cam_1", radians(74.0f), 16.0f / 9.0f, 1.0f,
                                    100.0f)};
  cam->translate({0, 15, 20});
  cam->rotate({radians(45.0f), 0, 0});
  cam->translate(vec3{cam->transform() * vec4{0, 0, -16, 0}});
  scene.addCamera(cam);
}

static Scene makeDefaultScene() {
  Scene scene;
  scene.ambient = {};
  addDefaultObjects(scene);
  addDefaultLights(scene);
  addDefaultCamera(scene);
  return scene;
}

int main() {
  constexpr size_t w{1600}, h{900};
  Window window{w, h, "VBAG 2.0"};

  auto scene{makeDefaultScene()};
  // TODO: make broadphase scene-independent
  DbvtBroadphase broadphase{scene};

  scene.render(window, [&] { broadphase.collide(); });

  return 0;
}