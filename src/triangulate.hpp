#ifndef TRIANGULATE_HPP
#define TRIANGULATE_HPP

#include "point.hpp"
#include "mesh.hpp"
#include <vector>
#include <algorithm>
#include <thread>
#include <stdexcept>

class Triangulate {
	using Points = std::vector<Point>;
	using Iterator = typename Points::iterator;

	Points &points;
	int threads;

	template <int axis = 0>
	class Partition {
		using Child = Partition<1-axis>;
		const Iterator first, middle, last;
		int threads;

		static auto less_than(const Point &p1, const Point &p2) {
			return p1[axis] < p2[axis] ? true : p1[axis] > p2[axis] ? false : p1[1-axis] < p2[1-axis];
		}

		void left_right(Mesh &left_mesh, Mesh &right_mesh) {
			if (threads > 1) {
				auto left_thread = std::thread([&]() {
					Child(first, middle, threads/2).triangulate(left_mesh);
				}), right_thread = std::thread([&]() {
					Child(middle, last, threads - threads/2).triangulate(right_mesh);
				});
				left_thread.join(), right_thread.join();
			} else {
				Child(first, middle, 1).triangulate(left_mesh);
				Child(middle, last, 1).triangulate(right_mesh);
			}
		}

		template <bool rhs>
		static Point const *find_candidate(Mesh &mesh, const Mesh::Iterator &edge, const Point &opposite) {
			const auto &point = edge->second;
			while (true) {
				const auto &[candidate, next] = *edge.back().peek();
				auto cross_product = (point - opposite) ^ (candidate - point);
				if (cross_product <= 0 == rhs)
					return nullptr;
				if (candidate == edge->first)
					return &candidate;
				if (!next.in_circle(rhs ? candidate : point, opposite, rhs ? point : candidate))
					return &candidate;
				mesh.disconnect(point, candidate);
			}
		}

	public:
		Partition(Iterator first, Iterator last, int threads) : first(first), last(last), middle(first + (last - first) / 2), threads(threads) {
			std::nth_element(first, middle, last, less_than);
		}

		void triangulate(Mesh &mesh) {
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
				Mesh left_mesh, right_mesh;
				left_right(left_mesh, right_mesh);
				auto left_edge = left_mesh.rightmost_clockwise(less_than);
				auto right_edge = right_mesh.leftmost_anticlockwise(less_than);
				while (true) {
					const auto &[l0, l1] = *left_edge;
					const auto &[r0, r1] = *right_edge;
					if (((r0 - l0) ^ (r1 - r0)) < 0)
						++right_edge;
					else if (((l0 - r0) ^ (l1 - l0)) > 0)
						++left_edge;
					else
						break;
				}
				left_edge.reverse();
				right_edge.reverse();
				while (true) {
					const auto &left_point = left_edge->second;
					const auto &right_point = right_edge->second;
					mesh.connect(left_point, right_point);
					auto left_candidate = find_candidate<false>(left_mesh, left_edge, right_point);
					auto right_candidate = find_candidate<true>(right_mesh, right_edge, left_point);
					if (left_candidate && right_candidate)
						left_candidate->in_circle(left_point, right_point, *right_candidate) ? ++left_edge : ++right_edge;
					else if (left_candidate)
						++left_edge;
					else if (right_candidate)
						++right_edge;
					else
						break;
				}
				mesh += left_mesh;
				mesh += right_mesh;
			}
		}
	};

public:
	Triangulate(std::vector<Point> &points, int threads) : points(points), threads(threads) { }

	auto operator()() {
		Mesh mesh;
		Partition(points.begin(), points.end(), threads).triangulate(mesh);
		return mesh;
	}
};

#endif
