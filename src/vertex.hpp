////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef VERTEX_HPP
#define VERTEX_HPP

#include "vector.hpp"
#include <utility>

using Vertex = Vector<2>;

template <> struct std::hash<Vertex> {
	std::size_t operator()(Vertex const &vertex) const {
		auto static constexpr hash = std::hash<double>();
		auto const seed = hash(vertex[0]);
		return seed ^ (hash(vertex[1]) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
	}
};

#endif
