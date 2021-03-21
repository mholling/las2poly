#ifndef MESH_HPP
#define MESH_HPP

#include "point.hpp"
#include "points.hpp"
#include "edge.hpp"
#include "triangle.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <thread>

class Mesh : std::vector<std::vector<PointIterator>> {
	PointIterator points_begin;

	auto &adjacent(const PointIterator &point) {
		return (*this)[point - points_begin];
	}

	void connect(const PointIterator &p1, const PointIterator &p2) {
		adjacent(p1).push_back(p2);
		adjacent(p2).push_back(p1);
	}

	void disconnect(const Edge &edge) {
		auto &neighbours = adjacent(edge.first);
		neighbours.erase(std::find(neighbours.begin(), neighbours.end(), edge.second));
	}

	void disconnect(const PointIterator &p1, const PointIterator &p2) {
		disconnect({p1, p2});
		disconnect({p2, p1});
	}

	static auto less_than(const Edge &edge, const Edge &edge1, const Edge &edge2) {
		const auto cross1 = edge ^ edge1, dot1 = edge * edge1;
		const auto cross2 = edge ^ edge2, dot2 = edge * edge2;
		// return std::atan2(cross1, dot1) < std::atan2(cross2, dot2);
		return cross1 < 0
			? cross2 > 0 || (dot1 < 0 ? dot2 > 0 || cross1 * dot2 < cross2 * dot1 : dot2 > 0 && cross1 * dot2 < cross2 * dot1)
			: cross2 > 0 && (dot1 < 0 ? dot2 < 0 && cross1 * dot2 < cross2 * dot1 : dot2 < 0 || cross1 * dot2 < cross2 * dot1);
	}

	auto next_interior(const Edge &edge) {
		const auto &neighbours = adjacent(edge.second);
		const auto next = std::max_element(neighbours.begin(), neighbours.end(), [&](const auto &p1, const auto &p2) {
			return p1 == edge.first ? true : p2 == edge.first ? false : less_than(edge, Edge(edge.second, p1), Edge(edge.second, p2));
		});
		if (next == neighbours.end())
			throw std::runtime_error("unexpected");
		return Edge(edge.second, *next);
	}

	auto next_exterior(const Edge &edge) {
		const auto &neighbours = adjacent(edge.second);
		const auto next = std::min_element(neighbours.begin(), neighbours.end(), [&](const auto &p1, const auto &p2) {
			return p1 == edge.first ? false : p2 == edge.first ? true : less_than(edge, Edge(edge.second, p1), Edge(edge.second, p2));
		});
		if (next == neighbours.end())
			throw std::runtime_error("unexpected");
		return Edge(edge.second, *next);
	}

	struct Iterator {
		Mesh &mesh;
		Edge edge;
		bool interior;

		Iterator(Mesh &mesh, Edge edge, bool interior) : mesh(mesh), edge(edge), interior(interior) { }
		auto peek() const { return interior ? mesh.next_interior(edge) : mesh.next_exterior(edge); }
		auto &operator++() { edge = peek(); return *this; }
		auto &reverse() { interior = !interior; edge = -edge; return *this; }
		auto &operator*() const { return edge; }
		auto operator->() const { return &edge; }
		auto search() const { return Iterator(mesh, peek(), !interior).peek(); }
	};

	auto exterior_clockwise(const PointIterator &rightmost) {
		const auto &neighbours = adjacent(rightmost);
		const auto next = std::min_element(neighbours.begin(), neighbours.end(), [&](const auto &p1, const auto &p2) {
			return (Edge(rightmost, p1) ^ Edge(rightmost, p2)) < 0;
		});
		return Iterator(*this, Edge(rightmost, *next), true);
	}

	auto exterior_anticlockwise(const PointIterator &leftmost) {
		const auto &neighbours = adjacent(leftmost);
		const auto next = std::max_element(neighbours.begin(), neighbours.end(), [&](const auto &p1, const auto &p2) {
			return (Edge(leftmost, p1) ^ Edge(leftmost, p2)) < 0;
		});
		return Iterator(*this, Edge(leftmost, *next), false);
	}

