#ifndef MESH_HPP
#define MESH_HPP

#include "point.hpp"
#include "edge.hpp"
#include "triangle.hpp"
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <functional>
#include <array>

class Mesh {
	using Graph = std::unordered_multimap<const Point &, const Point &>;
	using EdgeIterator = typename Graph::const_iterator;
	Graph graph;

	static auto less_than(const Edge &edge, const Edge &edge1, const Edge &edge2) {
		auto cross1 = edge ^ edge1, dot1 = edge * edge1;
		auto cross2 = edge ^ edge2, dot2 = edge * edge2;
		return cross1 < 0
			? cross2 > 0 || (dot1 < 0 ? dot2 > 0 || cross1 * dot2 < cross2 * dot1 : dot2 > 0 && cross1 * dot2 < cross2 * dot1)
			: cross2 > 0 && (dot1 < 0 ? dot2 < 0 && cross1 * dot2 < cross2 * dot1 : dot2 < 0 || cross1 * dot2 < cross2 * dot1);
	}

	auto next_interior(const EdgeIterator &edge) const {
		const auto [start, stop] = graph.equal_range(edge->second);
		auto next = std::max_element(start, stop, [&](const auto &edge1, const auto &edge2) {
			return edge1 || *edge ? true : edge2 || *edge ? false : less_than(*edge, edge1, edge2);
		});
		if (next == stop)
			throw std::runtime_error("unexpected");
		return next;
	}

	auto next_exterior(const EdgeIterator &edge) const {
		const auto [start, stop] = graph.equal_range(edge->second);
		auto next = std::min_element(start, stop, [&](const auto &edge1, const auto &edge2) {
			return edge1 || *edge ? false : edge2 || *edge ? true : less_than(*edge, edge1, edge2);
		});
		if (next == stop)
			throw std::runtime_error("unexpected");
		return next;
	}

	auto opposing(const EdgeIterator &edge) const {
		const auto [start, stop] = graph.equal_range(edge->second);
		return std::find(start, stop, -*edge);
	}

public:
	struct Iterator {
		const Mesh &mesh;
		EdgeIterator edge;
		bool interior;

		Iterator(const Mesh &mesh, const EdgeIterator &edge, bool interior) : mesh(mesh), edge(edge), interior(interior) { }
		auto peek() const { return interior ? mesh.next_interior(edge) : mesh.next_exterior(edge); }
		auto &operator++() { edge = peek(); return *this; }
		auto operator++(int) { auto old = *this; ++(*this); return old; }
		auto &reverse() { interior = !interior; edge = mesh.opposing(edge); return *this; }
		auto operator==(const Iterator &other) const { return edge == other.edge; }
		auto operator*() const { return *edge; }
		auto &operator->() const { return edge; }
		operator EdgeIterator() const { return edge; }
	};

	template <typename LessThan>
	auto rightmost_clockwise(LessThan less_than) const {
		const auto &[p1, p2] = *std::max_element(graph.begin(), graph.end(), [&](const auto &edge1, const auto &edge2) {
			return less_than(edge1.first, edge2.first);
		});
		const auto [start, stop] = graph.equal_range(p1);
		const auto edge = std::min_element(start, stop, [](const auto &edge1, const auto &edge2) {
			return (edge1 ^ edge2) < 0;
		});
		return Iterator(*this, edge, true);
	}

	template <typename LessThan>
	auto leftmost_anticlockwise(LessThan less_than) const {
		const auto &[p1, p2] = *std::min_element(graph.begin(), graph.end(), [&](const auto &edge1, const auto &edge2) {
			return less_than(edge1.first, edge2.first);
		});
		const auto [start, stop] = graph.equal_range(p1);
		const auto edge = std::max_element(start, stop, [](const auto &edge1, const auto &edge2) {
			return (edge1 ^ edge2) < 0;
		});
		return Iterator(*this, edge, false);
	}

	auto dangling(const Point &point) const {
		return graph.count(point) < 2;
	}

	void connect(const Point &p1, const Point &p2) {
		graph.emplace(p1, p2);
		graph.emplace(p2, p1);
	}

	void disconnect(const Point &p1, const Point &p2) {
		auto [start1, stop1] = graph.equal_range(p1);
		graph.erase(std::find(start1, stop1, Edge(p1, p2)));
		auto [start2, stop2] = graph.equal_range(p2);
		graph.erase(std::find(start2, stop2, Edge(p2, p1)));
	}

	auto &operator+=(Mesh &mesh) {
		graph.merge(mesh.graph);
		return *this;
	}

	template <typename TriangleFunction, typename EdgeFunction>
	void deconstruct(TriangleFunction yield_triangle, EdgeFunction yield_edge) {
		auto edge = rightmost_clockwise(std::less());
		const auto &point = edge->first;
		while (true) {
			yield_edge(-*edge);
			graph.erase(edge);
			if (edge->second == point)
				break;
			++edge;
		}
		while (!graph.empty()) {
			auto edge = Iterator(*this, graph.begin(), true);
			std::array edges = {edge++, edge++, edge};
			yield_triangle(Triangle({*edges[0], *edges[1], *edges[2]}));
			graph.erase(edges[0]);
			graph.erase(edges[1]);
			graph.erase(edges[2]);
		}
	}
};

#endif
