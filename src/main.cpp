#include "dbvt_broadphase.hpp"

static void addDefaultLights(Scene &scene) {
  auto light{std::make_shared<Light>(vec3{1})};
  light->intensity = 100;
  light->translate({-10, 15, -10});
  scene.addLight(light);
  light = std::make_shared<Light>(vec3{1});
  light->intensity = 100;
  light->translate({-10, 15, 10});
  scene.addLight(light);
  light = std::make_shared<Light>(vec3{1});
  light->intensity = 100;
  light->translate({10, 15, -10});
  scene.addLight(light);
  light = std::make_shared<Light>(vec3{1});
  light->intensity = 100;
  light->translate({10, 15, 10});
  scene.addLight(light);
}

static void addDefaultObjects(Scene &scene) {
  constexpr int xdim{3}, ydim{3}, zdim{3};
  auto mesh{new TriangleMesh{TriangleMeshData::cube()}};

  for (int i = 0; i < xdim; ++i) {
    for (int j = 0; j < ydim; ++j) {
      for (int k = 0; k < zdim; ++k) {
        auto actor{new Actor{"asdf", mesh}};
        actor->material.Kd = {float(i + 1) / xdim, float(j + 1) / ydim,
                              float(k + 1) / zdim};
        actor->scale(vec3{0.75});
        actor->translate({i - 0.5f * xdim, 10 + j, k - 0.5f * zdim});
        actor->initializeRigidBody(1);
        scene.addActor(actor);
      }
    }
  }
  auto groundMesh{new TriangleMesh{TriangleMeshData::plane()}};
  auto ground{new Actor{"ground", groundMesh}};
  ground->scale({50, 1, 50});
  ground->translate({0, -3.5, 0});
  ground->initializeRigidBody(0);
  scene.addActor(ground);
}

static void addDefaultCamera(Scene &scene) {
  auto cam{std::make_shared<Camera>("cam_1", radians(74.0f), 16.0f / 9.0f, 0.1f,
                                    1e2f)};
  cam->translate({0, 10, 20});
  cam->rotate({radians(30.0), 0, 0});
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
  Window window{w, h, "VBAG 2"};

  auto defaultScene{makeDefaultScene()};

  // TODO: make broadphase scene-independent
  DbvtBroadphase broadphase{defaultScene};

  defaultScene.render(window, [&] { //
    broadphase.collide();
  });

  return 0;
}