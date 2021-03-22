#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <streambuf>
#include <ostream>
#include <iostream>
#include <chrono>
#include <cstddef>

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

public:
	Logger(bool show) : null_stream(&null_buffer), output(show ? std::cerr : null_stream), start(now()) { }

	void info() {
		output << std::endl;
	}

	template <typename Arg, typename ...Args>
	void info(const Arg &arg, const Args &...args) {
		output << arg;
		info(args...);
	}

	template <typename ...Args>
	void info(std::size_t arg, const Args &...args) {
		if (arg < 1000)
			return info((int)arg, args...);
		static constexpr auto suffixes = {'k','M','G'};
		auto suffix = suffixes.begin();
		for (; arg >= 999'950 & suffix + 1 < suffixes.end(); arg /= 1000, ++suffix) ;
		info(std::fixed, std::setprecision(1), 0.001 * arg, *suffix, args...);
	}

	template <typename ...Args>
	void time(const Args &...args) {
		info(std::fixed, std::setprecision(1), std::chrono::duration<double>(now() - start).count(), "s: ", args...);
	}
};

#endif
