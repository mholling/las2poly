#ifndef MESH_HPP
#define MESH_HPP

#include "point.hpp"
#include <unordered_map>
#include <utility>
#include <algorithm>

class Mesh {
	using EdgeIterator = std::unordered_multimap<Point, Point>::const_iterator;
	std::unordered_multimap<Point, Point> graph;

	static auto atan2(const Edge &edge1, const Edge &edge2) {
		return std::atan2(edge1 ^ edge2, edge1 * edge2);
	}

	auto next_interior(const EdgeIterator &edge) const {
		const auto [start, stop] = graph.equal_range(edge->second);
		return std::max_element(start, stop, [&](const auto &edge1, const auto &edge2) {
			return edge1 || *edge ? true : edge2 || *edge ? false : atan2(*edge, edge1) < atan2(*edge, edge2);
		});
	}

	auto next_exterior(const EdgeIterator &edge) const {
		const auto [start, stop] = graph.equal_range(edge->second);
		return std::min_element(start, stop, [&](const auto &edge1, const auto &edge2) {
			return edge1 || *edge ? false : edge2 || *edge ? true : atan2(*edge, edge1) < atan2(*edge, edge2);
		});
	}

	struct Iterator {
		const Mesh &mesh;
		EdgeIterator edge;
		bool interior, forward;

		Iterator(const Mesh &mesh, const EdgeIterator &edge, bool interior, bool forward) : mesh(mesh), edge(edge), interior(interior), forward(forward) { }
		auto peek() const { return interior == forward ? mesh.next_interior(edge) : mesh.next_exterior(edge); }
		auto &operator++() { edge = peek(); return *this; }
		auto &reverse() { forward = !forward; return ++(*this); }
		auto operator*() const { return *edge; }
		auto &operator->() const { return edge; }
	};

public:
	template <typename LessThan>
	auto rightmost(LessThan less_than) const {
		const auto &[p1, p2] = *std::max_element(graph.begin(), graph.end(), [&](const auto &edge1, const auto &edge2) {
			return less_than(edge1.first, edge2.first);
		});
		const auto [start, stop] = graph.equal_range(p1);
		const auto edge = std::min_element(start, stop, [](const auto &edge1, const auto &edge2) {
			return (edge1 ^ edge2) < 0;
		});
		return Iterator(*this, edge, false, false);
	}

	template <typename LessThan>
	auto leftmost(LessThan less_than) const {
		const auto &[p1, p2] = *std::min_element(graph.begin(), graph.end(), [&](const auto &edge1, const auto &edge2) {
			return less_than(edge1.first, edge2.first);
		});
		const auto [start, stop] = graph.equal_range(p1);
		const auto edge = std::max_element(start, stop, [](const auto &edge1, const auto &edge2) {
			return (edge1 ^ edge2) < 0;
		});
		return Iterator(*this, edge, false, true);
	}

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
