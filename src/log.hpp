////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef LOG_HPP
#define LOG_HPP

#include <streambuf>
#include <iostream>
#include <chrono>
#include <iomanip>

class Log {
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

public:
	struct Count { };
	struct Time { };

	Log(bool show) : null_stream(&null_buffer), output(show ? std::cerr : null_stream), start(now()) { }

	void operator()() {
		output << std::endl;
	}

	template <typename Arg, typename ...Args>
	void operator()(Arg const &arg, Args const &...args) {
		output << arg;
		(*this)(args...);
	}

	template <typename Value, typename Name, typename ...Args>
	void operator()(Count, Value value, Name const &name, Args const &...args) {
		auto static constexpr suffixes = {"","k","M","G"};
		auto suffix = suffixes.begin();
		double decimal = value;
		for (; decimal >= 999.95 && suffix + 1 < suffixes.end(); decimal *= 0.001, ++suffix) ;
		(*this)(" ", std::fixed, std::setprecision(value < 1000 ? 0 : 1), decimal, *suffix, " ", name, value == 1 ? "" : "s", args...);
	}

	template <typename ...Args>
	void operator()(Time, Args const &...args) {
		auto elapsed = std::chrono::duration<double>(now() - start);
		auto minutes = std::chrono::duration_cast<std::chrono::minutes>(elapsed);
		if (minutes.count() > 0)
			(*this)(minutes.count(), "m", std::fixed, std::setw(2), std::setfill('0'), std::setprecision(0), (elapsed - minutes).count(), "s: ", args...);
		else
			(*this)(std::fixed, std::setprecision(1), elapsed.count(), "s: ", args...);
	}
};

#endif
