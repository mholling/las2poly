#ifndef PLY_HPP
#define PLY_HPP

#include "point.hpp"
#include "face.hpp"
#include <fstream>
#include <vector>
#include <cstddef>
#include <string>
#include <stdexcept>

class PLY {
	std::ifstream ply;
	std::vector<Point> points;
	std::size_t vertex_count, face_count;

	constexpr auto format_string() {
		union {
			long value;
			char bytes[sizeof(long)];
		} test = {.value = 1};
		return test.bytes[sizeof(long)-1] == 1 ? "format binary_big_endian 1.0" : "format binary_little_endian 1.0";
	};

public:
	PLY(const std::string &ply_path) {
		ply.exceptions(ply.exceptions() | std::ifstream::failbit);
		ply.open(ply_path, std::ios::binary);

		std::string line(4, ' ');
		ply.read(line.data(), 4);
		if (line != "ply\n")
			throw std::runtime_error("not a valid PLY file");

		std::getline(ply, line);
		if (line != format_string())
			throw std::runtime_error("not a native binary PLY file");

		for (;;) {
			std::string command, type;
			std::getline(ply, line);
			std::istringstream words(line);
			words >> command;
			if ("end_header" == command)
				break;
			if ("element" != command)
				continue;
			words >> type;
			words >> (type == "vertex" ? vertex_count : face_count);
		}

		points.reserve(vertex_count);
		for (std::size_t index = 0; index < vertex_count; ++index)
			points.push_back(Point(ply, index));
	}

	template <typename F>
	void each_face(F function) {
		for (std::size_t index = 0; index < face_count; ++index)
			function(Face(points, ply, index));
	}
};

#endif
