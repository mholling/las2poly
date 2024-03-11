////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef GEOJSON_HPP
#define GEOJSON_HPP

#include "vertex.hpp"
#include "linestrings.hpp"
#include "polygons.hpp"
#include "srs.hpp"
#include <sstream>
#include <optional>
#include <filesystem>
#include <type_traits>
#include <utility>
#include <iostream>
#include <string>
#include <fstream>

class GeoJSON {
	std::stringstream stream;
	std::optional<std::filesystem::path> json_path;

	template <typename Value>
	auto &operator<<(Value const &value) {
		stream << value;
		return *this;
	}

	friend auto &operator<<(GeoJSON &json, Vertex const &vertex) {
		return json << '[' << vertex[0] << ',' << vertex[1] << ']';
	}

	template <typename Vertices> requires (std::is_base_of_v<Linestring, Vertices>)
	friend auto &operator<<(GeoJSON &json, Vertices const &vertices) {
		for (json << '['; auto const &vertex: vertices)
			json << vertex << ',';
		return json << vertices.front() << ']';
	}

	friend auto &operator<<(GeoJSON &json, Polygon const &polygon) {
		for (auto separator = '['; auto const &ring: polygon)
			json << std::exchange(separator, ',') << ring;
		return json << ']';
	}

	friend auto &operator<<(GeoJSON &json, Polygons const &polygons) {
		for (auto separator = '['; auto const &polygon: polygons)
			json << std::exchange(separator, ',') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"Polygon\",\"coordinates\":" << polygon << "}}";
		return json << (polygons.empty() ? "[]" : "]");
	}

	friend auto &operator<<(GeoJSON &json, MultiPolygon const &multipolygon) {
		for (auto separator = "[{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"MultiPolygon\",\"coordinates\":["; auto const &polygon: multipolygon)
			json << std::exchange(separator, ",") << polygon;
		return json << (multipolygon.empty() ? "[]" : "]}}]");
	}

	friend auto &operator<<(GeoJSON &json, Linestrings const &linestrings) {
		for (auto separator = '['; auto const &linestring: linestrings)
			json << std::exchange(separator, ',') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"LineString\",\"coordinates\":" << linestring << "}}";
		return json << (linestrings.empty() ? "[]" : "]");
	}

	friend auto &operator<<(GeoJSON &json, MultiLinestrings const &multilinestrings) {
		for (auto separator = '['; auto const &multilinestring: multilinestrings) {
			json << std::exchange(separator, ',') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"MultiLineString\",\"coordinates\":";
			for (auto separator = '['; auto const &linestring: multilinestring)
				json << std::exchange(separator, ',') << linestring;
			json << (multilinestring.empty() ? "[]}}" : "]}}");
		}
		return json << (multilinestrings.empty() ? "[]" : "]");
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
	auto static constexpr allow_self_intersection = false;

	GeoJSON(std::optional<std::filesystem::path> const &json_path) : json_path(json_path) {
		stream.precision(15);
	}

	template <typename Polygons>
	void operator()(Polygons const &polygons, OptionalSRS const &srs) {
		*this << "{\"type\":\"FeatureCollection\",";
		if (srs)
			*this << *srs << ",";
		*this << "\"features\":" << polygons << "}";

		if (json_path) {
			auto file = std::ofstream(*json_path);
			file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
			file << *this << std::endl;
		} else
			std::cout << *this << std::endl;
	}

	operator bool() const {
		return json_path && std::filesystem::exists(*json_path);
	}
};

#endif
