#include "actor.hpp"

Actor::Actor(std::string name) : TransformableObject{std::move(name)} {}

Actor::Actor(std::string name, Material m)
    : TransformableObject{std::move(name)}, material{m} {}

Actor::Actor(const Actor &other)
    : TransformableObject{other}, material{other.material} {}

Actor::Actor(Actor &&other) noexcept
    : TransformableObject{std::move(other)},
      material{std::move(other.material)} {}

void Actor::translate(vec3 xyz) {
  this->TransformableObject::translate(xyz);
  // no need to rebound when simply translating
  _boundingBox.a += xyz;
  _boundingBox.b += xyz;
}

void Actor::rotate(vec3 euler) {
  this->TransformableObject::rotate(euler);
  bound();
}

void Actor::scale(vec3 xyz) {
  this->TransformableObject::scale(xyz);
  bound();
}

Actor &Actor::operator=(const Actor &other) {
  if (&other == this)
    goto skip;
  this->TransformableObject::operator=(other);
  material = other.material;
skip:
  return *this;
}

Actor &Actor::operator=(Actor &&other) noexcept {
  if (&other == this)
    goto skip;
  this->TransformableObject::operator=(std::move(other));
  material = std::move(other.material);
skip:
  return *this;
}
