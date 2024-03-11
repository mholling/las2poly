////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SHAPEFILE_HPP
#define SHAPEFILE_HPP

#include "polygons.hpp"
#include "linestrings.hpp"
#include "srs.hpp"
#include <limits>
#include <bit>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <numeric>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>

static_assert(std::numeric_limits<double>::is_iec559);
static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little);

template <typename Collection> struct ShapeType;
template <> struct ShapeType<Polygons>         { auto static constexpr value = 5; };
template <> struct ShapeType<MultiLinestrings> { auto static constexpr value = 3; };

class Shapefile {
	auto static constexpr int32_max = std::numeric_limits<std::int32_t>::max();

	template <typename Value, unsigned offset>
	void static little(char *data, Value value) {
		*reinterpret_cast<Value *>(data + offset) = value;
		if constexpr (std::endian::native == std::endian::big)
			std::reverse(data + offset, data + offset + sizeof(Value));
	}

	template <typename Value, unsigned offset>
	void static big(char *data, Value value) {
		*reinterpret_cast<Value *>(data + offset) = value;
		if constexpr (std::endian::native == std::endian::little)
			std::reverse(data + offset, data + offset + sizeof(Value));
	}

	struct SHPX {
		using Integer = std::int32_t;
		using Double = double;

		std::filesystem::path shp_path;
		std::filesystem::path shx_path;

		SHPX(std::filesystem::path const &shp_path) : shp_path(shp_path), shx_path(shp_path) {
			shx_path.replace_extension(".shx");
		}

		template <typename Collection>
		void operator()(Collection const &collection) {
			auto static constexpr file_header_size = 100ull;
			auto static constexpr record_header_size = 8ull;
			auto static constexpr content_prefix_size = 44ull;
			auto static constexpr file_magic = 9994;
			auto static constexpr file_version = 1000;
			auto static constexpr shape_type = ShapeType<Collection>::value;

			auto const bounds = Bounds(collection);
			auto const shx_file_length = file_header_size + collection.size() * record_header_size;
			auto const shp_file_length = std::accumulate(collection.begin(), collection.end(), file_header_size, [](auto const &sum, auto const &polygon) {
				return std::accumulate(polygon.begin(), polygon.end(), sum + record_header_size + content_prefix_size, [](auto const &sum, auto const &vertices) {
					return sum + sizeof(Integer) + (vertices.size() + 1ull) * sizeof(Double) * 2ull;
				});
			});

			if (shp_file_length > sizeof(std::uint16_t) * int32_max)
				throw std::runtime_error("too many points for shapefile format");

			char file_header[file_header_size] = {0};
			   big<Integer,  0>(file_header, file_magic);
			little<Integer, 28>(file_header, file_version);
			little<Integer, 32>(file_header, shape_type);
			little<Double,  36>(file_header, bounds.xmin);
			little<Double,  44>(file_header, bounds.ymin);
			little<Double,  52>(file_header, bounds.xmax);
			little<Double,  60>(file_header, bounds.ymax);
			little<Double,  68>(file_header, 0.0); // zmin
			little<Double,  76>(file_header, 0.0); // zmax
			little<Double,  84>(file_header, 0.0); // mmin
			little<Double,  92>(file_header, 0.0); // mmax

			auto shp_file = std::ofstream(shp_path, std::ios_base::binary);
			auto shx_file = std::ofstream(shx_path, std::ios_base::binary);
			shp_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
			shx_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

			big<Integer, 24>(file_header, shx_file_length / sizeof(std::uint16_t));
			shx_file.write(file_header, file_header_size);

			big<Integer, 24>(file_header, shp_file_length / sizeof(std::uint16_t));
			shp_file.write(file_header, file_header_size);

			for (auto id = 0ull, record_offset = file_header_size; auto const &geometry: collection) {
				auto const num_parts = geometry.size();
				auto const num_points = std::accumulate(geometry.begin(), geometry.end(), 0ull, [](auto const &sum, auto const &vertices) {
					return sum + vertices.size() + 1ull;
				});
				auto const content_length = content_prefix_size + num_parts * sizeof(Integer) + num_points * sizeof(Double) * 2;
				auto const bounds = Bounds(geometry);

				char record_header[record_header_size];
				big<Integer, 0>(record_header, record_offset / sizeof(std::uint16_t));
				big<Integer, 4>(record_header, content_length / sizeof(std::uint16_t));

				shx_file.write(record_header, record_header_size);
				record_offset += record_header_size + content_length;

				big<Integer, 0>(record_header, ++id);
				shp_file.write(record_header, record_header_size);

				char content_prefix[content_prefix_size];
				little<Integer,  0>(content_prefix, shape_type);
				little< Double,  4>(content_prefix, bounds.xmin);
				little< Double, 12>(content_prefix, bounds.ymin);
				little< Double, 20>(content_prefix, bounds.xmax);
				little< Double, 28>(content_prefix, bounds.ymax);
				little<Integer, 36>(content_prefix, num_parts);
				little<Integer, 40>(content_prefix, num_points);

				auto indices = std::vector<char>(num_parts * sizeof(Integer));
				auto coords = std::vector<char>(num_points * sizeof(Double) * 2);
				auto index = reinterpret_cast<Integer *>(indices.data());
				auto coord = reinterpret_cast<Double *>(coords.data());

				for (auto count = 0ul; auto const &vertices: geometry) {
					*index++ = count;
					count += vertices.size() + 1ul;
					std::for_each(vertices.rbegin(), vertices.rend(), [&](auto const &vertex) {
						auto const &[x, y] = vertex;
						*coord++ = x, *coord++ = y;
					});
					auto const &[x, y] = vertices.back();
					*coord++ = x, *coord++ = y;
				}

				if constexpr (std::endian::native == std::endian::big) {
					for (auto pos = indices.begin(); pos != indices.end(); pos += sizeof(Integer))
						std::reverse(pos, pos + sizeof(Integer));
					for (auto pos = coords.begin(); pos != coords.end(); pos += sizeof(Double))
						std::reverse(pos, pos + sizeof(Double));
				}

				shp_file.write(content_prefix, content_prefix_size);
				shp_file.write(indices.data(), indices.size());
				shp_file.write(coords.data(), coords.size());
			}
		}
	};

