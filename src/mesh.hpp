#ifndef MESH_HPP
#define MESH_HPP

#include "point.hpp"
#include "edge.hpp"
#include "face.hpp"
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <utility>
#include <functional>
#include <array>

class Mesh {
	using Graph = std::unordered_multimap<Point, Point>;
	using EdgeIterator = typename Graph::const_iterator;
	Graph graph;

	struct DanglingEdge : std::runtime_error {
		DanglingEdge() : runtime_error("dangling edge") { }
	};

	static auto atan2(const Edge &edge1, const Edge &edge2) {
		return std::atan2(edge1 ^ edge2, edge1 * edge2);
	}

	auto next_interior(const EdgeIterator &edge) const {
		const auto [start, stop] = graph.equal_range(edge->second);
		auto next = std::max_element(start, stop, [&](const auto &edge1, const auto &edge2) {
			return edge1 || *edge ? true : edge2 || *edge ? false : atan2(*edge, edge1) < atan2(*edge, edge2);
		});
		if (next == stop)
			throw DanglingEdge();
		return next;
	}

	auto next_exterior(const EdgeIterator &edge) const {
		const auto [start, stop] = graph.equal_range(edge->second);
		auto next = std::min_element(start, stop, [&](const auto &edge1, const auto &edge2) {
			return edge1 || *edge ? false : edge2 || *edge ? true : atan2(*edge, edge1) < atan2(*edge, edge2);
		});
		if (next == stop)
			throw DanglingEdge();
		return next;
	}

	auto opposing(const EdgeIterator &edge) const {
		const auto [start, stop] = graph.equal_range(edge->second);
		return std::find(start, stop, -*edge);
	}

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
		return Iterator(*this, edge, true);
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
		return Iterator(*this, edge, false);
	}

	auto connected(const Point &point) const {
		return graph.count(point) > 1;
	}

	void connect(const Point &p1, const Point &p2) {
		graph.insert(Edge(p1, p2));
		graph.insert(Edge(p2, p1));
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

	template <typename FaceFunction, typename EdgeFunction>
	void deconstruct(FaceFunction yield_face, EdgeFunction yield_edge) {
		auto edge = rightmost(std::less());
		auto point = edge->first;
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
			yield_face(Face({*edges[0], *edges[1], *edges[2]}));
			graph.erase(edges[0]);
			graph.erase(edges[1]);
			graph.erase(edges[2]);
		}
	}
};

#endif
