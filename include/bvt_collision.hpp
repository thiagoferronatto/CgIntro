#ifndef BVT_COLLISION_HPP
#define BVT_COLLISION_HPP

#include <utility>
#include <vector>

#include "aabb.hpp"
#include "DynamicTree.h"
#include "TriangleMeshBVH.h"

inline bool aabbOverlap(const Aabb &box1, const Aabb &box2) {
  return (box1.a.x <= box2.b.x && box1.b.x >= box2.a.x) &&
         (box1.a.y <= box2.b.y && box1.b.y >= box2.a.y) &&
         (box1.a.z <= box2.b.z && box1.b.z >= box2.a.z);
}

template <typename Tree, typename Policy>
inline void collideTT(const Tree &tree0, const Tree &tree1, Policy policy) {
  using Node = decltype(tree0.getNode(0));
  using NodePair = std::pair<Node, Node>;
  auto root0{tree0.getNode(tree0.root())}, root1{tree1.getNode(tree1.root())};
  if (root0 && root1) {
    int depth = 1;
    int treshold = 124;
    std::vector<NodePair> stack;
    stack.resize(128);
    stack[0] = NodePair(root0, root1);
    do {
      NodePair p = stack[--depth];
      if (depth > treshold) {
        stack.resize(stack.size() * 2);
        treshold = int(stack.size()) - 4;
      }
      if (p.first == p.second) {
        if (!p.first->isLeaf()) {
          stack[depth++] = NodePair(tree0.getNode(p.first->children[0]),
                                    tree0.getNode(p.first->children[0]));
          stack[depth++] = NodePair(tree0.getNode(p.first->children[1]),
                                    tree0.getNode(p.first->children[1]));
          stack[depth++] = NodePair(tree0.getNode(p.first->children[0]),
                                    tree0.getNode(p.first->children[1]));
        }
      } else if (aabbOverlap(p.first->bounds, p.second->bounds)) {
        if (!p.first->isLeaf()) {
          if (!p.second->isLeaf()) {
            stack[depth++] = NodePair(tree0.getNode(p.first->children[0]),
                                      tree1.getNode(p.second->children[0]));
            stack[depth++] = NodePair(tree0.getNode(p.first->children[1]),
                                      tree1.getNode(p.second->children[0]));
            stack[depth++] = NodePair(tree0.getNode(p.first->children[0]),
                                      tree1.getNode(p.second->children[1]));
            stack[depth++] = NodePair(tree0.getNode(p.first->children[1]),
                                      tree1.getNode(p.second->children[1]));
          } else {
            stack[depth++] =
                NodePair(tree0.getNode(p.first->children[0]), p.second);
            stack[depth++] =
                NodePair(tree0.getNode(p.first->children[1]), p.second);
          }
        } else {
          if (!p.second->isLeaf()) {
            stack[depth++] =
                NodePair(p.first, tree1.getNode(p.second->children[0]));
            stack[depth++] =
                NodePair(p.first, tree1.getNode(p.second->children[1]));
          } else {
            // TODO: this feels awful, do something about it eventually
            if constexpr (std::is_same_v<Tree, cg::DynamicTree>)
              policy.process((cg::TriangleMeshBVH *)p.first->userData,
                             (cg::TriangleMeshBVH *)p.second->userData);
            else
              policy.process(p.first->first, p.first->count, p.second->first,
                             p.second->count);
          }
        }
      }
    } while (depth);
  }
}

#endif // BVT_COLLISION_HPP