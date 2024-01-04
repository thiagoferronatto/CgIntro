#include "window.hpp"

Window::Window(size_t width, size_t height, const char *title)
    : _width{width}, _height{height} {
  if (!glfwInit())
    throw std::runtime_error{"GLFW could not be initialized"};
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_SAMPLES, 8);
  _window = glfwCreateWindow(GLsizei(width), GLsizei(height), title, nullptr,
                             nullptr);
  if (!_window) {
    glfwTerminate();
    throw std::runtime_error{"GLFW window could not be created"};
  }
  glfwMakeContextCurrent(_window);
  if (!gladLoadGL()) {
    glfwTerminate();
    throw std::runtime_error{"GLAD could not load OpenGL"};
  }
  if (glfwRawMouseMotionSupported())
    glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

  glfwSetFramebufferSizeCallback(_window,
                                 [](GLFWwindow *window, int width, int height) {
                                   glViewport(0, 0, width, height);
                                 });

  // ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  ImGui_ImplGlfw_InitForOpenGL(_window, true);
  ImGui_ImplOpenGL3_Init();
  auto segoeUi = io.Fonts->AddFontFromFileTTF("assets/fonts/segoeui.ttf", 16);
  if (segoeUi) {
    io.Fonts->Build();
    io.FontDefault = segoeUi;
  }
}

Window::~Window() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
}

size_t Window::width() const {
  int w, h;
  glfwGetWindowSize(_window, &w, &h);
  return w;
}

size_t Window::height() const {
  int w, h;
  glfwGetWindowSize(_window, &w, &h);
  return h;
}

bool Window::shouldClose() const { return glfwWindowShouldClose(_window); }
void Window::swapBuffers() const { glfwSwapBuffers(_window); }
void Window::pollEvents() const { glfwPollEvents(); }

bool Window::keyIsPressed(int key) const {
  return glfwGetKey(_window, key) == GLFW_PRESS;
}

std::tuple<float, float> Window::getCursorPos() const {
  double x, y;
  glfwGetCursorPos(_window, &x, &y);
  return std::forward_as_tuple(x, y);
}

void Window::show() const { glfwShowWindow(_window); }
