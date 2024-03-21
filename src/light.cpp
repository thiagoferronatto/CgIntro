#include "light.hpp"

Light::Light(std::string name)
    : TransformableObject{std::move(name)}, color{1}, intensity{1} {}

Light::Light(vec3 color, float intensity)
    : TransformableObject{std::string{"light_"} + std::to_string(_instances++)},
      color{color}, intensity{intensity} {}

Light::Light(std::string name, vec3 color, float intensity)
    : TransformableObject{std::move(name)}, color{color}, intensity{intensity} {
}

Light::Light(const Light &other)
    : TransformableObject{other}, color{other.color},
      intensity{other.intensity} {}

Light::Light(Light &&other) noexcept
    : TransformableObject{std::move(other)}, color{std::move(other.color)},
      intensity{other.intensity} {}

Light &Light::operator=(const Light &other) {
  if (&other == this)
    goto skip;
  this->TransformableObject::operator=(other);
  _name = other._name;
  //_children = other._children;
  _transform = other._transform;
  color = other.color;
  intensity = other.intensity;
skip:
  return *this;
}

Light &Light::operator=(Light &&other) noexcept {
  if (&other == this)
    goto skip;
  _name = std::move(other._name);
  //_children = std::move(other._children);
  _transform = std::move(other._transform);
  color = std::move(other.color);
  intensity = other.intensity;
skip:
  return *this;
}
