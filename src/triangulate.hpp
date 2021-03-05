#ifndef TRIANGULATE_HPP
#define TRIANGULATE_HPP

#include "point.hpp"
#include "mesh.hpp"
#include <vector>
#include <optional>
#include <algorithm>
#include <thread>
#include <stdexcept>

class Triangulate {
	std::vector<Point> &points;
	int threads;

	template <typename ContainerIterator, int axis = 0>
	class Node {
		using Child = Node<ContainerIterator, 1-axis>;
		const ContainerIterator first, middle, last;
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
		static auto find_candidate(Mesh &mesh, const Mesh::Iterator &edge, const Point &opposite) {
			const auto &point = edge->second;
			const auto &candidate = edge.peek()->second;
			auto cross_product = (point - opposite) ^ (candidate - opposite);
			if (cross_product < 0 == rhs)
				return std::optional<Point const *>();
			if (!mesh.connected(point))
				return std::optional<Point const *>(&candidate);
			mesh.disconnect(point, candidate);
			const auto &next_candidate = edge.peek()->second;
			if (next_candidate.in_circle(rhs ? candidate : point, opposite, rhs ? point : candidate))
				return find_candidate<rhs>(mesh, edge, opposite);
			mesh.connect(point, candidate);
			return std::optional<Point const *>(&candidate);
		}

	public:
		Node(ContainerIterator first, ContainerIterator last, int threads) : first(first), last(last), middle(first + (last - first) / 2), threads(threads) {
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
						if (left_candidate.value()->in_circle(left_point, right_point, *right_candidate.value()))
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
		}
	};

public:
	Triangulate(std::vector<Point> &points, int threads) : points(points), threads(threads) { }

	auto operator()() {
		Mesh mesh;
		Node(points.begin(), points.end(), threads).triangulate(mesh);
		return mesh;
	}
};

#endif
