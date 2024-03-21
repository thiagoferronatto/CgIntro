#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <memory>
#include <string>
#include <vector>

class Object {
public:
  Object(std::string name);
  Object(const Object &other);
  Object(Object &&other) noexcept;

  Object &operator=(const Object &other);
  Object &operator=(Object &&other) noexcept;

  virtual ~Object() = default;

  const std::string &name() const;
  std::string &name();

  // template <typename T> void addChild(std::shared_ptr<T> child);

  // const std::vector<std::shared_ptr<Object>> &children() const;

  // std::shared_ptr<Object> parent() const;

protected:
  std::string _name;
  // std::vector<std::shared_ptr<Object>> _children{};
  // std::shared_ptr<Object> _parent{};

private:
};

// template <typename T> inline void Object::addChild(std::shared_ptr<T> child)
// {
//   _children.emplace_back(std::move(child));
// }

#endif // OBJECT_HPP