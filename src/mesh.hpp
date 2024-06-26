////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef MESH_HPP
#define MESH_HPP

#include "point.hpp"
#include "points.hpp"
#include "edge.hpp"
#include "circle.hpp"
#include "triangles.hpp"
#include "triangle.hpp"
#include "rtree.hpp"
#include "app.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <optional>
#include <thread>
#include <optional>
#include <cmath>

class Mesh : std::vector<std::vector<PointIterator>> {
	Points &points;

	auto &adjacent(PointIterator point) {
		return (*this)[point - points.begin()];
	}

	auto &adjacent(PointIterator point) const {
		return (*this)[point - points.begin()];
	}

	void connect(PointIterator p1, PointIterator p2) {
		adjacent(p1).push_back(p2);
		adjacent(p2).push_back(p1);
	}

	void disconnect(Edge const &edge) {
		auto &neighbours = adjacent(edge.first);
		neighbours.erase(std::find(neighbours.begin(), neighbours.end(), edge.second));
	}

	void disconnect(PointIterator p1, PointIterator p2) {
		disconnect({p1, p2});
		disconnect({p2, p1});
	}

	auto static less_than(Edge const &edge, PointIterator p1, PointIterator p2) {
		return edge < p1
			? edge > p2 || Edge(p1, p2) > edge.second
			: edge > p2 && Edge(p1, p2) > edge.second;
	}

	auto next_interior(Edge const &edge) {
		auto const &neighbours = adjacent(edge.second);
		auto const next = std::max_element(neighbours.begin(), neighbours.end(), [&](auto const &p1, auto const &p2) {
			return p1 == edge.first ? true : p2 == edge.first ? false : less_than(edge, p1, p2);
		});
		if (next == neighbours.end())
			throw std::runtime_error("unexpected");
		return Edge(edge.second, *next);
	}

	auto next_exterior(Edge const &edge) {
		auto const &neighbours = adjacent(edge.second);
		auto const next = std::min_element(neighbours.begin(), neighbours.end(), [&](auto const &p1, auto const &p2) {
			return p1 == edge.first ? false : p2 == edge.first ? true : less_than(edge, p1, p2);
		});
		if (next == neighbours.end())
			throw std::runtime_error("unexpected");
		return Edge(edge.second, *next);
	}

	struct Iterator {
		Mesh &mesh;
		Edge edge;
		bool interior;

		Iterator(Mesh &mesh, Edge edge, bool interior) :
			mesh(mesh),
			edge(edge),
			interior(interior)
		{ }

		auto peek() const {
			return interior ? mesh.next_interior(edge) : mesh.next_exterior(edge);
		}

		auto &operator++() {
			edge = peek();
			return *this;
		}

		auto &reverse() {
			interior = !interior;
			edge = -edge;
			return *this;
		}

		auto &operator*() const {
			return edge;
		}

		auto operator->() const {
			return &edge;
		}

		auto search() const {
			return Iterator(mesh, peek(), !interior).peek();
		}
	};

	auto exterior_clockwise(PointIterator rightmost) {
		auto const &neighbours = adjacent(rightmost);
		auto const next = std::min_element(neighbours.begin(), neighbours.end(), [&](auto const &p1, auto const &p2) {
			return Edge(p1, p2) < rightmost;
		});
		return Iterator(*this, Edge(rightmost, *next), true);
	}

	auto exterior_anticlockwise(PointIterator leftmost) {
		auto const &neighbours = adjacent(leftmost);
		auto const next = std::max_element(neighbours.begin(), neighbours.end(), [&](auto const &p1, auto const &p2) {
			return Edge(p1, p2) < leftmost;
		});
		return Iterator(*this, Edge(leftmost, *next), false);
	}

	auto exterior_clockwise(PointIterator begin, PointIterator end) {
		auto const rightmost = std::max_element(begin, end);
		return exterior_clockwise(rightmost);
	}

	auto exterior_anticlockwise(PointIterator begin, PointIterator end) {
		auto const leftmost = std::min_element(begin, end);
		return exterior_anticlockwise(leftmost);
	}

