#ifndef ACTOR_HPP
#define ACTOR_HPP

#include "material.hpp"
#include "rigid_body.hpp"
#include "transformable_object.hpp"

class Actor : public TransformableObject, public RigidBody {
public:
  Actor(std::string name);
  Actor(std::string name, Material m);
  Actor(const Actor &other);
  Actor(Actor &&other) noexcept;

  void translate(vec3 xyz) override;
  void rotate(vec3 euler) override;
  void scale(vec3 xyz) override;

  Actor &operator=(const Actor &other);
  Actor &operator=(Actor &&other) noexcept;

  Material material{};
};

#endif // ACTOR_HPP