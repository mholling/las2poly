#ifndef TRIANGULATION_HPP
#define TRIANGULATION_HPP

#include "point.hpp"
#include "mesh.hpp"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <thread>

class Triangulation : public Mesh {
	using Points = std::vector<Point>;
	using Iterator = typename Points::iterator;

	template <bool rhs>
	static Point const *find_candidate(Mesh &mesh, const Mesh::Iterator &edge, const Point &opposite) {
		const auto &point = edge->second;
		while (true) {
			const auto &[candidate, next] = *edge.search();
			auto cross_product = (point - opposite) ^ (candidate - point);
			if (rhs ? cross_product <= 0 : cross_product >= 0)
				return nullptr;
			if (candidate == edge->first)
				return &candidate;
			if (!next.in_circle(rhs ? candidate : point, opposite, rhs ? point : candidate))
				return &candidate;
			mesh.disconnect(point, candidate);
		}
	}

	template <bool horizontal = true>
	static void triangulate(Mesh &mesh, Iterator first, Iterator last, int threads) {
		auto less_than = [](const Point &p1, const Point &p2) {
			if constexpr (horizontal)
				return p1[0] < p2[0] ? true : p1[0] > p2[0] ? false : p1[1] < p2[1];
			else
				return p1[1] < p2[1] ? true : p1[1] > p2[1] ? false : p1[0] > p2[0];
		};

		switch (last - first) {
		case 0:
		case 1:
			throw std::runtime_error("not enough points");

		case 2:
			mesh.connect(*(first+1), *(first+0));
			break;

		case 3:
			mesh.connect(*(first+2), *(first+1), *(first+0));
			break;

		default:
			auto middle = first + (last - first) / 2;
			std::nth_element(first, middle, last, less_than);
			Mesh left_mesh, right_mesh;

			if (threads > 1) {
				auto left_thread = std::thread([&]() {
					triangulate<!horizontal>(left_mesh, first, middle, threads/2);
				}), right_thread = std::thread([&]() {
					triangulate<!horizontal>(right_mesh, middle, last, threads - threads/2);
				});
				left_thread.join(), right_thread.join();
			} else {
				triangulate<!horizontal>(left_mesh, first, middle, 1);
				triangulate<!horizontal>(right_mesh, middle, last, 1);
			}

			const auto &rightmost = *std::max_element(first, middle, less_than);
			const auto &leftmost = *std::min_element(middle, last, less_than);
			auto left_edge = left_mesh.exterior_clockwise(rightmost);
			auto right_edge = right_mesh.exterior_anticlockwise(leftmost);

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

public:
	Triangulation(Points &points, int threads) {
		triangulate(*this, points.begin(), points.end(), threads);
	}
};

#endif
