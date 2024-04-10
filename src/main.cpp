#include "gl_util.hpp"
#include "window.hpp"

int main() {
  constexpr size_t w{900}, h{900};
  Window window{w, h, "Computer Graphics Intro"};

  window.show();

  // Contém as posições dos vértices dos triângulos
  // Atualmente possui somente 3 vértices, então só compõe 1 triângulo
  constexpr float pi{3.1415926535}, r{0.5};
  float vertices[]{
    r * cosf(0),                r * sinf(0),                0,
    r * cosf(2.0f * pi / 3.0f), r * sinf(2.0f * pi / 3.0f), 0,
    r * cosf(4.0f * pi / 3.0f), r * sinf(4.0f * pi / 3.0f), 0
  };

  // Contém as cores dos vértices dos triângulos
  // Atualmente possui somente 3 cores, então só compõe 1 triângulo
  constexpr float colors[] {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1
  };

  GLuint vbos[2]; // Guarda as IDs dos 2 buffers de vértices, um para as posições e outro para as cores
  glCheck(glCreateBuffers(2, vbos)); // Cria 2 buffers e guarda em vbos
  glCheck(glBindBuffer(GL_ARRAY_BUFFER, vbos[0])); // Fixa o primeiro buffer
  glCheck(glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), vertices, GL_STATIC_DRAW)); // Copia os dados de vertices pro buffer

  glCheck(glBindBuffer(GL_ARRAY_BUFFER, vbos[1])); // Fixa o segundo buffer
  glCheck(glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), colors, GL_STATIC_DRAW)); // Copia os dados de colors pro buffer

  GLuint vao; // Guarda a ID do vetor de vértices (contém tanto as posições quanto as cores)
  glCheck(glCreateVertexArrays(1, &vao)); // Cria 1 vetor de vértices e guarda em vao
  glCheck(glBindVertexArray(vao)); // Fixa esse vetor de vértices
  glCheck(glBindBuffer(GL_ARRAY_BUFFER, vbos[0])); // Fixa o primero buffer (posições)
  glCheck(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr)); // Diz que a localização 0 dentro do vetor de vértices vai conter as posições
  glCheck(glEnableVertexAttribArray(0)); // Ativa a localização 0 do vetor de vértices
  glCheck(glBindBuffer(GL_ARRAY_BUFFER, vbos[1])); // Fixa o segundo buffer (cores)
  glCheck(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr)); // Diz que a localização 1 dentro do vetor de vértices vai conter as cores
  glCheck(glEnableVertexAttribArray(1)); // Ativa a localização 1 do vetor de vértices

  // Um shader de vértices básico
  constexpr auto vsSrc{R"(
    #version 460

    layout (location = 0) in vec3 position;
    layout (location = 1) in vec3 color;

    uniform float t;

    out vec3 vertexColor;

    void main(void) {
      float c = cos(t), s = sin(t);
      vec3 p = mat3(
        c, -s, 0,
        s,  c, 0,
        0,  0, 1
      ) * position;
      gl_Position = vec4(p, 1);
      vertexColor = color;
    }
  )"};

  // Um shader de fragmentos básico
  constexpr auto fsSrc{R"(
    #version 460

    #extension GL_NV_fragment_shader_barycentric : require

    in vec3 vertexColor;

    out vec4 fragmentColor;

    void main(void) {
      const float epsilon = 0.01;
      fragmentColor = vec4(vertexColor, 1);
      vec3 b = gl_BaryCoordNV;
      if (b.x < epsilon || b.y < epsilon || b.z < epsilon
          || abs(b.x - b.y) < 10.0 * epsilon
          && abs(b.x - b.z) < 10.0 * epsilon
          && abs(b.y - b.z) < 10.0 * epsilon)
        fragmentColor = 0.25 * (fragmentColor * fragmentColor + 2.0 * sqrt(2.0 * fragmentColor));
    }
  )"};

  auto vs{glCreateShader(GL_VERTEX_SHADER)}; // Cria uma ID para o shader de vértices
  glCheck(glShaderSource(vs, 1, &vsSrc, nullptr)); // Diz que a string em vsSrc vai ser o código do shader de vértices
  glCheck(glCompileShader(vs)); // Compila o shader de vértices
  glCheckShaderCompilation(vs); // Checa se houve erros de compilação

  auto fs{glCreateShader(GL_FRAGMENT_SHADER)}; // Cria uma ID para o shader de fragmentos
  glCheck(glShaderSource(fs, 1, &fsSrc, nullptr)); // Diz que a string em fsSrc vai ser o código do shader de fragmentos
  glCheck(glCompileShader(fs)); // Compila o shader de fragmentos
  glCheckShaderCompilation(fs); // Checa se houve erros de compilação

  auto program{glCreateProgram()}; // Cria um programa (combinação de shaders)
  glCheck(glAttachShader(program, vs)); // Insere o shader de vértices no programa
  glCheck(glAttachShader(program, fs)); // Insere o shader de fragmentos no programa
  glCheck(glLinkProgram(program)); // Linka o programa
  glCheckProgramLinkage(program); // Checa se houve erros de linkagem

  glUseProgram(program); // Começa a utilizar o programa

  glCheck(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)); // Diz que triângulos terão seus interiores preenchidos

  auto tLoc{glGetUniformLocation(program, "t")};

  float t = 0;
  while (!window.shouldClose()) {
    glCheck(glClearColor(1, 1, 1, 1)); // Define a cor de fundo da janela
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)); // Limpa a janela usando a cor de fundo
    glCheck(glDrawArrays(GL_TRIANGLES, 0, 9)); // Desenha os vértices usando os buffers e shaders
    glCheck(glUniform1f(tLoc, t));
    window.swapBuffers();
    window.pollEvents();
    t += 0.01;
  }

  glCheck(glDeleteVertexArrays(1, &vao)); // Deleta o vetor de vértices
  glCheck(glDeleteBuffers(2, vbos)); // Deleta os buffers

  return 0;
}