	template <bool rhs>
	auto find_candidate(Iterator const &edge, PointIterator opposite) {
		auto const &[prev, point] = *edge;
		while (true) {
			auto const [candidate, next] = edge.search();
			auto const orientation = Edge(point, candidate) <=> opposite;
			if (rhs ? orientation <= 0 : orientation >= 0)
				return std::optional<PointIterator>();
			if (candidate == prev)
				return std::optional<PointIterator>(candidate);
			if (Circle(rhs ? candidate : point, opposite, rhs ? point : candidate) <= next)
				return std::optional<PointIterator>(candidate);
			disconnect(point, candidate);
		}
	}

	template <bool horizontal = true>
	void triangulate(PointIterator begin, PointIterator end, int threads) {
		auto static constexpr less_than = [](Point const &p1, Point const &p2) {
			if constexpr (horizontal)
				return p1[0] < p2[0] ? true : p1[0] > p2[0] ? false : p1[1] < p2[1];
			else
				return p1[1] < p2[1] ? true : p1[1] > p2[1] ? false : p1[0] > p2[0];
		};
		auto const middle = begin + (end - begin) / 2;
		std::nth_element(begin, middle, end, less_than);

		switch (end - begin) {
		case 0:
		case 1:
			break;
		case 3:
			if (Edge(begin+2, begin+1) <=> begin != 0)
				connect(begin, begin+2);
			connect(begin+2, begin+1);
			[[fallthrough]];
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
			auto const rightmost = std::max_element(begin, middle, less_than);
			auto const leftmost = std::min_element(middle, end, less_than);
			auto left = exterior_clockwise(rightmost);
			auto right = exterior_anticlockwise(leftmost);
			while (true) {
				auto const tangent = Edge(left->first, right->first);
				if (tangent < right->second)
					++right;
				else if (tangent < left->second)
					++left;
				else
					break;
			}
			left.reverse(), right.reverse();
			auto pairs = std::vector<Edge>();
			while (true) {
				auto const &left_point = left->second;
				auto const &right_point = right->second;
				pairs.emplace_back(left_point, right_point);
				auto const left_candidate = find_candidate<false>(left, right_point);
				auto const right_candidate = find_candidate<true>(right, left_point);
				if (left_candidate && right_candidate)
					Circle(left_point, right_point, *right_candidate) > *left_candidate ? ++left : ++right;
				else if (left_candidate)
					++left;
				else if (right_candidate)
					++right;
				else
					break;
			}
			for (auto const &[p1, p2]: pairs)
				connect(p1, p2);
		}
	}

	void deconstruct(Triangles &triangles, PointIterator begin, PointIterator end, double width, bool anticlockwise, int threads) {
		if (threads > 1) {
			auto const middle = begin + (end - begin) / 2;
			auto left_triangles = Triangles();
			auto right_triangles = Triangles();
			auto left_thread = std::thread([&]() {
				deconstruct(left_triangles, begin, middle, width, anticlockwise, threads/2);
			}), right_thread = std::thread([&]() {
				deconstruct(right_triangles, middle, end, width, anticlockwise, threads - threads/2);
			});
			left_thread.join(), right_thread.join();
			triangles.merge(left_triangles);
			triangles.merge(right_triangles);
		}
		for (auto point = begin; point < end; ++point)
			for (auto const neighbours = adjacent(point); auto const &neighbour: neighbours) {
				auto const edge1 = Iterator(*this, Edge(point, neighbour), anticlockwise);
				if (edge1->second < begin || !(edge1->second < end))
					continue;
				auto const edge2 = Iterator(*this, edge1.peek(), anticlockwise);
				if (edge2->second < begin || !(edge2->second < end))
					continue;
				auto const edge3 = Iterator(*this, edge2.peek(), anticlockwise);
				if (edge3->second != point)
					throw std::runtime_error("corrupted mesh");
				auto const triangle = Triangle{{*edge1, *edge2, *edge3}};
				if (triangle > width)
					triangles.insert(triangle);
				for (auto const &edge: triangle)
					disconnect(edge);
			}
	}

