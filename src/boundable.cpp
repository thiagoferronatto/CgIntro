#include "boundable.hpp"

// taken from
// https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
bool Boundable::boundsIntersect(Boundable &other) {
  return (_boundingBox.a.x <= other._boundingBox.b.x &&
          _boundingBox.b.x >= other._boundingBox.a.x) &&
         (_boundingBox.a.y <= other._boundingBox.b.y &&
          _boundingBox.b.y >= other._boundingBox.a.y) &&
         (_boundingBox.a.z <= other._boundingBox.b.z &&
          _boundingBox.b.z >= other._boundingBox.a.z);
}

bool Boundable::isBound() const { return _isBound; }

AABB Boundable::bounds() const { return _boundingBox; };