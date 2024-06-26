////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef PLY_HPP
#define PLY_HPP

#include "srs.hpp"
#include "point.hpp"
#include <istream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cstddef>
#include <bit>

class PLY {
	std::istream &input;

	auto line() {
		auto string = std::string();
		for (std::getline(input, string); string.rfind("comment", 0) == 0;)
			std::getline(input, string);
		return string;
	}

	void expect(std::string const words) {
		if (line() != words)
			throw std::runtime_error("unable to process PLY file");
	}

	template <typename Value>
	void expect(std::string const words, Value &value) {
		if (auto string = line(); string.rfind(words, 0) != 0)
			throw std::runtime_error("unable to process PLY file");
		else
			std::istringstream(string.erase(0, words.size())) >> value;
	}

public:
	std::size_t size;
	OptionalSRS srs;

	PLY(std::istream &input) : input(input) {
		if constexpr (std::endian::native == std::endian::big)
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
