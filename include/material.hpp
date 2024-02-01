#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <string>

#include "glm/glm.hpp"

struct Material {
  glm::vec3 Ka{0.35f};  // ambient color
  glm::vec3 Kd{1.0f};   // diffuse color
  glm::vec3 Ks{1.0f};   // specular color
  float Ns{1000.0f};    // shininess
  float Ni{1.0f};       // index of refraction
  float d{1.0f};        // dissolve (transparency)
  std::string map_Kd{}; // albedo texture
};

#endif // MATERIAL_HPP