#ifndef FACE_HPP
#define FACE_HPP

#include "point.hpp"
#include "vertices.hpp"
#include <array>
#include <cstddef>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <algorithm>

class Face : public Vertices<std::array<Point, 3>> {
public:
	struct Hash {
		std::size_t operator()(const Face &face) const { return Edge::Hash()(*face.edges().begin()); }
	};

	template <typename I>
	Face(I first, I last) {
		while (first < last)
			vertices[--last - first] = *last;
		std::rotate(vertices.begin(), std::min_element(vertices.begin(), vertices.end()), vertices.end());
		auto p = vertices.begin();
		if (((*(p+1) - *p) ^ (*(p+2) - *p)) < 0)
			std::iter_swap(p+1, p+2);
	}

	auto bounds() const {
		return vertices[0].bounds() + vertices[1].bounds() + vertices[2].bounds();
	}

	friend auto operator==(const Face &face1, const Face &face2) {
		return face1.vertices == face2.vertices;
	}

	friend auto operator||(const Face &face1, const Face &face2) {
		for (const auto edge1: face1.edges())
			for (const auto edge2: face2.edges())
				if (edge1 || edge2)
					return true;
		return false;
	}

	auto operator>(double length) const {
		for (const auto edge: edges())
			if (edge > length)
				return true;
		return false;
	}
};

#endif
