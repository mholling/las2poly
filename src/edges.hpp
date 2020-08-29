#ifndef EDGES_HPP
#define EDGES_HPP

#include "edge.hpp"
#include "face.hpp"
#include "connections.hpp"
#include <unordered_set>

class Edges {
	std::unordered_set<Edge, Edge::Hash> edges;

public:
	void insert(const Face &face) {
		for (const auto edge: face.edges())
			if (!edges.erase(-edge))
				edges.insert(edge);
	}

	void erase(const Face &face) {
		for (const auto edge: face.edges())
			if (!edges.erase(edge))
				edges.insert(-edge);
	}

	auto rings() const {
		return Connections(edges).rings();
	}
};

#endif
