#ifndef DBVH_HPP
#define DBVH_HPP

#include <memory>

#include "scene.hpp"

class DynamicBoundingVolumeHierarchy {
public:
  struct Node {
    AABB aabb;
    std::shared_ptr<Actor> object{};
    std::shared_ptr<Node> parent{}, left{}, right{};
    std::string debugName;
  };

  using NodePtr = std::shared_ptr<Node>;
  using VisitFun = std::function<void(NodePtr)>;

  DynamicBoundingVolumeHierarchy(const Scene &scene) {
    std::vector<NodePtr> workingList;
    for (auto &actor : scene.actors()) {
      auto tmpNode{std::make_shared<Node>(actor->bounds(), actor)};
      tmpNode->debugName = actor->name();
      workingList.push_back(tmpNode);
    }
    while (workingList.size() > 1) {
      float minSqDist{FLT_MAX};
      NodePtr n1, n2;
      for (auto &node1 : workingList) {
        for (auto &node2 : workingList) {
          if (node1 == node2)
            continue;
          auto c1{0.5f * (node1->aabb.a + node1->aabb.b)};
          auto c2{0.5f * (node2->aabb.a + node2->aabb.b)};
          auto cdiff{c1 - c2};
          auto sqDist{glm::dot(cdiff, cdiff)};
          if (sqDist < minSqDist) {
            minSqDist = sqDist;
            n1 = node1;
            n2 = node2;
          }
        }
      }
      auto newNode{
          std::make_shared<Node>(AABB{glm::min(n1->aabb.a, n2->aabb.a),
                                      glm::max(n1->aabb.b, n2->aabb.b)},
                                 nullptr, nullptr, n1, n2)};
      newNode->debugName = "(" + n1->debugName + " + " + n2->debugName + ")";
      n1->parent = n2->parent = newNode;
      std::erase(workingList, n1);
      std::erase(workingList, n2);
      workingList.push_back(newNode);
    }
    if (!workingList.empty())
      _root = workingList[0];
  }

  void visit(VisitFun f) { _visit(_root, f); }

  void debugPrint() {
    visit([](NodePtr node) {
      printf("[(%f %f %f), (%f %f %f)]", node->aabb.a.x, node->aabb.a.y,
             node->aabb.a.z, node->aabb.b.x, node->aabb.b.y, node->aabb.b.z);
      if (node->object)
        printf("%s", node->object->name().c_str());
      puts("");
    });
  }

private:
  void _visit(NodePtr node, VisitFun f) {
    if (node) {
      _visit(node->left, f);
      _visit(node->right, f);
      f(node);
    }
  }

  std::shared_ptr<Node> _root{};
};

using DBVH = DynamicBoundingVolumeHierarchy;

#endif // DBVH_HPP