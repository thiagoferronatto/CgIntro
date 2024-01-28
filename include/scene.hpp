#ifndef SCENE_HPP
#define SCENE_HPP

#include <map>
#include <memory>
#include <stdexcept>

#include "camera.hpp"
#include "custom_assert.hpp"
#include "gl_util.hpp"
#include "light.hpp"
#include "shader_sources.hpp"
#include "triangle_mesh.hpp"
#include "window.hpp"

class Scene {
public:
  template <typename T> void addActor(std::shared_ptr<T> object);
  void addCamera(std::shared_ptr<Camera> camera);
  void addLight(std::shared_ptr<Light> light);

  void render(
      const Window &window, const std::function<void()> &f = [] {});

  const std::vector<std::shared_ptr<Actor>> &objects() const;
  const std::vector<std::shared_ptr<Light>> &lights() const;

  struct Options {
    bool toneMap{}, wireframe{}, desaturate{true};
  } options;

  vec3 ambient{};

private:
  void _addChildren(std::shared_ptr<Object> object);

  std::vector<std::shared_ptr<Camera>> _cameras;
  std::vector<std::shared_ptr<Actor>> _actors;
  std::vector<std::shared_ptr<Light>> _lights;
  std::shared_ptr<TransformableObject> _currentObject;
  vec3 _ambient{};
};

template <typename T> void Scene::addActor(std::shared_ptr<T> object) {
  ASSERT(std::dynamic_pointer_cast<Actor>(object),
         "type \"%s\" is not an actor",
         typeid(decltype(object)::element_type).name());
  _actors.push_back(object);
  _addChildren(object);
}

#endif // SCENE_HPP