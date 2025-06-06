#ifndef TRIANGLE_MESH_HPP
#define TRIANGLE_MESH_HPP

#include <vector>

#include "glad/glad.h"
#include "glm/vec3.hpp"
#include "types.hpp"

// just one reusable VAO
GLuint vao{};

class TriangleMesh {
public:
  void draw() {
    if (!vbos[0] || !vbos[1] || !ebo) {
      glDeleteBuffers(2, vbos);
      glDeleteBuffers(1, &ebo);
      glGenBuffers(2, vbos);
      glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
      glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
                   vertices.data(), GL_STATIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
      glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3),
                   normals.data(), GL_STATIC_DRAW);
      glGenBuffers(1, &ebo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                   triangles.size() * sizeof(glm::uvec3), triangles.data(),
                   GL_STATIC_DRAW);
    }

    if (!vao)
      glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glDrawElements(GL_TRIANGLES, 3 * triangles.size(), GL_UNSIGNED_INT,
                   nullptr);
  }

  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<glm::uvec3> triangles;

  // OpenGL stuff
  u32 vbos[2]{};
  u32 ebo{};
};

#endif // TRIANGLE_MESH_HPP