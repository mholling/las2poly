////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef FILL_HPP
#define FILL_HPP

#include "bounds.hpp"
#include <vector>
#include <queue>
#include <algorithm>
#include <numeric>
#include <stdexcept>

class Fill {
	using Empty = std::vector<bool>;
	using Queue = std::queue<Empty::iterator>;

	int static constexpr margin = 5;
	double resolution;
	int imin, jmin;
	long long columns;
	Empty empty;

public:
	Fill(Bounds const &bounds, double resolution) : resolution(resolution) {
		imin = bounds.ymin / resolution;
		jmin = bounds.xmin / resolution;
		int imax = bounds.ymax / resolution;
		int jmax = bounds.xmax / resolution;
		int height = imax - imin + 1;
		int width  = jmax - jmin + 1;
		long long rows = height + 2 * margin;
		columns = width + 2 * margin;
		empty.assign(rows * columns, true);
	}

	void operator()(Bounds const &bounds) {
		auto const i0 = static_cast<int>(bounds.ymin / resolution) - imin;
		auto const i1 = static_cast<int>(bounds.ymax / resolution) - imin;
		auto const j0 = static_cast<int>(bounds.xmin / resolution) - jmin;
		auto const j1 = static_cast<int>(bounds.xmax / resolution) - jmin;
		auto const row_begin = empty.begin() + (i0 + margin) * columns;
		auto const row_end   = empty.begin() + (i1 + margin) * columns;
		for (auto row = row_begin; row <= row_end; row += columns)
			std::fill(row + j0 + margin, row + j1 + margin + 1, false);
	}

	template <typename Points>
	void operator()(Points &points) {
		auto const unfilled = std::accumulate(empty.begin(), empty.end(), 0ull);
		auto const filled = empty.size() - unfilled;
		if (unfilled > 10 * filled && unfilled / 2 + filled > 500'000'000)
			throw std::runtime_error("tileset too sparse");

		auto queue = Queue();
		for (queue.push(empty.begin()); !queue.empty(); queue.pop()) {
			auto here = queue.front();
			const auto row_begin = here - (here - empty.begin()) % columns, row_end = row_begin + columns;
			while (row_begin < here && *(here - 1))
				--here;
			for (bool above = false, below = false; here < row_end && *here; *here++ = false) {
				int const    row = (here - empty.begin()) / columns;
				int const column = (here - empty.begin()) % columns;
				auto const i = row - margin + imin;
				auto const j = column - margin + jmin;
				if ((i + j) % 2) {
					auto const x = resolution * (j + 0.5);
					auto const y = resolution * (i + 0.5);
					points.emplace_back(x, y);
				}
				if (!above && empty.end() - here > columns && *(here + columns))
					above = !above, queue.push(here + columns);
				if (!below && here >= empty.begin() + columns && *(here - columns))
					below = !below, queue.push(here - columns);
				if (above && !*(here + columns))
					above = !above;
				if (below && !*(here - columns))
					below = !below;
			}
		}
	}
};

#endif