	struct DBF {
		std::filesystem::path dbf_path;

		DBF(std::filesystem::path const &shp_path) : dbf_path(shp_path) {
			dbf_path.replace_extension(".dbf");
		}

		template <typename Collection>
		void operator()(Collection const &collection) {
			auto static constexpr header_size = 65;
			auto static constexpr field_width = 1 + std::numeric_limits<std::uint32_t>::digits10;

			auto const now = std::chrono::system_clock::now();
			auto const time = std::chrono::system_clock::to_time_t(now);
			auto const &local = *std::localtime(&time);

			char header[header_size] = {0};
			header[0] = 0x03;                // version number
			header[1] = local.tm_year;       // year
			header[2] = local.tm_mon + 1;    // month
			header[3] = local.tm_mday;       // day
			header[8] = header_size;         // length of header
			header[10] = field_width + 1;    // length of record
			header[29] = 0x57;               // ANSI language driver
			header[32] = 'F';                //
			header[33] = 'I';                // field name
			header[34] = 'D';                //
			header[43] = 'N';                // field type
			header[48] = field_width;        // field width
			header[64] = 0x0d;               // terminator

			little<std::uint32_t, 4>(header, collection.size());

			auto file = std::ofstream(dbf_path, std::ios_base::binary);
			file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
			file.write(header, header_size);

			for (auto fid = 0ul; fid < collection.size(); ++fid)
				file << '\x20' << std::setfill(' ') << std::setw(field_width) << fid;
			file << '\x1a';
		}
	};

	struct PRJ {
		std::filesystem::path prj_path;

		PRJ(std::filesystem::path const &shp_path) : prj_path(shp_path) {
			prj_path.replace_extension(".prj");
		}

		void operator()(SRS const &srs) {
			auto file = std::ofstream(prj_path);
			file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
			file << srs.wkt;
		}
	};

	SHPX shpx;
	DBF dbf;
	PRJ prj;

public:
	auto static constexpr allow_self_intersection = true;

	Shapefile(std::filesystem::path const &shp_path) :
		shpx(shp_path),
		dbf(shp_path),
		prj(shp_path)
	{ }

	template <typename Collection>
	void operator()(Collection const &collection, OptionalSRS const &srs) {
		if (collection.size() >= int32_max)
			throw std::runtime_error("too many collection for shapefile format");
		shpx(collection);
		dbf(collection);
		if (srs)
			prj(*srs);
	}

	void operator()(Linestrings const &linestrings, OptionalSRS const &srs) {
		auto collection = MultiLinestrings();
		for (auto const &linestring: linestrings)
			collection.emplace_back().push_back(linestring);
		(*this)(collection, srs);
	}

	void operator()(MultiPolygon const &multipolygon, OptionalSRS const &srs) {
		auto collection = Polygons();
		if (!multipolygon.empty())
			for (auto &rings = collection.emplace_back(); auto const &polygon: multipolygon)
				rings.insert(rings.end(), polygon.begin(), polygon.end());
		(*this)(collection, srs);
	}

	operator bool() const {
		using std::filesystem::exists;
		return exists(shpx.shp_path) || exists(shpx.shx_path) || exists(dbf.dbf_path) || exists(prj.prj_path);
	}
};

#endif
