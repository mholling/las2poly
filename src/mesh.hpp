#ifndef MESH_HPP
#define MESH_HPP

#include "point.hpp"
#include "edge.hpp"
#include "triangle.hpp"
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <functional>
#include <array>

class Mesh : std::unordered_multimap<const Point &, const Point &> {
	using EdgeIterator = const_iterator;

	static auto less_than(const Edge &edge, const Edge &edge1, const Edge &edge2) {
		auto cross1 = edge ^ edge1, dot1 = edge * edge1;
		auto cross2 = edge ^ edge2, dot2 = edge * edge2;
		return cross1 < 0
			? cross2 > 0 || (dot1 < 0 ? dot2 > 0 || cross1 * dot2 < cross2 * dot1 : dot2 > 0 && cross1 * dot2 < cross2 * dot1)
			: cross2 > 0 && (dot1 < 0 ? dot2 < 0 && cross1 * dot2 < cross2 * dot1 : dot2 < 0 || cross1 * dot2 < cross2 * dot1);
	}

	auto next_interior(const EdgeIterator &edge) const {
		const auto [start, stop] = equal_range(edge->second);
		auto next = std::max_element(start, stop, [&](const auto &edge1, const auto &edge2) {
			return edge1 || *edge ? true : edge2 || *edge ? false : less_than(*edge, edge1, edge2);
		});
		if (next == stop)
			throw std::runtime_error("unexpected");
		return next;
	}

	auto next_exterior(const EdgeIterator &edge) const {
		const auto [start, stop] = equal_range(edge->second);
		auto next = std::min_element(start, stop, [&](const auto &edge1, const auto &edge2) {
			return edge1 || *edge ? false : edge2 || *edge ? true : less_than(*edge, edge1, edge2);
		});
		if (next == stop)
			throw std::runtime_error("unexpected");
		return next;
	}

	auto opposing(const EdgeIterator &edge) const {
		const auto [start, stop] = equal_range(edge->second);
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
		auto &reverse() { interior = !interior; edge = mesh.opposing(edge); return *this; }
		auto operator*() const { return *edge; }
		auto &operator->() const { return edge; }
		operator EdgeIterator() const { return edge; }
		auto search() const { return Iterator(mesh, peek(), !interior).peek(); }
	};

	template <typename LessThan>
	auto rightmost_clockwise(LessThan less_than) const {
		const auto &[p1, p2] = *std::max_element(begin(), end(), [&](const auto &edge1, const auto &edge2) {
			return less_than(edge1.first, edge2.first);
		});
		const auto [start, stop] = equal_range(p1);
		const auto edge = std::min_element(start, stop, [](const auto &edge1, const auto &edge2) {
			return (edge1 ^ edge2) < 0;
		});
		return Iterator(*this, edge, true);
	}

	template <typename LessThan>
	auto leftmost_anticlockwise(LessThan less_than) const {
		const auto &[p1, p2] = *std::min_element(begin(), end(), [&](const auto &edge1, const auto &edge2) {
			return less_than(edge1.first, edge2.first);
		});
		const auto [start, stop] = equal_range(p1);
		const auto edge = std::max_element(start, stop, [](const auto &edge1, const auto &edge2) {
			return (edge1 ^ edge2) < 0;
		});
		return Iterator(*this, edge, false);
	}

	void connect(const Point &p1, const Point &p2) {
		emplace(p1, p2);
		emplace(p2, p1);
	}

	void connect(const Point &p1, const Point &p2, const Point &p3) {
		if (((p3 - p2) ^ (p2 - p1)) != 0) {
			connect(p1, p2);
			connect(p2, p3);
			connect(p3, p1);
		} else if (p1 < p2 == p2 < p3) {
			connect(p1, p2);
			connect(p2, p3);
		} else if (p2 < p1 == p1 < p3) {
			connect(p3, p1);
			connect(p1, p2);
		} else {
			connect(p2, p3);
			connect(p3, p1);
		}
	}

	void disconnect(const Point &p1, const Point &p2) {
		auto [start1, stop1] = equal_range(p1);
		erase(std::find(start1, stop1, Edge(p1, p2)));
		auto [start2, stop2] = equal_range(p2);
		erase(std::find(start2, stop2, Edge(p2, p1)));
	}

	auto &operator+=(Mesh &mesh) {
		merge(mesh);
		return *this;
	}

	template <typename TriangleFunction, typename EdgeFunction>
	void deconstruct(TriangleFunction yield_triangle, EdgeFunction yield_edge) {
		auto edge = rightmost_clockwise(std::less());
		const auto &point = edge->first;
		while (true) {
			yield_edge(-*edge);
			erase(edge);
			if (edge->second == point)
				break;
			++edge;
		}
		while (!empty()) {
			auto edge = Iterator(*this, begin(), true);
			std::array edges = {edge, ++edge, ++edge};
			if (edges[0]->first != edges[2]->second || (*edges[0] ^ *edges[1]) < 0 || (*edges[1] ^ *edges[2]) < 0 || (*edges[2] ^ *edges[0]) < 0)
				throw std::runtime_error("corrupted edge graph (" + std::to_string(size()) + " edges left)");
			yield_triangle(Triangle({*edges[0], *edges[1], *edges[2]}));
			erase(edges[0]);
			erase(edges[1]);
			erase(edges[2]);
		}
	}
};

#endif
