#ifndef TRIANGULATE_HPP
#define TRIANGULATE_HPP

#include "point.hpp"
#include "mesh.hpp"
#include <vector>
#include <optional>
#include <utility>
#include <algorithm>
#include <stdexcept>

class Triangulate {
	std::vector<Point> &points;

	template <typename ContainerIterator, int axis = 0>
	class Node {
		using Child = Node<ContainerIterator, 1-axis>;
		const ContainerIterator first, middle, last;

		static auto less_than(const Point &p1, const Point &p2) {
			return p1[axis] < p2[axis] ? true : p1[axis] > p2[axis] ? false : p1[1-axis] < p2[1-axis];
		}

		template <bool rhs>
		static auto find_candidate(Mesh &mesh, const Mesh::Iterator &edge, const Point &opposite) {
			const auto &point = edge->second;
			auto candidate = edge.peek()->second;
			auto cross_product = (point - opposite) ^ (candidate - opposite);
			if (cross_product < 0 == rhs)
				return std::optional<Point>();
			if (!mesh.connected(point))
				return std::optional<Point>(candidate);
			mesh.disconnect(point, candidate);
			auto next_candidate = edge.peek()->second;
			if (next_candidate.in_circle(rhs ? candidate : point, opposite, rhs ? point : candidate))
				return find_candidate<rhs>(mesh, edge, opposite);
			mesh.connect(point, candidate);
			return std::optional<Point>(candidate);
		}

	public:
		Node(ContainerIterator first, ContainerIterator last) : first(first), last(last), middle(first + (last - first) / 2) {
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
				auto left_edge = left_mesh.rightmost(less_than);
				auto right_edge = right_mesh.leftmost(less_than);

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

				left_edge.reverse();
				right_edge.reverse();

				while (true) {
					const auto &left_point = left_edge->second;
					const auto &right_point = right_edge->second;
					mesh.connect(left_point, right_point);

					auto left_candidate = find_candidate<false>(left_mesh, left_edge, right_point);
					auto right_candidate = find_candidate<true>(right_mesh, right_edge, left_point);

					if (left_candidate && right_candidate) {
						if (left_candidate.value().in_circle(left_point, right_point, right_candidate.value()))
							right_candidate.reset();
						else
							left_candidate.reset();
					}

					if (left_candidate)
						++left_edge;
					else if (right_candidate)
						++right_edge;
					else
						break;
				}

				mesh += left_mesh;
				mesh += right_mesh;
			}
			return mesh;
		}
	};

public:
	Triangulate(std::vector<Point> &points) : points(points) { }

	auto operator()() {
		return Node(points.begin(), points.end()).triangulate();
	}
};

#endif
