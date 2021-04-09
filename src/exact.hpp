#ifndef EXACT_HPP
#define EXACT_HPP

#include "ieee754.hpp"
#include <cstddef>
#include <array>
#include <type_traits>
#include <algorithm>
#include <utility>

template <std::size_t N = 1>
class Exact : std::array<double, N> {
	using Array = std::array<double, N>;

	template <std::size_t M>
	friend class Exact;

	static auto split(double a) {
		static constexpr auto bits = IEEE754::bits();
		static constexpr auto s = 1ul + (1ul << (bits + 1u) / 2u);
		const auto c = s * a;
		const auto aa = c - a;
		const auto ah = c - aa;
		const auto al = a - ah;
		return std::pair(al, ah);
	};

	template <typename Initialise>
	Exact(Initialise initialise) { initialise(*this); }

public:
	Exact(double d) : Array{{d}} { }

	auto operator>(const int &zero) const { return Array::back() > zero; }
	auto operator<(const int &zero) const { return Array::back() < zero; }

	template <std::size_t M>
	auto operator+(const Exact<M> &other) const {
		return Exact<M+N>([&](auto &result) {
			auto begin = result.begin(), end = std::copy(Array::begin(), Array::end(), begin);
			for (auto q: other) {
				auto here = begin;
				while (here < end) {
					auto &e = *here++;
					const auto x = e + q;
					const auto qv = x - e;
					const auto ev = x - qv;
					const auto qr = q - qv;
					const auto er = e - ev;
					e = er + qr, q = x;
				}
				*here = q;
				++begin, ++end;
			}
		});
	}

	template <std::size_t M, typename = std::enable_if_t<M == 1 && N == 1>>
	auto operator*(const Exact<M> &other) const {
		return Exact<2>([&](auto &result) {
			const auto &a = Array::front();
			const auto &b = other.front();
			const auto [al, ah] = split(a);
			const auto [bl, bh] = split(b);
			const auto h = a * b;
			const auto err1 = h - (ah * bh);
			const auto err2 = err1 - (al * bh);
			const auto err3 = err2 - (ah * bl);
			result[0] = (al * bl) - err3;
			result[1] = h;
		});
	}
};

#endif
