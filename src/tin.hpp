#ifndef TIN_HPP
#define TIN_HPP

#include "faces.hpp"
#include "point.hpp"
#include "thinned.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>

class TIN {
	template <typename Iterator, int axis = 0>
	class Node {
		using Child = Node<Iterator, 1-axis>;
		std::vector<Child> children;
		Iterator first, last;
		Faces faces;

		static auto compare(const Point &p1, const Point &p2) {
			return p1[axis] < p2[axis] ? true : p1[axis] > p2[axis] ? false : p1[1-axis] < p2[1-axis];
		}

	public:
		Node(Iterator first, Iterator last) : first(first), last(last) {
			const auto middle = first + (last - first) / 2;
			std::nth_element(first, middle, last, compare);
			if (last - first > 3) {
				children.push_back(Child(first, middle));
				children.push_back(Child(middle, last));
			}
		}

		template <typename Insert, typename Erase>
		void each_face(Insert insert, Erase erase) {
			for (auto &node: children)
				node.each_face(insert, erase);
			if (last - first == 3) {
				Face face(first, last);
				faces.insert(face);
				insert(face);
			} else if (last - first > 3) {
				// TODO: merge children; inserting and erasing faces as appropriate
				children.clear();
				children.shrink_to_fit();
			}
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
	auto &concat(Tile tile) {
		for (const auto &point: tile)
			points.insert(point, better_than);
		return *this;
	}

	template <typename Insert, typename Erase>
	void each_face(Insert insert, Erase erase) {
		auto thinned = points.to_vector();
		if (thinned.size() < 3)
			throw std::runtime_error("not enough points");
		Node(thinned.begin(), thinned.end()).each_face(insert, erase);
	}
};

#endif
