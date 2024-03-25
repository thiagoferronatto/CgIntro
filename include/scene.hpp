#ifndef SCENE_HPP
#define SCENE_HPP

#include <map>
#include <memory>
#include <stdexcept>

#include "actor.hpp"
#include "camera.hpp"
#include "custom_assert.hpp"
#include "gl_util.hpp"
#include "light.hpp"
#include "shader_sources.hpp"
#include "window.hpp"

class Scene {
public:
  void addActor(Actor *actor);
  void addCamera(std::shared_ptr<Camera> camera);
  void addLight(std::shared_ptr<Light> light);

  void render(
      const Window &window, const std::function<void()> &f = [] {});

  const std::vector<Actor *> &actors() const;
  const std::vector<std::shared_ptr<Light>> &lights() const;

  struct Options {
    bool toneMap{}, wireframe{}, desaturate{};
  } options;

  vec3 ambient{};

  float dt() const { return _dt; }

private:
  void _addChildren(std::shared_ptr<Object> object);

  std::vector<std::shared_ptr<Camera>> _cameras;
  std::vector<Actor *> _actors;
  std::vector<std::shared_ptr<Light>> _lights;
  TransformableObject *_currentObject;
  float _dt{};
};

#endif // SCENE_HPP