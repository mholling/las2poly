////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef LOG_HPP
#define LOG_HPP

#include <chrono>
#include <iostream>
#include <optional>
#include <utility>

struct Log {
	struct Count { };
	struct Time { };

private:
	class Loud {
		auto static now() {
			return std::chrono::system_clock::now();
		};

		std::chrono::time_point<std::chrono::system_clock> start;

	public:
		Loud() : start(now()) { }

		void operator()() const {
			std::cerr << std::endl;
		}

		template <typename Arg, typename ...Args>
		void operator()(Arg const &arg, Args const &...args) const {
			std::cerr << arg;
			(*this)(args...);
		}

		template <typename Value, typename Name, typename ...Args>
		void operator()(Count, Value value, Name const &name, Args const &...args) const {
			auto static constexpr suffixes = {"","k","M","G"};
			auto suffix = suffixes.begin();
			double decimal = value;
			for (; decimal >= 999.95 && suffix + 1 < suffixes.end(); decimal *= 0.001, ++suffix) ;
			(*this)(" ", std::fixed, std::setprecision(value < 1000 ? 0 : 1), decimal, *suffix, " ", name, value == 1 ? "" : "s", args...);
		}

		template <typename ...Args>
		void operator()(Time, Args const &...args) const {
			auto elapsed = std::chrono::duration<double>(now() - start);
			auto minutes = std::chrono::duration_cast<std::chrono::minutes>(elapsed);
			if (minutes.count() > 0)
				(*this)(minutes.count(), "m", std::fixed, std::setw(2), std::setfill('0'), std::setprecision(0), (elapsed - minutes).count(), "s: ", args...);
			else
				(*this)(std::fixed, std::setprecision(1), elapsed.count(), "s: ", args...);
		}
	};

	using Optional = std::optional<Loud>;

	Optional optional;

public:
	Log(bool loud) : optional(loud ? Optional(std::in_place_t()) : Optional()) { }

	template <typename ...Args>
	void operator()(Args const &...args) const {
		if (optional)
			(*optional)(args...);
	}
};

#endif
