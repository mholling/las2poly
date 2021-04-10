#ifndef EXACT_HPP
#define EXACT_HPP

#include "ieee754.hpp"
#include <cstddef>
#include <array>
#include <utility>
#include <algorithm>

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

	Exact() = default;

	auto partition() const {
		auto here = this->begin();
		const auto middle = here + N/2, end = here + N;
		auto e1 = Exact<N/2>();
		auto e2 = Exact<N-N/2>();
		for (auto e = e1.begin(); here < middle; )
			*e++ = *here++;
		for (auto e = e2.begin(); here < end; )
			*e++ = *here++;
		return std::pair(e1, e2);
	}

public:
	Exact(double d) : Array{{d}} { }

	auto operator>(const int &zero) const { return this->back() > zero; }
	auto operator<(const int &zero) const { return this->back() < zero; }
	auto operator>=(const int &zero) const { return this->back() >= zero; }
	auto operator<=(const int &zero) const { return this->back() <= zero; }
	auto operator!=(const int &zero) const { return this->back() != zero; }

	template <std::size_t M>
	auto operator+(const Exact<M> &other) const {
		auto result = Exact<M+N>();
		auto begin = result.begin(), end = std::copy(this->begin(), this->end(), begin);
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
		return result;
	}

	template <std::size_t M>
	auto operator*(const Exact<M> &other) const {
		if constexpr (N == 1 && M == 1) {
			auto result = Exact<M+N>();
			const auto &a = this->front();
			const auto &b = other.front();
			const auto [al, ah] = split(a);
			const auto [bl, bh] = split(b);
			const auto h = a * b;
			const auto err1 = h - (ah * bh);
			const auto err2 = err1 - (al * bh);
			const auto err3 = err2 - (ah * bl);
			result[0] = (al * bl) - err3;
			result[1] = h;
			return result;
		} else if constexpr(N > 1 && M > 1) {
			auto [a1, a2] = this->partition();
			auto [b1, b2] = other.partition();
			return (a1 * b1 + a1 * b2) + (a2 * b1 + a2 * b2);
		}
	}
};

#endif
