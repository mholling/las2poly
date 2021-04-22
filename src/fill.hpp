////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef FILL_HPP
#define FILL_HPP

#include <vector>
#include <queue>
#include <algorithm>

template <int margin>
class Fill {
	using Empty = std::vector<bool>;
	using Queue = std::queue<Empty::iterator>;

	int columns;
	Empty empty;
	Queue queue;

public:
	Fill(int width, int height) : columns(width + 2 * margin), empty((width + 2 * margin) * (height + 2 * margin), true) { }

	void operator()(int imin, int jmin, int imax, int jmax) {
		for (auto row = empty.begin() + (imin + margin) * columns, row_end = empty.begin() + (imax + margin) * columns; row <= row_end; row += columns)
			std::fill(row + jmin + margin, row + jmax + margin + 1, false);
	}

	template <typename Function>
	void operator()(Function function) {
		for (queue.push(empty.begin()); !queue.empty(); queue.pop()) {
			auto here = queue.front();
			const auto row_begin = here - (here - empty.begin()) % columns, row_end = row_begin + columns;
			while (row_begin < here && *(here - 1))
				--here;
			for (bool above = false, below = false; here < row_end && *here; *here++ = false) {
				int const    row = (here - empty.begin()) / columns;
				int const column = (here - empty.begin()) % columns;
				function(row - margin, column - margin);
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
