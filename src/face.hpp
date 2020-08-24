#ifndef FACE_HPP
#define FACE_HPP

#include "point.hpp"
#include "edge.hpp"
#include "vertices.hpp"
#include "rtree.hpp"
#include <array>
#include <stdexcept>
#include <cstddef>
#include <unordered_set>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <queue>

class Face : public Vertices<std::array<const Point *, 3>> {
	std::size_t index;

	struct InvalidTIN : std::runtime_error {
		InvalidTIN() : runtime_error("not a valid TIN") { }
	};

public:
	struct Hash {
		auto operator()(const Face &face) const { return face.index; }
	};

	template <typename C>
	Face(const C &points, std::ifstream &input, std::size_t index) : index(index) {
		unsigned char vertex_count;
		input.read(reinterpret_cast<char *>(&vertex_count), sizeof(vertex_count));
		if (vertex_count != 3)
			throw InvalidTIN();
		for (unsigned int point_index, index = 0; index < 3; ++index) {
			input.read(reinterpret_cast<char *>(&point_index), sizeof(point_index));
			if (point_index >= points.size())
				throw InvalidTIN();
			vertices[index] = &points.at(point_index);
		}
	}

	auto bounds() const {
		return vertices[0]->bounds() + vertices[1]->bounds() + vertices[2]->bounds();
	}

	friend auto operator==(const Face &face1, const Face &face2) {
		return face1.index == face2.index;
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

	template <typename C, typename F>
	static void each_group(C &faces, F function) {
		RTree<Face> rtree(faces);
		while (!faces.empty()) {
			std::unordered_set<Face, Hash> group;
			std::queue<Face> pending;
			for (pending.push(*faces.begin()); !pending.empty(); pending.pop()) {
				const auto &face = pending.front();
				faces.erase(face);
				group.insert(face);
				rtree.search(face, [&](const auto &other) {
					if (group.count(other))
						return;
					if (face || other)
						pending.push(other);
				});
			}
			function(group);
		}
	}
};

#endif
