#include "transformable_object.hpp"

#include "glm/gtx/euler_angles.hpp"

TransformableObject::TransformableObject(std::string name)
    : Object{std::move(name)}, _transform{glm::identity<glm::mat4>()} {}

TransformableObject::TransformableObject(const TransformableObject &other)
    : Object{other}, _transform{other._transform} {}

TransformableObject::TransformableObject(TransformableObject &&other) noexcept
    : Object{std::move(other)}, _transform{std::move(other._transform)} {}

TransformableObject &
TransformableObject::operator=(const TransformableObject &other) {
  if (&other == this)
    goto skip;
  this->Object::operator=(other);
  _transform = other._transform;
skip:
  return *this;
}

TransformableObject &
TransformableObject::operator=(TransformableObject &&other) noexcept {
  if (&other == this)
    goto skip;
  this->Object::operator=(std::move(other));
  _transform = std::move(other._transform);
skip:
  return *this;
}

void TransformableObject::translate(vec3 xyz) {
  _transform = glm::translate(_transform, xyz);
  for (auto child : _children)
    if (auto transChild{std::dynamic_pointer_cast<TransformableObject>(child)})
      transChild->translate(xyz);
}

void TransformableObject::rotate(float angle, vec3 axis) {
  vec3 s{scale()};
  _transform[0] /= s.x;
  _transform[1] /= s.y;
  _transform[2] /= s.z;
  _transform = glm::rotate(_transform, angle, axis);
  _transform[0] *= s.x;
  _transform[1] *= s.y;
  _transform[2] *= s.z;
  for (auto child : _children)
    if (auto transChild{std::dynamic_pointer_cast<TransformableObject>(child)})
      transChild->rotate(angle, axis);
}

void TransformableObject::scale(vec3 xyz) {
  _transform = glm::scale(_transform, xyz);
  for (auto child : _children)
    if (auto transChild{std::dynamic_pointer_cast<TransformableObject>(child)})
      transChild->scale(xyz);
}

vec3 TransformableObject::position() const { return _transform[3]; }

void TransformableObject::setPosition(vec3 xyz) { _transform[3] = {xyz, 1}; }

vec3 TransformableObject::rotation() const {
  vec3 euler;
  glm::extractEulerAngleXYZ(_transform, euler.x, euler.y, euler.z);
  return euler;
}

void TransformableObject::setRotation(vec3 euler) {
  auto s{scale()};
  _transform[0] = s.x * vec4{1, 0, 0, 0};
  _transform[1] = s.y * vec4{0, 1, 0, 0};
  _transform[2] = s.z * vec4{0, 0, 1, 0};
  _transform = glm::eulerAngleXYZ(euler.x, euler.y, euler.z) * _transform;
}

vec3 TransformableObject::scale() const {
  return {glm::length(_transform[0]), //
          glm::length(_transform[1]), //
          glm::length(_transform[2])};
}

void TransformableObject::setScale(vec3 xyz) {
  _transform[0] *= xyz.x / glm::length(_transform[0]);
  _transform[1] *= xyz.y / glm::length(_transform[1]);
  _transform[2] *= xyz.z / glm::length(_transform[2]);
}

const mat4 &TransformableObject::transform() const { return _transform; }

mat4 &TransformableObject::transform() { return _transform; }
