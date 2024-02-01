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

  Actor &operator=(const Actor &other);
  Actor &operator=(Actor &&other) noexcept;

  Material material{};
};

#endif // ACTOR_HPP