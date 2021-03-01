#ifndef FACE_HPP
#define FACE_HPP

#include "edge.hpp"
#include <array>
#include <algorithm>
#include <cstddef>

using Face = std::array<Edge, 3>;

auto operator>(const Face &face, double length) {
	return std::any_of(face.begin(), face.end(), [=](const auto &edge) {
		return edge > length;
	});
};

template <> struct std::hash<Face> {
	std::size_t operator()(const Face &face) const { return hash<Edge>()(face[0]); }
};

#endif
