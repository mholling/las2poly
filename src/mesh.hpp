#ifndef MESH_HPP
#define MESH_HPP

#include "point.hpp"
#include "exterior.hpp"
#include <unordered_map>
#include <utility>
#include <algorithm>

class Mesh {
	std::unordered_multimap<Point, Point> graph;

public:
	void connect(const Point &p1, const Point &p2) {
		graph.insert(std::pair(p1, p2));
		graph.insert(std::pair(p2, p1));
	}

	void disconnect(const Point &p1, const Point &p2) {
		auto [start1, stop1] = graph.equal_range(p1);
		graph.erase(std::find(start1, stop1, std::pair(p1, p2)));
		auto [start2, stop2] = graph.equal_range(p2);
		graph.erase(std::find(start2, stop2, std::pair(p2, p1)));
	}

	auto exterior() const {
		return Exterior(graph);
	}

	auto &operator+=(Mesh &mesh) {
		graph.merge(mesh.graph);
		return *this;
	}

	template <typename Function>
	void each_face(Function function) {
		// TODO
		// * strip off exterior edges running clockwise
		// * split remaining edges into faces and yield each to function
	}
};

#endif
