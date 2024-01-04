#include "ppm.hpp"

#include <fstream>
#include <sstream>

#include "log.hpp"

PortablePixelMap::PortablePixelMap(const std::string &file) {
  std::ifstream is{file};
  if (!is) {
    log("[ERROR] Could not open file %s, terminating", file.c_str());
    std::terminate();
  }
  std::string tmp;
  std::getline(is, tmp);
  if (tmp != "P6") {
    log("[ERROR] Invalid PPM file", file.c_str());
    std::terminate();
  }
  std::getline(is, tmp);
  std::istringstream{tmp} >> _width >> _height;
  std::getline(is, tmp);
  _pixels = new char[3 * _width * _height]{};
  is.read(_pixels, 3 * _width * _height);
}

PortablePixelMap::~PortablePixelMap() {
  if (_pixels)
    delete[] _pixels;
}

const char *PortablePixelMap::pixels() const { return _pixels; }

size_t PortablePixelMap::width() const { return _width; }

size_t PortablePixelMap::height() const { return _height; }
