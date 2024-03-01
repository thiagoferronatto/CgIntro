#ifndef BOUNDABLE_HPP
#define BOUNDABLE_HPP

#include "aabb.hpp"

class Boundable {
public:
  // bound() should always set _isBound
  // must also be called when transformations are applied,
  // unless equivalent operations are performed on the AABB
  virtual void bound() = 0;

  bool boundsIntersect(Boundable &other);

  bool isBound() const;

  Aabb bounds() const;

protected:
  Aabb _boundingBox{};
  bool _isBound{};
};

#endif // BOUNDABLE_HPP