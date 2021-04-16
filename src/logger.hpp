////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distrubuted under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <streambuf>
#include <iostream>
#include <chrono>
#include <cstddef>
#include <iomanip>

class Logger {
	struct NullBuffer : std::streambuf {
		int overflow(int c) { return c; }
	};

	NullBuffer null_buffer;
	std::ostream null_stream;
	std::ostream &output;
	std::chrono::time_point<std::chrono::system_clock> start;

	auto static now() {
		return std::chrono::system_clock::now();
	};

	auto elapsed() const {
		return std::chrono::duration<double>(now() - start).count();
	}

public:
	Logger(bool show) : null_stream(&null_buffer), output(show ? std::cerr : null_stream), start(now()) { }

	void info() {
		output << std::endl;
	}

	template <typename Arg, typename ...Args>
	void info(Arg const &arg, Args const &...args) {
		output << arg;
		info(args...);
	}

	template <typename ...Args>
	void info(std::size_t arg, char const *word, Args const &...args) {
		auto static constexpr suffixes = {"","k","M","G"};
		auto suffix = suffixes.begin();
		double decimal = arg;
		for (; decimal >= 999.95 && suffix + 1 < suffixes.end(); decimal *= 0.001, ++suffix) ;
		info(" ", std::fixed, std::setprecision(arg < 1000 ? 0 : 1), decimal, *suffix, " ", word, arg > 1 ? "s" : "", args...);
	}

	template <typename ...Args>
	void time(Args const &...args) {
		info(std::fixed, std::setprecision(1), elapsed(), "s: ", args...);
	}
};

#endif
