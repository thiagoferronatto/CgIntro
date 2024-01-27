#include "camera.hpp"

#include "custom_assert.hpp"

Camera::Camera(std::string name, float fov, float aspect, float near, float far)
    : TransformableObject{name}, _fov{fov}, _aspect{aspect}, _near{near},
      _far{far}, _perspective{glm::perspective(fov, aspect, near, far)},
      _worldToCamera{glm::inverse(_transform)} {}

Camera::Camera(const Camera &other)
    : TransformableObject{other}, _fov{other._fov}, _aspect{other._aspect},
      _near{other._near}, _far{other._far}, _perspective{other._perspective},
      _worldToCamera{other._worldToCamera} {}

Camera::Camera(Camera &&other) noexcept
    : TransformableObject{std::move(other)}, _fov{other._fov},
      _aspect{other._aspect}, _near{other._near}, _far{other._far},
      _perspective{std::move(other._perspective)},
      _worldToCamera{std::move(other._worldToCamera)} {}

mat4 Camera::perspective() const { return _perspective; }

mat4 Camera::worldToCamera() const { return _worldToCamera; }

void Camera::translate(vec3 xyz) {
  this->TransformableObject::translate(xyz);
  updateWorldToCamera();
}

void Camera::rotate(float angle, vec3 xyz) {
  this->TransformableObject::rotate(angle, xyz);
  updateWorldToCamera();
}

void Camera::scale(vec3 xyz) {
  // Cameras should not be scaled
}

void Camera::setScale(vec3 xyz) {
  // Cameras should NOT be scaled
}

float Camera::fov() const { return glm::degrees(_fov); }

float Camera::aspect() const { return _aspect; }

float Camera::near() const { return _near; }

float Camera::far() const { return _far; }

void Camera::setFov(float deg) {
  ASSERT(0 <= deg && deg <= 180,
         "camera FOV must be in the range [1, 179], was %g", deg);
  _fov = glm::radians(deg);
  updatePerspective();
}

void Camera::setAspect(float aspect) {
  _aspect = aspect;
  updatePerspective();
}

void Camera::setNear(float near) {
  ASSERT(near >= 0, "near plane distance should be positive, was %g", near);
  ASSERT(near <= _far,
         "near plane must be closer than the far plane (%g), was %g", _far,
         _near);
  _near = near;
  updatePerspective();
}

void Camera::setFar(float far) {
  ASSERT(far >= 0, "far plane distance should be positive, was %g", far);
  ASSERT(far >= _near,
         "far plane must be farther than the near plane (%g), was %g", _near,
         far);
  _far = far;
  updatePerspective();
}

void Camera::updatePerspective() {
  _perspective = glm::perspective(_fov, _aspect, _near, _far);
}

void Camera::updateWorldToCamera() {
  _worldToCamera = glm::inverse(_transform);
}
