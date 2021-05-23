////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SRS_HPP
#define SRS_HPP

#include "wkts.hpp"
#include <string>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <utility>

struct SRS {
	std::string wkt;
	std::optional<int> epsg;

	struct InvalidEPSG : std::runtime_error {
		InvalidEPSG(int epsg) : runtime_error("invalid EPSG code: " + std::to_string(epsg)) { }
	};

	SRS(int const &epsg) : epsg(epsg) {
		auto [pair, end] = std::equal_range(std::begin(wkts), std::end(wkts), std::pair(epsg, ""), [](auto const &pair1, auto const &pair2) {
			return pair1.first < pair2.first;
		});
		if (pair == end)
			throw InvalidEPSG(epsg);
		wkt = pair->second;
	}

	SRS(std::string::iterator const &begin, std::string::iterator const &end) : wkt(begin, end) { }

	friend auto operator<(SRS const &srs1, SRS const &srs2) {
		return srs1.wkt < srs2.wkt;
	}
};

using OptionalSRS = std::optional<SRS>;

#endif
