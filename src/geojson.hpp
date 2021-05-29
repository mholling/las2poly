////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef GEOJSON_HPP
#define GEOJSON_HPP

#include "polygons.hpp"
#include "srs.hpp"
#include "ring.hpp"
#include "vector.hpp"
#include <sstream>
#include <filesystem>
#include <utility>
#include <iostream>
#include <string>
#include <fstream>

class GeoJSON {
	std::stringstream stream;
	std::filesystem::path json_path;

	template <typename Value>
	auto &operator<<(Value const &value) {
		stream << value;
		return *this;
	}

	friend auto &operator<<(GeoJSON &json, Vector<2> const &vector) {
		auto separator = '[';
		for (auto const &coord: vector)
			json << std::exchange(separator, ',') << coord;
		return json << ']';
	}

	friend auto &operator<<(GeoJSON &json, Ring const &ring) {
		json << '[';
		for (auto const &vertex: ring)
			json << vertex << ',';
		return json << ring.front() << ']';
	}

	friend auto &operator<<(GeoJSON &json, Polygon const &polygon) {
		auto separator = '[';
		for (auto const &ring: polygon)
			json << std::exchange(separator, ',') << ring;
		return json << ']';
	}

	friend auto &operator<<(GeoJSON &json, Polygons const &polygons) {
		auto separator = '[';
		for (auto const &polygon: polygons)
			json << std::exchange(separator, ',') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"Polygon\",\"coordinates\":" << polygon << "}}";
		return json << (separator == '[' ? "[]" : "]");
	}

	friend auto &operator<<(GeoJSON &json, SRS const &srs) {
		json << "\"crs\":{\"type\":\"name\",\"properties\":{\"name\":\"";
		if (srs.epsg)
			json << "urn:ogc:def:crs:EPSG::" << *srs.epsg;
		else {
			auto wkt = srs.wkt;
			for (auto pos = wkt.find('"'); pos != std::string::npos; pos = wkt.find('"', ++pos))
				wkt.insert(pos++, 1, '\\');
			json << wkt;
		}
		return json << "\"}}";
	}

	friend auto &operator<<(std::ostream &stream, GeoJSON &json) {
		return stream << json.stream.str();
	}

public:
	GeoJSON(std::filesystem::path const &json_path) : json_path(json_path) {
		stream.precision(15);
	}

	void operator()(Polygons const &polygons, OptionalSRS const &srs) {
		*this << "{\"type\":\"FeatureCollection\",";
		if (srs)
			*this << *srs << ",";
		*this << "\"features\":" << polygons << "}";

		if (json_path == "-")
			std::cout << *this << std::endl;
		else {
			auto file = std::ofstream(json_path);
			file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
			file << *this << std::endl;
		}
	}

	operator bool() const {
		return json_path != "-" && std::filesystem::exists(json_path);
	}
};

#endif
