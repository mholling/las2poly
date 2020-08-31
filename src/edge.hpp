#ifndef EDGE_HPP
#define EDGE_HPP

#include "point.hpp"
#include "vector.hpp"
#include <cstdint>
#include <cstddef>

struct Edge {
	Point p0, p1;

	struct Hash {
		static constexpr auto spare_bits = (sizeof(std::size_t) - sizeof(std::uint32_t)) * 8;
		std::size_t operator()(const Edge &edge) const { return Point::Hash()(edge.p0) ^ Point::Hash()(edge.p1) << spare_bits; }
	};

	Edge(const Point &p0, const Point &p1) : p0(p0), p1(p1) { }

	auto delta() const {
		return p1 - p0;
	}

	auto delta3d() const {
		return dynamic_cast<const Vector<3> &>(p1) - p0;
	}

	auto vegetation(unsigned char max) const {
		return p0.vegetation(max) && p1.vegetation(max);
	}

	friend auto operator==(const Edge &edge1, const Edge &edge2) {
		return edge1.p0 == edge2.p0 && edge1.p1 == edge2.p1;
	}

	friend auto operator||(const Edge &edge1, const Edge &edge2) {
		return edge1.p0 == edge2.p1 && edge1.p1 == edge2.p0;
	}

	friend auto operator<(const Edge &edge1, const Edge &edge2) {
		return edge1.delta().sqnorm() < edge2.delta().sqnorm();
	}

	auto operator&&(const Point &p) const {
		return p0 == p || p1 == p;
	}

	auto operator<<(const Point &p) const {
		return (p0 < p) && !(p1 < p) && ((p0 - p) ^ (p1 - p)) > 0;
	}

	auto operator>>(const Point &p) const {
		return (p1 < p) && !(p0 < p) && ((p1 - p) ^ (p0 - p)) > 0;
	}

	auto operator^(const Point &p) const {
		return (p0 - p) ^ (p1 - p);
	}

	auto operator>(double length) const {
		return delta().sqnorm() > length * length;
	}

	auto operator-() const {
		return Edge(p1, p0);
	}
};

#endif
