#ifndef BEZIER_PATCH_HPP
#define BEZIER_PATCH_HPP

#include <cassert>
#include <print>
#include <type_traits>
#include <vector>

#include "aabb.hpp"
#include "quickhull/QuickHull.hpp"
#include "triangle_mesh.hpp"

class BezierPatch {
public:
  using PointCloud = std::vector<glm::vec3>;

  BezierPatch(PointCloud &&controlPoints)
      : _controlPoints{std::move(controlPoints)} {
    assert(_controlPoints.size() == 16);
    ++_patchCount;
  }

  bool operator==(const BezierPatch &other) const {
    return _patchId == other._patchId;
  }

  AAB getSubpatchAabb(const Type &u, const Type &v) const {
    Type B[4];
    B[0] = B[3] = 1;
    B[1] = B[2] = 3;

    auto u2 = u * u, u3 = u2 * u;
    auto uc = Type{1} - u, uc2 = uc * uc, uc3 = uc2 * uc;
    auto v2 = v * v, v3 = v2 * v;
    auto vc = Type{1} - v, vc2 = vc * vc, vc3 = vc2 * vc;
    Type U[4]{1, u, u2, u3};
    Type UC[4]{1, uc, uc2, uc3};
    Type V[4]{1, v, v2, v3};
    Type VC[4]{1, vc, vc2, vc3};

    Type xhat, yhat, zhat;
    for (i32 i{}; i < 4; ++i) {
      auto BU = B[i] * U[i] * UC[3 - i];
      for (i32 j{}; j < 4; ++j) {
        auto BV = B[j] * V[j] * VC[3 - j];
        auto BUBV = BU * BV;
        auto &p = _controlPoints[4 * i + j];
        xhat = xhat + BUBV * p.x;
        yhat = yhat + BUBV * p.y;
        zhat = zhat + BUBV * p.z;
      }
    }

    Interval xi, yi, zi;
#if USING_AA
    xi = xhat.convert();
    yi = yhat.convert();
    zi = zhat.convert();
#else
    xi = xhat;
    yi = yhat;
    zi = zhat;
#endif // USING_AA

    return AAB{{xi.left(), yi.left(), zi.left()},
               {xi.right(), yi.right(), zi.right()}};
  }

  std::vector<TriangleMesh>
  getAabbMeshes(i32 subdCount, std::vector<AAB> *boxes = nullptr) const {
    std::vector<TriangleMesh> meshArray(subdCount * subdCount);
    float u = 0, v = 0;
    auto incr{1.0f / subdCount};
    u64 counter = 0;
    for (i32 i{}; i < subdCount; ++i, u += incr, v = 0) {
      for (i32 j{}; j < subdCount; ++j, v += incr) {
        auto box{getSubpatchAabb(Interval{u, u + incr}, Interval{v, v + incr})};
        boxes->push_back(box);
        meshArray[counter++] = box.getMesh();
      }
    }
    return meshArray;
  }

#if USING_AA
  std::vector<TriangleMesh> getHullMeshes(
      i32 subdCount,
      std::vector<quickhull::ConvexHull<f64>> *hulls = nullptr) const {
    std::vector<TriangleMesh> meshArray(subdCount * subdCount);
    float u{};
    float v{};
    auto incr{1.0f / subdCount};
    u64 counter = 0;
    for (i32 i{}; i < subdCount; ++i, u += incr, v = 0) {
      for (i32 j{}; j < subdCount; ++j, v += incr) {
        auto mesh{
            getSubpatchHull(interval{u, u + incr}, interval{v, v + incr})};
        // TODO: extract hull and insert into hulls vector
        meshArray[counter++] = mesh;
      }
    }
    return meshArray;
  }
#endif // USING_AA

#if USING_AA
  TriangleMesh getSubpatchHull(const Type &u, const Type &v) const {
    // Type is an affine form
    Type xhat, yhat, zhat;

    /* Bézier computations */ {
      Type B[4];
      B[0] = B[3] = 1;
      B[1] = B[2] = 3;

      auto u2 = u * u, u3 = u2 * u;
      auto uc{Type{1} - u}, uc2{uc * uc}, uc3{uc2 * uc};
      auto v2{v * v}, v3{v2 * v};
      auto vc{Type{1} - v}, vc2{vc * vc}, vc3{vc2 * vc};
      Type U[4]{1, u, u2, u3};
      Type UC[4]{1, uc, uc2, uc3};
      Type V[4]{1, v, v2, v3};
      Type VC[4]{1, vc, vc2, vc3};
      for (i32 i = 0; i < 4; ++i) {
        auto BU = B[i] * U[i] * UC[3 - i];
        for (i32 j = 0; j < 4; ++j) {
          Type BV = B[j] * V[j] * VC[3 - j];
          const glm::vec3 &p = _controlPoints[4 * i + j];
          auto BUBV = BU * BV;
          xhat = xhat + BUBV * p.x;
          yhat = yhat + BUBV * p.y;
          zhat = zhat + BUBV * p.z;
        }
      }
    }

    // term condensation
    f32 xsum = 0, ysum = 0, zsum = 0;
    for (u64 i = 2, len{xhat.get_length()}; i < len; ++i)
      xsum += std::fabs(xhat.get_coeff(i));

    for (u64 i = 2, len{yhat.get_length()}; i < len; ++i)
      ysum += std::fabs(yhat.get_coeff(i));

    for (u64 i = 2, len{zhat.get_length()}; i < len; ++i)
      zsum += std::fabs(zhat.get_coeff(i));

    // reduced affine forms
    quickhull::Vector3<f64> r[6] = {};
    r[0] = {xhat.get_center(), yhat.get_center(), zhat.get_center()};
    r[1] = {xhat.get_coeff(0), yhat.get_coeff(0), zhat.get_coeff(0)};
    r[2] = {xhat.get_coeff(1), yhat.get_coeff(1), zhat.get_coeff(1)};
    r[3].x = xsum;
    r[4].y = ysum;
    r[5].z = zsum;

    quickhull::Vector3<f64> vertices[32];
    for (i32 i = 0; i < 32; ++i) {
      auto &v = vertices[i];
      v = r[0];
      for (i32 j = 0; j < 5; ++j)
        v += (i & 1 << j ? 1.0 : -1.0) * r[j + 1];
    }

    // used a tiny quickhull library
    quickhull::QuickHull<f64> qh;
    auto hull = qh.getConvexHull(vertices, 32, true, false, DBL_EPSILON);

    // convex hull mesh generation for visualization
    TriangleMesh mesh;
    auto &verts = mesh.vertices, &norms = mesh.normals;
    auto &trigs = mesh.triangles;
    auto &hTrigs = hull.getIndexBuffer();
    auto &hVerts = hull.getVertexBuffer();
    for (u32 i = 0; i < hTrigs.size(); i += 3) {
      auto i0 = hTrigs[i], i1 = hTrigs[i + 1], i2 = hTrigs[i + 2];
      trigs.push_back({i, i + 1, i + 2});

      auto v0{hVerts[i0]}, v1{hVerts[i1]}, v2{hVerts[i2]};
      verts.push_back({v0.x, v0.y, v0.z});
      verts.push_back({v1.x, v1.y, v1.z});
      verts.push_back({v2.x, v2.y, v2.z});

      auto n{(hVerts[i2] - hVerts[i0])
                 .crossProduct(hVerts[i1] - hVerts[i0])
                 .getNormalized()};
      norms.push_back({n.x, n.y, n.z});
      norms.push_back({n.x, n.y, n.z});
      norms.push_back({n.x, n.y, n.z});
    }

    return mesh;
  }
#endif // USING_AA

