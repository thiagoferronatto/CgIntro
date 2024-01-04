#ifndef PPM_HPP
#define PPM_HPP

#include <string>
#include <vector>

#include "glm/glm.hpp"

class PortablePixelMap {
public:
  PortablePixelMap(const std::string &fileName);

  ~PortablePixelMap();

  const char *pixels() const;

  size_t width() const;
  size_t height() const;

private:
  char *_pixels{};
  size_t _width{}, _height{};
};

using PPM = PortablePixelMap;

#endif // PPM_HPP