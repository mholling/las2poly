LAS2LAND(1) - General Commands Manual

# NAME

**las2land** - extract land areas from lidar tiles

# SYNOPSIS

**las2land**
\[options]
*tile.las*
\[*tile.las&nbsp;...*]
*land.json*

# DESCRIPTION

**las2land**
delineates land and water areas from a classified lidar point cloud.
Airborne lidar data exhibits voids in areas where surface water is present.
These voids are detected by triangulating the lidar point cloud; groups of large triangles are likely to indicate the presence of water.
Slope analysis of each group is performed to reject non-horizontal voids, which can occur in steep terrain.
Land polygons are formed from the outline of the remaining triangles.

Input to the program is a list of lidar tiles in LAS format.
The tiles should share a common projected SRS, typically a UTM projection.

The lidar tiles should be classified to indicate ground points.
Non-ground points are used in the triangulation, but not for analysis of flatness.
Spurious points, including low- and high-noise points
(classes 7 and 18)
and specular water reflections
(class 9)
are discarded.

Output from the program is a polygon file in GeoJSON format, representing all land areas present in the lidar tiles.
(Subtract land polygons from the lidar footprint to obtain water polygons.)

A single
**--width**
or
**--length**
option is required to calibrate the void detection process.

# OPTIONS

**-w**, **--width** *metres*

> Specify the minimum width of waterbodies to be detected.
> The value should be comfortably larger than the linear point density of the lidar data.
> Choose a value according to the scale you're working at.

**-s**, **--slope** *degrees*

> Set the maximum slope for a void area to be considered a potential waterbody.
> Some leeway is required to accommodate situations such as steep-sided river banks.
> Too lenient a value risks capturing non-water voids, such as steep cliffs, which can occlude a lidar sensor.
> The default value of 10&#176; works well in practice.

**-a**, **--area** *metres&#178;*

> Set an area threshold for removal of small waterbodies and islands.
> A sensible default value is used according to the minimum waterbody width.

**-l**, **--length** *metres*

> Specify a triangle threshold length, if desired.
> Triangles larger than this threshold are added to potential water voids.
> By default, a value the same as the minimum waterbody width is used, however a somewhat smaller value can be used to capture more detail.

**-i**, **--simplify**

> Simplify the output polygons using Visvalingam's algorithm.
> Simplified polygons may have self-intersections and are not guaranteed to be topologically valid, although this is rare.

**-m**, **--smooth**

> Apply line smoothing to simplified polygons, rounding off any corners sharper than 15&#176;.
> This option is useful for visual applications such as maps.

**-d**, **--discard** *class,...*

> Choose a list of lidar point classes to discard.
> The default value of 0,1,7,9,12,18 discards unclassified, overlap, water and noise points.

**-e**, **--epsg** *number*

> Specify an EPSG code to set in the output file.
> This should be the same EPSG as the input data, since no reprojection occurs.

**-t**, **--threads** *number*

> Select the number of threads to use when processing.
> Defaults to the number of available hardware threads.

**--tiles** *tiles.txt*

> Provide a text file containing a list of lidar tiles to be processed, in place of command-line arguments.

**-o**, **--overwrite**

> Allow the output file to be overwritten if it already exists.

**-p**, **--progress**

> Show progress information while data is processed.

**-v**, **--version**

> Show the program version.

**-h**, **--help**

> Show a brief help summary.

# EXAMPLES

Process all lidar tiles in current directory:

	$ las2land --width 8 *.las land.json

Process tiles from an input file list:

	$ las2land --width 8 --tiles tiles.txt land.json

Tag output with an EPSG and pipe to another command for conversion:

	$ las2land -w 8 -e 28355 *.las - | ogr2ogr land.shp /vsistdin/

Apply line smoothing:

	$ las2land --width 8 --smooth *.las land.json

Use a smaller triangle length to keep finer detail:

	$ las2land --width 12 --length 6 *.las land.json

Add 'bridge deck' points (class 17) to water areas:

	$ las2land --width 8 --discard 0,1,7,9,12,17,18 *.las land.json

# AUTHORS

Matthew Hollingworth

macOS 11.1 - April 16, 2021