#include "object.hpp"

Object::Object(std::string name) : _name{std::move(name)} {}

Object::Object(const Object &other)
    : _name{other._name} //, _children{other._children}
{}

Object::Object(Object &&other) noexcept
    : _name{std::move(other._name)} //, _children{std::move(other._children)}
{}

Object &Object::operator=(const Object &other) {
  if (&other == this)
    goto skip;
  _name = other._name;
  //_children = other._children;
skip:
  return *this;
}

Object &Object::operator=(Object &&other) noexcept {
  if (&other == this)
    goto skip;
  _name = std::move(other._name);
  //_children = std::move(other._children);
skip:
  return *this;
}

const std::string &Object::name() const { return _name; }

std::string &Object::name() { return _name; }

// const std::vector<std::shared_ptr<Object>> &Object::children() const {
//   return _children;
// }
//
// std::shared_ptr<Object> Object::parent() const { return _parent; }