  TriangleMesh tessellate(u32 level) const {
    TriangleMesh mesh;
    auto stepSize{1.0f / level};
    f32 u{};
    f32 v{};

    for (u32 i{}; i < level; ++i, u += stepSize) {
      for (u32 j{}; j < level; ++j, v += stepSize) {
        f32 B[4];
        B[0] = B[3] = 1;
        B[1] = B[2] = 3;

        auto u2{u * u}, u3{u2 * u};
        auto uc{1 - u}, uc2{uc * uc}, uc3{uc2 * uc};
        auto v2{v * v}, v3{v2 * v};
        auto vc{1 - v}, vc2{vc * vc}, vc3{vc2 * vc};
        f32 U[4]{1, u, u2, u3};
        f32 DU[4]{0, 1, 2 * u, 3 * u2};
        f32 UC[4]{1, uc, uc2, uc3};
        f32 DUC[4]{0, -1, 2 * u - 2, -3 * uc2};
        f32 V[4]{1, v, v2, v3};
        f32 DV[4]{0, 1, 2 * v, 3 * v2};
        f32 VC[4]{1, vc, vc2, vc3};
        f32 DVC[4]{0, -1, 2 * v - 2, -3 * vc2};

        glm::vec3 s{};
        glm::vec3 du{};
        glm::vec3 dv{};
        for (i32 i{}; i < 4; ++i) {
          auto BU{B[i] * U[i] * UC[3 - i]};
          auto DBU{B[i] * (DU[i] * UC[3 - i] + U[i] * DUC[3 - i])};
          for (i32 j{}; j < 4; ++j) {
            auto BV{B[j] * V[j] * VC[3 - j]};
            auto DBV{B[j] * (DV[j] * VC[3 - j] + V[j] * DVC[3 - j])};
            auto p{_controlPoints[4 * i + j]};
            s += BU * BV * p;
            du += DBU * BV * p;
            dv += BU * DBV * p;
          }
        }
        mesh.vertices.push_back(std::move(s));
        auto vertexNormal{glm::normalize(glm::cross(du, dv))};
        mesh.normals.push_back(std::move(vertexNormal));
      }
      v = 0;
    }
    for (u32 i{level}, end{level * level}; i < end; ++i) {
      if (i % level == level - 1)
        continue;
      mesh.triangles.push_back({i - level, i, i + 1});
      mesh.triangles.push_back({i + 1, i - level + 1, i - level});
    }
    return mesh;
  }

  auto &controlPoints() { return _controlPoints; }

private:
  PointCloud _controlPoints;
  inline static size_t _patchCount = 0;
  size_t _patchId = _patchCount;
};

#endif // BEZIER_PATCH_HPP