	template <typename ...Functions>
	void strip_exterior(PointIterator begin, PointIterator end, bool anticlockwise, Functions const &...functions) {
		if (end - begin < 2)
			throw std::runtime_error("not enough points");
		auto const start = anticlockwise ? exterior_clockwise(begin, end) : exterior_anticlockwise(begin, end);
		for (auto edge = start; ; ++edge) {
			(functions(*edge), ...);
			disconnect(*edge);
			if (edge->second == start->first)
				break;
		}
	}

	using RTree = ::RTree<PointIterator>;

	void interpolate(PointIterator begin, PointIterator end, RTree const &rtree, int threads) {
		if (threads > 1) {
			auto const middle = begin + (end - begin) / 2;
			auto left_thread = std::thread([&]() {
				interpolate(begin, middle, rtree, threads/2);
			}), right_thread = std::thread([&]() {
				interpolate(middle, end, rtree, threads - threads/2);
			});
			left_thread.join(), right_thread.join();
		}
		for (auto point = begin; point < end; ++point)
			for (auto const neighbours = adjacent(point); auto const &neighbour: neighbours) {
				auto const edge1 = Iterator(*this, Edge(point, neighbour), true);
				if (edge1->second < begin || !(edge1->second < end))
					continue;
				auto const edge2 = Iterator(*this, edge1.peek(), true);
				if (edge2->second < begin || !(edge2->second < end))
					continue;
				auto const edge3 = Iterator(*this, edge2.peek(), true);
				if (edge3->second != point)
					throw std::runtime_error("corrupted mesh");
				auto const &p1 = edge1->first;
				auto const &p2 = edge2->first;
				auto const &p3 = edge3->first;
				auto const bounds = Bounds(p1) + Bounds(p2) + Bounds(p3);
				for (auto const &point: rtree.search(bounds)) {
					auto const w1 = (*edge2 ^ point) / (*edge2 ^ p1);
					auto const w2 = (*edge3 ^ point) / (*edge3 ^ p2);
					auto const w3 = (*edge1 ^ point) / (*edge1 ^ p3);
					if (w1 >= 0 && w2 >= 0 && w3 >= 0)
						point->ground(w1 * p1->elevation + w2 * p2->elevation + w3 * p3->elevation);
				}
				disconnect(*edge1);
				disconnect(*edge2);
				disconnect(*edge3);
			}
	}

	void interpolate(PointIterator ground_begin, PointIterator ground_end, int threads) {
		auto const rtree = RTree(ground_end, points.end(), threads);
		strip_exterior(ground_begin, ground_end, true);
		interpolate(ground_begin, ground_end, rtree, threads);
	}

public:
	Mesh(Points &points) :
		vector(points.size()),
		points(points)
	{
		triangulate(points.begin(), points.end(), 1);
	}

	Mesh(App const &app, Points &points) :
		vector(points.size()),
		points(points)
	{
		auto const ground_begin = std::partition(points.begin(), points.end(), [](auto const &point) {
			return point.synthetic();
		});
		auto const ground_end = std::partition(ground_begin, points.end(), [](auto const &point) {
			return point.ground();
		});

		app.log("triangulating", ground_end - ground_begin, "point");
		triangulate(ground_begin, ground_end, app.threads);

		app.log("interpolating", points.end() - ground_end, "point");
		interpolate(ground_begin, ground_end, app.threads);

		app.log("triangulating", points.size(), "point");
		triangulate(points.begin(), points.end(), app.threads);
	}

	template <typename Edges>
	void deconstruct(App const &app, Triangles &triangles, Edges &edges) {
		strip_exterior(points.begin(), points.end(), app.land, [&](auto const &edge) {
			edges.insert(-edge);
		});
		deconstruct(triangles, points.begin(), points.end(), *app.width, app.land, app.threads);
	}

	auto median_length() const {
		auto lengths = std::vector<double>();
		lengths.reserve(size() * 6);

		for (auto p0 = points.begin(); p0 < points.end(); ++p0)
			for (auto const &p1: adjacent(p0))
				lengths.push_back((*p1 - *p0).sqnorm());

		auto const begin = lengths.begin(), end = lengths.end();
		auto const median = begin + (end - begin) / 2;
		std::nth_element(begin, median, end);

		return median == end ?
			std::optional<double>() :
			std::optional(std::sqrt(*median));
	}
};

#endif
