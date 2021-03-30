#ifndef PLY_HPP
#define PLY_HPP

#include "endian.hpp"
#include "point.hpp"
#include <istream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cstddef>

class PLY {
	std::istream &input;

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
	std::size_t size;

	PLY(std::istream &input) : input(input) {
		if constexpr (Endian::big)
			expect("format binary_big_endian 1.0");
		else
			expect("format binary_little_endian 1.0");
		expect("element vertex", size);
		expect("property float64 x");
		expect("property float64 y");
		expect("property float64 z");
		expect("property uint8 classification");
		expect("end_header");
	}

	auto read() const {
		double x, y, z;
		unsigned char classification;
		input.read(reinterpret_cast<char *>(&x), sizeof(x));
		input.read(reinterpret_cast<char *>(&y), sizeof(y));
		input.read(reinterpret_cast<char *>(&z), sizeof(z));
		input.read(reinterpret_cast<char *>(&classification), sizeof(classification));
		return Point(x, y, z, classification, false, false, 12 == classification);
	}
};

#endif
