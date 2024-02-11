#ifndef TRANSFORMABLE_OBJECT_HPP
#define TRANSFORMABLE_OBJECT_HPP

#include "glm/ext.hpp"
#include "object.hpp"

using namespace glm;

class TransformableObject : public Object {
public:
  TransformableObject(std::string name);
  TransformableObject(const TransformableObject &other);
  TransformableObject(TransformableObject &&other) noexcept;

  TransformableObject &operator=(const TransformableObject &other);
  TransformableObject &operator=(TransformableObject &&other) noexcept;

  virtual void translate(vec3 xyz);
  virtual void rotate(vec3 euler);
  virtual void rotate(float angle, vec3 axis);
  virtual void scale(vec3 xyz);

  virtual vec3 position() const;
  virtual void setPosition(vec3 xyz);
  virtual vec3 rotation() const;
  virtual void setRotation(vec3 euler);
  virtual vec3 scale() const;
  virtual void setScale(vec3 xyz);

  const mat4 &transform() const;
  mat4 &transform();

protected:
  mat4 _transform;

private:
};

#endif // TRANSFORMABLE_OBJECT_HPP