#ifndef TIN_HPP
#define TIN_HPP

#include "point.hpp"
#include "thinned.hpp"
#include "mesh.hpp"
#include <utility>
#include <algorithm>
#include <stdexcept>

class TIN {
	Thinned points;

	template <typename Iterator, int axis = 0>
	class Node {
		using Child = Node<Iterator, 1-axis>;
		const Iterator first, middle, last;

		static auto less_than(const Point &p1, const Point &p2) {
			return p1[axis] < p2[axis] ? true : p1[axis] > p2[axis] ? false : p1[1-axis] < p2[1-axis];
		}

		template <typename EdgeIterator>
		static auto find_candidate(Mesh &mesh, const EdgeIterator &edge, const Point &opposite, bool right_side) {
			const auto &point = edge->first;
			while (true) {
				auto candidate = edge.previous()->first;
				auto cross_product = (point - opposite) ^ (candidate - opposite);
				if (cross_product < 0 == right_side)
					return std::optional<Point>();
				mesh.disconnect(point, candidate);
				auto next_candidate = edge.previous()->first;
				if (next_candidate.in_circle(right_side ? candidate : point, opposite, right_side ? point : candidate))
					continue;
				mesh.connect(point, candidate);
				return std::optional<Point>(candidate);
			}
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
			case 3:
				mesh.connect(*(first+0), *(first+2));
				mesh.connect(*(first+2), *(first+1));
			case 2:
				mesh.connect(*(first+1), *(first+0));
				break;
			default:
				auto left_mesh = Child(first, middle).triangulate();
				auto right_mesh = Child(middle, last).triangulate();
				const auto left_exterior = left_mesh.exterior();
				const auto right_exterior = right_mesh.exterior();
				auto left_edge = left_exterior.rightmost(less_than);
				auto right_edge = right_exterior.leftmost(less_than);

				auto check_right = [&]() {
					return (*right_edge ^ left_edge->first) > 0;
				};
				auto check_left = [&]() {
					return (*left_edge ^ right_edge->first) < 0;
				};

				while (!check_right() && !check_left()) {
					if (!check_right())
						++right_edge;
					if (!check_left())
						++left_edge;
				}

				while (true) {
					const auto &left_point = left_edge->first;
					const auto &right_point = right_edge->first;
					mesh.connect(left_point, right_point);

					auto left_candidate = find_candidate(left_mesh, left_edge, right_point, false);
					auto right_candidate = find_candidate(right_mesh, right_edge, left_point, true);

					if (left_candidate && right_candidate) {
						if (left_candidate.value().in_circle(left_point, right_point, right_candidate.value()))
							right_candidate.reset();
						else
							left_candidate.reset();
					}

					if (left_candidate) {
						left_mesh.connect(left_point, left_candidate.value());
						--left_edge;
					} else if (right_candidate) {
						right_mesh.connect(right_point, right_candidate.value());
						--right_edge;
					} else
						break;
				}

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
