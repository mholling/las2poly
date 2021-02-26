#ifndef TIN_HPP
#define TIN_HPP

#include "face.hpp"
#include "edge.hpp"
#include "point.hpp"
#include "mesh.hpp"
#include "thinned.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>

class TIN {
	template <typename Iterator, int axis = 0>
	class Node {
		using Child = Node<Iterator, 1-axis>;
		const Iterator first, middle, last;

		static auto less_than(const Point &p1, const Point &p2) {
			return p1[axis] < p2[axis] ? true : p1[axis] > p2[axis] ? false : p1[1-axis] < p2[1-axis];
		}

	public:
		Node(Iterator first, Iterator last) : first(first), last(last), middle(first + (last - first) / 2) {
			std::nth_element(first, middle, last, less_than);
		}

		Mesh triangulate() {
			Mesh mesh;
			switch (last - first) {
			case 0:
			case 1:
				throw std::runtime_error("not enough points");
			case 2:
				mesh += Edge(first);
				break;
			case 3:
				mesh += Face(first);
				break;
			default:
				auto left_mesh = Child(first, middle).triangulate();
				auto right_mesh = Child(middle, last).triangulate();
				auto left_edge = left_mesh.exterior().rightmost(less_than);
				auto right_edge = right_mesh.exterior().leftmost(less_than);
				auto check_right = [&]() {
					return (*right_edge ^ left_edge->p1) > 0;
				};
				auto check_left = [&]() {
					return (*left_edge ^ right_edge->p0) > 0;
				};
				while (!check_right() && !check_left()) {
					if (!check_right())
						++right_edge;
					if (!check_left())
						--left_edge;
				}
				auto edge = Edge(left_edge->p1, right_edge->p0);
				// ...
				mesh += left_mesh;
				mesh += right_mesh;
			}
			return mesh;
		}
	};

	static auto better_than(const Point &point1, const Point &point2) {
		return point1.is_ground()
			? point2.is_ground() ? point1[2] < point2[2] : true
			: point2.is_ground() ? false : point1[2] < point2[2];
	}

	Thinned<Point> points;

public:
	template <typename Tile>
	auto &operator+=(Tile tile) {
		for (const auto &point: tile)
			points.insert(point, better_than);
		return *this;
	}

	auto triangulate() {
		auto thinned = points.to_vector();
		return Node(thinned.begin(), thinned.end()).triangulate();
	}
};

#endif
