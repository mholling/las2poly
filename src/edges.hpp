#ifndef EDGES_HPP
#define EDGES_HPP

#include "edge.hpp"
#include "face.hpp"
#include "faces.hpp"
#include "rings.hpp"
#include <unordered_set>
#include <algorithm>

class Edges {
	std::unordered_set<Edge> edges;

public:
	auto &operator+=(const Edge &edge) {
		if (!edges.erase(-edge))
			edges.insert(edge);
		return *this;
	}

	auto operator-=(const Edge &edge) {
		if (!edges.erase(edge))
			edges.insert(-edge);
		return *this;
	}

	auto &operator+=(const Face &face) {
		for (const auto &edge: face)
			*this += edge;
		return *this;
	}

	auto &operator-=(const Face &face) {
		for (const auto &edge: face)
			*this -= edge;
		return *this;
	}

	auto operator||(const Faces &faces) const {
		return std::any_of(faces.begin(), faces.end(), [&](const auto &face) {
			return std::any_of(face.begin(), face.end(), [&](const auto &edge) {
				return edges.count(edge) > 0;
			});
		});
	}

	auto rings() const {
		return Rings(edges)();
	}
};

#endif
