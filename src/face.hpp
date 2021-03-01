#ifndef FACE_HPP
#define FACE_HPP

#include "point.hpp"
#include "edge.hpp"
#include <array>
#include <cstddef>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <algorithm>

class Face {
	std::array<Point, 3> points;

	template <typename Iterator>
	struct EdgeIterator {
		const Iterator start;
		Iterator here;

		EdgeIterator(Iterator start, Iterator here) : start(start), here(here) { }
		auto &operator++() { ++here; return *this;}
		auto operator++(int) { auto old = *this; ++here; return old;}
		// auto operator==(EdgeIterator other) const { return here == other.here; }
		auto operator!=(EdgeIterator other) const { return here != other.here; }
		auto operator*() { return Edge(*here, *(here - start == 2 ? start : here + 1)); }
	};

public:
	auto begin() const { return EdgeIterator(points.begin(), points.begin()); }
	auto   end() const { return EdgeIterator(points.begin(), points.end()); }

	// // TODO
	// template <typename Iterator>
	// Face(Iterator i) : points({*i++, *i++, *i++}) {
	// 	auto p = points.begin();
	// 	std::iter_swap(p, std::min_element(begin(), end()));
	// 	if (((*(p+1) - *p) ^ (*(p+2) - *p)) < 0)
	// 		std::iter_swap(p+1, p+2);
	// }

	friend auto operator==(const Face &face1, const Face &face2) {
		return face1.points == face2.points;
	}

	auto operator>(double length) const {
		for (const auto edge: *this)
			if (edge > length)
				return true;
		return false;
	}
};

template <> struct std::hash<Face> {
	std::size_t operator()(const Face &face) const { return hash<Edge>()(*face.begin()); }
};

#endif
