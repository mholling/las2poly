#ifndef LAS_HPP
#define LAS_HPP

#include "raw_point.hpp"
#include <fstream>
#include <cstddef>
#include <stdexcept>

class LAS {
	std::ifstream &input;

public:
	std::size_t count;

	LAS(std::ifstream &input) : input(input), count(0) {
		throw std::runtime_error("LAS reader not implemented");
	}

	auto point() {
		RawPoint point;
		return point;
	}
};

#endif
