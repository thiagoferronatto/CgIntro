#include "actor.hpp"

Actor::Actor(std::string name, const TriangleMesh &mesh)
    : TransformableObject{std::move(name)}, mesh{&mesh} {}

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
  auto center = _boundingBox.center();
  // an attempt to avoid rebounding when scaling
  _boundingBox.a = (((_boundingBox.a - center) * abs(xyz)) + center);
  _boundingBox.b = (((_boundingBox.b - center) * abs(xyz)) + center);
}

void Actor::setPosition(vec3 xyz) {
  _boundingBox.a += xyz - position();
  _boundingBox.b += xyz - position();
  this->TransformableObject::setPosition(xyz);
}

void Actor::setRotation(vec3 euler) {
  this->TransformableObject::setRotation(euler);
  bound();
}

void Actor::setScale(vec3 xyz) {
  this->TransformableObject::setScale(xyz);
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

void Actor::bound() {
  _boundingBox.a = vec3{std::numeric_limits<float>::max()};
  _boundingBox.b = vec3{std::numeric_limits<float>::lowest()};
  const auto &meshData{mesh->data()};
  for (auto &local_v : meshData.vertices) {
    vec3 v = _transform * vec4{local_v, 1};
    _boundingBox.a = min(_boundingBox.a, v);
    _boundingBox.b = max(_boundingBox.b, v);
  }
  _isBound = true;
}

void Actor::initializeRigidBody(float mass) {
  this->RigidBody::initializeRigidBody(mass);
  _centerOfMass = {};
  const auto &meshData{mesh->data()};
  if (_inverseMass == 0.0f) {
    _invInertiaTensor = {};
    for (auto &local_v : meshData.vertices)
      _centerOfMass += vec3{_transform * vec4{local_v, 1}};
    _centerOfMass /= float(meshData.vertices.size());
    return;
  }
  float vertexMass = 1.0f / (float(meshData.vertices.size()) * _inverseMass);
  vec3 inertiaTensor{};
  for (auto &local_v : meshData.vertices) {
    vec3 v{_transform * vec4{local_v, 1}};
    inertiaTensor.x += vertexMass * (v.y * v.y + v.z * v.z);
    inertiaTensor.y += vertexMass * (v.x * v.x + v.z * v.z);
    inertiaTensor.z += vertexMass * (v.x * v.x + v.y * v.y);
    _centerOfMass += v;
  }
  _invInertiaTensor = 1.0f / inertiaTensor;
  _centerOfMass /= float(meshData.vertices.size());
}