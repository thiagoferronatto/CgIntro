#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "transformable_object.hpp"

class Camera : public TransformableObject {
public:
  Camera(std::string name, float fov, float aspect, float near, float far);
  Camera(const Camera &other);
  Camera(Camera &&other) noexcept;

  mat4 perspective() const;
  mat4 worldToCamera() const;

  void translate(vec3 xyz) override;
  void rotate(float angle, vec3 xyz) override;
  void scale(vec3 xyz) override;

  float fov() const;
  float aspect() const;
  float near() const;
  float far() const;

  void setFov(float deg);
  void setAspect(float aspect);
  void setNear(float near);
  void setFar(float far);

  void updateWorldToCamera();
  void updatePerspective();

private:
  float _fov, _aspect, _near, _far;
  mat4 _perspective, _worldToCamera;
};

#endif // CAMERA_HPP