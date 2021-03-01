#ifndef EXTERIOR_HPP
#define EXTERIOR_HPP

#include "point.hpp"
#include "edge.hpp"
#include <cmath>
#include <algorithm>

template <typename Graph>
class Exterior {
	const Graph &graph;

	static auto atan2(const Edge &edge1, const Edge &edge2) {
		return std::atan2(edge1 ^ edge2, edge1 * edge2);
	}

	const auto &next(const Edge &edge) const {
		const auto [start, stop] = graph.equal_range(edge.second);
		return *std::min_element(start, stop, [&](const auto &edge1, const auto &edge2) {
			return edge1 || edge ? false : edge2 || edge ? true : atan2(edge, edge1) < atan2(edge, edge2);
		});
	}

	const auto &prev(const Edge &edge) const {
		const auto [start, stop] = graph.equal_range(edge.second);
		return *std::max_element(start, stop, [&](const auto &edge1, const auto &edge2) {
			return edge1 || edge ? true : edge2 || edge ? false : atan2(edge, edge1) < atan2(edge, edge2);
		});
	}

	struct ForwardIterator {
		const Exterior &exterior;
		Edge edge;

		ForwardIterator(const Exterior &exterior, const Edge &edge) : exterior(exterior), edge(edge) { }
		auto &operator++() { edge = exterior.next(edge); return *this; }
		auto &operator--() { edge = -exterior.prev(edge); return *this; }
		auto previous() const { return ForwardIterator(exterior, -exterior.prev(edge)); }
		auto operator*() const { return edge; }
		auto operator->() const { return &edge; }
	};

	struct ReverseIterator {
		const Exterior &exterior;
		Edge edge;

		ReverseIterator(const Exterior &exterior, const Edge &edge) : exterior(exterior), edge(edge) { }
		auto &operator++() { edge = exterior.prev(edge); return *this; }
		auto &operator--() { edge = -exterior.next(edge); return *this; }
		auto previous() const { return ReverseIterator(exterior, -exterior.next(edge)); }
		auto operator*() const { return edge; }
		auto operator->() const { return &edge; }
	};

public:
	Exterior(const Graph &graph) : graph(graph) { }

	template <typename LessThan>
	auto rightmost(LessThan less_than) const {
		const auto &[p1, p2] = *std::max_element(graph.begin(), graph.end(), [&](const auto &edge1, const auto &edge2) {
			return less_than(edge1.first, edge2.first);
		});
		const auto [start, stop] = graph.equal_range(p1);
		const auto edge = *std::min_element(start, stop, [](const auto &edge1, const auto &edge2) {
			return (edge1 ^ edge2) < 0;
		});
		return ReverseIterator(*this, edge);
	}

	template <typename LessThan>
	auto leftmost(LessThan less_than) const {
		const auto &[p1, p2] = *std::min_element(graph.begin(), graph.end(), [&](const auto &edge1, const auto &edge2) {
			return less_than(edge1.first, edge2.first);
		});
		const auto [start, stop] = graph.equal_range(p1);
		const auto edge = *std::max_element(start, stop, [](const auto &edge1, const auto &edge2) {
			return (edge1 ^ edge2) < 0;
		});
		return ForwardIterator(*this, edge);
	}
};

#endif
