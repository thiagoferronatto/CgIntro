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
  _transform[3].x += xyz.x;
  _transform[3].y += xyz.y;
  _transform[3].z += xyz.z;
  //_transform = glm::translate(_transform, xyz / scale());
  // for (auto child : _children)
  //  if (auto
  //  transChild{std::dynamic_pointer_cast<TransformableObject>(child)})
  //    transChild->translate(xyz);
}

void TransformableObject::rotate(vec3 euler) {
  auto x{euler.x}, y{euler.y}, z{euler.z}, cx{cos(x)}, sx{sin(x)}, cy{cos(y)},
      sy{sin(y)}, cz{cos(z)}, sz{sin(z)};
  mat4 rx{1, 0,  0,   0, //
          0, cx, -sx, 0, //
          0, sx, cx,  0, //
          0, 0,  0,   1};
  mat4 ry{cy,  0, sy, 0, //
          0,   1, 0,  0, //
          -sy, 0, cy, 0, //
          0,   0, 0,  1};
  mat4 rz{cz, -sz, 0, 0, //
          sz, cz,  0, 0, //
          0,  0,   1, 0, //
          0,  0,   0, 1};
  vec3 s{scale()};
  auto pos = position();
  translate(-pos);
  _transform[0] /= s.x;
  _transform[1] /= s.y;
  _transform[2] /= s.z;
  _transform = rz * ry * rx * _transform;
  _transform[0] *= s.x;
  _transform[1] *= s.y;
  _transform[2] *= s.z;
  translate(pos);
  // for (auto child : _children)
  //   if (auto
  //   transChild{std::dynamic_pointer_cast<TransformableObject>(child)})
  //     transChild->rotate(euler);
}

void TransformableObject::rotate(float t, vec3 axis) {
  if (axis.x == 0.0f && axis.y == 0.0f && axis.z == 0.0f)
    return;
  axis = normalize(axis);
  auto x{axis.x}, y{axis.y}, z{axis.z}, x2{x * x}, y2{y * y}, z2{z * z},
      c{cos(t)}, s{sin(t)}, c_{1.0f - c};
  // clang-format off
  mat4 r{c + x2 * c_, x * y * c_ - z * s, x * z * c_ + y * s, 0,
         y * x * c_ + z * s, c + y2 * c_, y * z * c_ - x * s, 0,
         z * x * c_ - y * s, z * y * c_ + x * s, c + z2 * c_, 0,
         0, 0, 0, 1};
  // clang-format on
  vec3 s_{scale()};
  auto pos = position();
  translate(-pos);
  _transform[0] /= s_.x;
  _transform[1] /= s_.y;
  _transform[2] /= s_.z;
  _transform = r * _transform;
  _transform[0] *= s_.x;
  _transform[1] *= s_.y;
  _transform[2] *= s_.z;
  translate(pos);
  // for (auto child : _children)
  //   if (auto
  //   transChild{std::dynamic_pointer_cast<TransformableObject>(child)})
  //     transChild->rotate(t, axis);
}

void TransformableObject::scale(vec3 xyz) {
  _transform[0] *= xyz.x;
  _transform[1] *= xyz.y;
  _transform[2] *= xyz.z;
  //_transform = glm::scale(_transform, xyz);
  // for (auto child : _children)
  //  if (auto
  //  transChild{std::dynamic_pointer_cast<TransformableObject>(child)})
  //    transChild->scale(xyz);
}

vec3 TransformableObject::position() const { return _transform[3]; }

void TransformableObject::setPosition(vec3 xyz) { _transform[3] = {xyz, 1}; }

vec3 TransformableObject::rotation() const {
  vec3 euler;
  extractEulerAngleXYZ(inverse(_transform), euler.x, euler.y, euler.z);
  return euler;
}

void TransformableObject::setRotation(vec3 euler) {
  auto s{scale()};
  _transform[0] = s.x * vec4{1, 0, 0, 0};
  _transform[1] = s.y * vec4{0, 1, 0, 0};
  _transform[2] = s.z * vec4{0, 0, 1, 0};
  rotate(euler);
}

// WARNING: expensive as fuck
vec3 TransformableObject::scale() const {
  return {glm::length(_transform[0]), //
          glm::length(_transform[1]), //
          glm::length(_transform[2])};
}

void TransformableObject::setScale(vec3 xyz) { scale(xyz / scale()); }

const mat4 &TransformableObject::transform() const { return _transform; }

mat4 &TransformableObject::transform() { return _transform; }
