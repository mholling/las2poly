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

	auto format_string() const {
		union {
			long value;
			char bytes[sizeof(long)];
		} test = {.value = 1};
		return test.bytes[sizeof(long)-1] == 1 ? "format binary_big_endian 1.0" : "format binary_little_endian 1.0";
	};

	auto line() {
		std::string string;
		string.reserve(1000);
		for (std::getline(ply, string); string.rfind("comment", 0) == 0;)
			std::getline(ply, string);
		return string;
	}

	void expect(const std::string words) {
		if (line() != words)
			throw std::runtime_error("unable to process PLY file");
	}

	template <typename T>
	void expect(const std::string words, T &t) {
		auto string = line();
		if (string.rfind(words, 0) != 0)
			throw std::runtime_error("unable to process PLY file");
		std::istringstream(string.erase(0, words.size())) >> t;
	}

public:
	PLY(const std::string &ply_path) {
		ply.exceptions(ply.exceptions() | std::ifstream::failbit);
		ply.open(ply_path, std::ios::binary);

		expect("ply");
		expect(format_string());
		expect("element vertex", vertex_count);
		expect("property float64 x");
		expect("property float64 y");
		expect("property float64 z");
		expect("property uint8 classification");
		expect("element face", face_count);
		expect("property list uint8 uint32 vertex_indices");
		expect("end_header");

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
