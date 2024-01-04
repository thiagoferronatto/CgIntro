#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "transformable_object.hpp"

struct Light : public TransformableObject {
  Light(std::string name);
  Light(vec3 color, float intensity = 1);
  Light(std::string name, vec3 color, float intensity = 1);
  Light(const Light &other);
  Light(Light &&other) noexcept;

  Light &operator=(const Light &other);
  Light &operator=(Light &&other) noexcept;

  vec3 color;
  float intensity;

private:
  static inline size_t _instances{};
};

#endif // LIGHT_HPP