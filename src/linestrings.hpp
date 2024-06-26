////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef LINESTRINGS_HPP
#define LINESTRINGS_HPP

#include "vertex.hpp"
#include <list>
#include <vector>

using Linestring = std::list<Vertex>;
using Linestrings = std::vector<Linestring>;
using MultiLinestrings = std::vector<Linestrings>;

#endif