	template <bool rhs>
	Point const *find_candidate(const Iterator &edge, const PointIterator &opposite) {
		const auto &[prev, point] = *edge;
		while (true) {
			const auto [candidate, next] = edge.search();
			const auto cross_product = Edge(opposite, point) ^ Edge(point, candidate);
			if (rhs ? cross_product <= 0 : cross_product >= 0)
				return nullptr;
			if (candidate == prev)
				return &*candidate;
			if (!next->in_circle(rhs ? *candidate : *point, *opposite, rhs ? *point : *candidate))
				return &*candidate;
			disconnect(point, candidate);
		}
	}

	template <bool horizontal = true>
	void triangulate(PointIterator begin, PointIterator end, int threads) {
		auto less_than = [](const Point &p1, const Point &p2) {
			if constexpr (horizontal)
				return p1[0] < p2[0] ? true : p1[0] > p2[0] ? false : p1[1] < p2[1];
			else
				return p1[1] < p2[1] ? true : p1[1] > p2[1] ? false : p1[0] > p2[0];
		};
		const auto middle = begin + (end - begin) / 2;
		std::nth_element(begin, middle, end, less_than);

		switch (end - begin) {
		case 0:
		case 1:
			throw std::runtime_error("not enough points");
		case 3:
			if ((Edge(begin+2, begin+1) ^ Edge(begin+1, begin)) != 0)
				connect(begin, begin+2);
			connect(begin+2, begin+1);
		case 2:
			connect(begin+1, begin);
			break;
		default:
			if (threads > 1) {
				auto left_thread = std::thread([&]() {
					triangulate<!horizontal>(begin, middle, threads/2);
				}), right_thread = std::thread([&]() {
					triangulate<!horizontal>(middle, end, threads - threads/2);
				});
				left_thread.join(), right_thread.join();
			} else {
				triangulate<!horizontal>(begin, middle, 1);
				triangulate<!horizontal>(middle, end, 1);
			}
			const auto rightmost = std::max_element(begin, middle, less_than);
			const auto leftmost = std::min_element(middle, end, less_than);
			auto left = exterior_clockwise(rightmost);
			auto right = exterior_anticlockwise(leftmost);
			while (true) {
				const auto tangent = Edge(left->first, right->first);
				if ((tangent ^ *right) < 0)
					++right;
				else if ((tangent ^ *left) < 0)
					++left;
				else
					break;
			}
			left.reverse(), right.reverse();
			auto pairs = std::vector<Edge>();
			while (true) {
				const auto &left_point = left->second;
				const auto &right_point = right->second;
				pairs.emplace_back(left_point, right_point);
				const auto left_candidate = find_candidate<false>(left, right_point);
				const auto right_candidate = find_candidate<true>(right, left_point);
				if (left_candidate && right_candidate)
					left_candidate->in_circle(*left_point, *right_point, *right_candidate) ? ++left : ++right;
				else if (left_candidate)
					++left;
				else if (right_candidate)
					++right;
				else
					break;
			}
			for (const auto &[p1, p2]: pairs)
				connect(p1, p2);
		}
	}

public:
	Mesh(Points &points, int threads) : vector(points.size()), points_begin(points.begin()) {
		triangulate(points.begin(), points.end(), threads);
	}

	template <typename TriangleFunction, typename EdgeFunction>
	void deconstruct(TriangleFunction yield_triangle, EdgeFunction yield_edge) {
		const auto rightmost = std::max_element(points_begin, points_begin + size());
		for (auto edge = exterior_clockwise(rightmost); ; ++edge) {
			yield_edge(-*edge);
			disconnect(*edge);
			if (edge->second == rightmost)
				break;
		}
		for (auto point = points_begin, points_end = points_begin + size(); point < points_end; ++point)
			for (auto &neighbours = adjacent(point); !neighbours.empty(); ) {
				auto edge = Iterator(*this, Edge(point, neighbours.front()), true);
				const Triangle triangle = {*edge, *++edge, *++edge};
				if (!triangle)
					throw std::runtime_error("corrupted mesh");
				yield_triangle(triangle);
				for (const auto &edge: triangle)
					disconnect(edge);
			}
	}
};

#endif
