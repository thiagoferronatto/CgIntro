#ifndef ACTOR_HPP
#define ACTOR_HPP

#include "material.hpp"
#include "rigid_body.hpp"
#include "transformable_object.hpp"
#include "triangle_mesh.hpp"

class Actor : public TransformableObject, public RigidBody {
public:
  Actor(std::string name, const TriangleMesh *mesh);
  Actor(std::string name, Material m);
  Actor(const Actor &other);
  Actor(Actor &&other) noexcept;

  void translate(vec3 xyz) override;
  void rotate(vec3 euler) override;
  void scale(vec3 xyz) override;

  void setPosition(vec3 xyz) override;
  void setRotation(vec3 euler) override;
  void setScale(vec3 xyz) override;

  Actor &operator=(const Actor &other);
  Actor &operator=(Actor &&other) noexcept;

  void bound() override;
  void initializeRigidBody(float mass) override;

  Material material{};
  cg::Reference<TriangleMesh> mesh;
};

#endif // ACTOR_HPP