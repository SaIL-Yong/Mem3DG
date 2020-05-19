
#include <cassert>
#include <cmath>
#include <iostream>

#include <geometrycentral/utilities/vector3.h>

#include "ddgsolver/icosphere.h"

namespace ddgsolver {

namespace gc = ::geometrycentral;

void icosphere(std::vector<gc::Vector3> &coords,
               std::vector<std::vector<std::size_t>> &polygons, int n) {
  // Initialize vertex coordinates
  static const double t = (1.0 + sqrt(5.0)) / 2.0;
  auto makeNormedVertex = [](double x, double y, double z) -> gc::Vector3 {
    return gc::Vector3{std::move(x), std::move(y), std::move(z)}.normalize();
  };

  coords.emplace_back(makeNormedVertex(-1, t, 0));
  coords.emplace_back(makeNormedVertex(1, t, 0));
  coords.emplace_back(makeNormedVertex(-1, -t, 0));
  coords.emplace_back(makeNormedVertex(1, -t, 0));
  coords.emplace_back(makeNormedVertex(0, -1, t));
  coords.emplace_back(makeNormedVertex(0, 1, t));
  coords.emplace_back(makeNormedVertex(0, -1, -t));
  coords.emplace_back(makeNormedVertex(0, 1, -t));
  coords.emplace_back(makeNormedVertex(t, 0, -1));
  coords.emplace_back(makeNormedVertex(t, 0, 1));
  coords.emplace_back(makeNormedVertex(-t, 0, -1));
  coords.emplace_back(makeNormedVertex(-t, 0, 1));

  for (std::size_t i = 0; i < coords.size(); ++i) {
    coords[i] = coords[i].normalize();
  }

  // Initialize Faces
  polygons.emplace_back(std::vector<std::size_t>{0, 11, 5});
  polygons.emplace_back(std::vector<std::size_t>{0, 5, 1});
  polygons.emplace_back(std::vector<std::size_t>{0, 1, 7});
  polygons.emplace_back(std::vector<std::size_t>{0, 7, 10});
  polygons.emplace_back(std::vector<std::size_t>{0, 10, 11});
  polygons.emplace_back(std::vector<std::size_t>{1, 5, 9});
  polygons.emplace_back(std::vector<std::size_t>{5, 11, 4});
  polygons.emplace_back(std::vector<std::size_t>{11, 10, 2});
  polygons.emplace_back(std::vector<std::size_t>{10, 7, 6});
  polygons.emplace_back(std::vector<std::size_t>{7, 1, 8});
  polygons.emplace_back(std::vector<std::size_t>{3, 9, 4});
  polygons.emplace_back(std::vector<std::size_t>{3, 4, 2});
  polygons.emplace_back(std::vector<std::size_t>{3, 2, 6});
  polygons.emplace_back(std::vector<std::size_t>{3, 6, 8});
  polygons.emplace_back(std::vector<std::size_t>{3, 8, 9});
  polygons.emplace_back(std::vector<std::size_t>{4, 9, 5});
  polygons.emplace_back(std::vector<std::size_t>{2, 4, 11});
  polygons.emplace_back(std::vector<std::size_t>{6, 2, 10});
  polygons.emplace_back(std::vector<std::size_t>{8, 6, 7});
  polygons.emplace_back(std::vector<std::size_t>{9, 8, 1});

  auto getMidPoint = [&coords](size_t t1, size_t t2) -> std::size_t {
    gc::Vector3 &p1 = coords[t1];
    gc::Vector3 &p2 = coords[t2];
    coords.emplace_back(((p1 + p2) / 2).normalize());
    return coords.size() - 1;
  };

  // Preallocate space
  std::size_t finalsize = polygons.size() * std::pow(4, n);
  std::vector<std::vector<std::size_t>> polygons_new;
  polygons_new.reserve(finalsize);
  polygons.reserve(finalsize);

  // Subdivide n times by quadrisection
  for (std::size_t iter = 0; iter < n; ++iter) {
    std::size_t sz = polygons.size();
    polygons_new.clear();
    for (std::size_t f = 0; f < sz; ++f) {
      std::size_t a = getMidPoint(polygons[f][0], polygons[f][1]);
      std::size_t b = getMidPoint(polygons[f][1], polygons[f][2]);
      std::size_t c = getMidPoint(polygons[f][2], polygons[f][0]);

      polygons_new.emplace_back(std::vector<std::size_t>{polygons[f][0], a, c});
      polygons_new.emplace_back(std::vector<std::size_t>{polygons[f][1], b, a});
      polygons_new.emplace_back(std::vector<std::size_t>{polygons[f][2], c, b});
      polygons_new.emplace_back(std::vector<std::size_t>{a, b, c});
    }
    std::swap(polygons, polygons_new);
  }
}
} // end namespace ddgsolver