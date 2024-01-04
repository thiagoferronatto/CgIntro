#include "custom_assert.hpp"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "scene.hpp"
#include "window.hpp"

int main() {
  constexpr size_t w{1600}, h{900};
  Window window{w, h, "window"};

  Scene scene;

  auto mesh = TriangleMesh::cube();
  scene.addActor<TriangleMesh>(mesh);

  auto cam{std::make_shared<Camera>("cam_1", glm::radians(74.0f),
                                    float(w) / float(h), 0.1f, 1000.0f)};
  auto light{std::make_shared<Light>(vec3{1})};
  light->intensity = 10;
  cam->addChild(light);
  cam->translate({0, 0, 5});

  scene.addCamera(cam);

  scene.render(window);

  return 0;
}