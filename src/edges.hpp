#ifndef EDGES_HPP
#define EDGES_HPP

#include "edge.hpp"
#include "face.hpp"
#include "faces.hpp"
#include <unordered_set>
#include <algorithm>

using Edges = std::unordered_set<Edge>;

auto &operator-=(Edges &edges, const Face &face) {
	for (const auto &edge: face)
		if (!edges.erase(edge))
			edges.insert(-edge);
	return edges;
}

auto operator||(const Edges &edges, const Faces &faces) {
	return std::any_of(faces.begin(), faces.end(), [&](const auto &face) {
		return std::any_of(face.begin(), face.end(), [&](const auto &edge) {
			return edges.count(edge) > 0;
		});
	});
}

#endif
