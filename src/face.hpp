#ifndef FACE_HPP
#define FACE_HPP

#include "edge.hpp"
#include <array>
#include <cstddef>

using Face = std::array<Edge, 3>;

auto operator>(const Face &face, double length) {
	for (const auto edge: face)
		if (edge > length)
			return true;
	return false;
};

template <> struct std::hash<Face> {
	std::size_t operator()(const Face &face) const { return hash<Edge>()(face[0]); }
};

#endif
