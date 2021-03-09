#ifndef PLY_HPP
#define PLY_HPP

#include "raw_point.hpp"
#include <fstream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cstddef>

class PLY {
	std::ifstream &input;

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
		for (std::getline(input, string); string.rfind("comment", 0) == 0;)
			std::getline(input, string);
		return string;
	}

	void expect(const std::string words) {
		if (line() != words)
			throw std::runtime_error("unable to process PLY file");
	}

	template <typename Value>
	void expect(const std::string words, Value &value) {
		auto string = line();
		if (string.rfind(words, 0) != 0)
			throw std::runtime_error("unable to process PLY file");
		std::istringstream(string.erase(0, words.size())) >> value;
	}

public:
	std::size_t count;

	PLY(std::ifstream &input) : input(input) {
		expect(format_string());
		expect("element vertex", count);
		expect("property float64 x");
		expect("property float64 y");
		expect("property float64 z");
		expect("property uint8 classification");
		expect("end_header");
	}

	auto point() const {
		RawPoint point;
		input.read(reinterpret_cast<char *>(&point.x), sizeof(point.x));
		input.read(reinterpret_cast<char *>(&point.y), sizeof(point.y));
		input.read(reinterpret_cast<char *>(&point.z), sizeof(point.z));
		input.read(reinterpret_cast<char *>(&point.c), sizeof(point.c));
		return point;
	}
};

#endif
