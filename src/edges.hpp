#ifndef EDGES_HPP
#define EDGES_HPP

#include "edge.hpp"
#include "face.hpp"
#include "faces.hpp"
#include "rings.hpp"
#include <unordered_set>

class Edges {
	std::unordered_set<Edge> edges;

public:
	auto &operator+=(const Face &face) {
		for (const auto &edge: face)
			if (!edges.erase(-edge))
				edges.insert(edge);
		return *this;
	}

	auto &operator-=(const Face &face) {
		for (const auto &edge: face)
			if (!edges.erase(edge))
				edges.insert(-edge);
		return *this;
	}

	auto operator||(const Faces &faces) const {
		for (const auto &face: faces)
			for (const auto edge: face)
				if (edges.count(edge) > 0)
					return true;
		return false;
	}

	auto rings() const {
		return Rings(edges)();
	}
};

#endif
