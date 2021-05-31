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
These voids are detected by triangulating the lidar point cloud: groups of large triangles are likely to indicate the presence of water.
Slope analysis of each group is performed to reject non-horizontal voids, which can occur in steep terrain.
Land polygons are formed from the outline of the remaining triangles.

Input to the program is a list of lidar tiles in LAS format.
The tiles should share a common projected SRS, typically a UTM projection.
Points are thinned before triangulation, so no pre-processing is required.

The lidar tiles should be classified to indicate ground points.
Ground heights of non-ground points are interpolated from neighbouring ground points before analysis of flatness.
Spurious points, including low- and high-noise points
(classes 7 and 18)
and specular water reflections
(class 9)
are discarded.

Output from the program is a polygon file in GeoJSON or shapefile format, representing all land areas present in the lidar tiles.
File format is chosen according to the filename extension: .json for GeoJSON and .shp for shapefile.

For GeoJSON output, polygons conform to OGC standard, with anticlockwise exteriors and non-self-intersecting rings which may touch at vertices.
For shapefile output, polygons conform to ESRI standard, with clockwise exteriors and non-touching rings which may self-intersect at vertices.

A single
**--width**
option is required to calibrate the void detection process.

# OPTIONS

**-w**, **--width** *metres*

> Specify the minimum width of waterbodies to be detected.
> The value should be comfortably larger than the linear point density of the lidar data.
> Choose a value according to the scale you're working at.

**-a**, **--area** *metres&#178;*

> Set a minimum area for waterbodies and islands to be retained.
> A sensible default value is used according to the minimum waterbody width.

**-d**, **--delta** *metres*

> Set an allowable average height difference across opposing edges of a watercourse or waterbody.
> (Some leeway is required to accommodate situations such as steep-sided river banks.)

**-s**, **--slope** *degrees*

> Set the maximum slope for waterbodies.
> This secondary filter ensures remaining non-horizontal voids are ignored.

**-r**, **--water**

> Extract polygons for waterbodies instead of land areas.
> Unexpected results may occur when using partly-covered tiles.
> Tile extents should be continguous when using this option.

**-i**, **--simplify**

> Simplify the output polygons using Visvalingam's algorithm.

**-m**, **--smooth**

> Apply line smoothing to simplified polygons, rounding off corner angles sharper than 15 degrees.

**-g**, **--angle** *degrees*

> Apply line smoothing using the specified smoothing angle.

**--discard** *class,...*

> Choose a list of lidar point classes to discard.
> The default value of 0,1,7,9,12,18 discards unclassified, overlap, water and noise points.

**-c**, **--convention** *ogc|esri*

> Force polygon orientation according to OGC or ESRI convention.
> By default, GeoJSON files use the OGC convention and shapefiles use the ESRI convention.

**-e**, **--epsg** *number*

> Specify an EPSG code to set in the output file.
> Use this option to override missing or incorrect georeferencing in the lidar tiles.

**-t**, **--threads** *number*

> Select the number of threads to use when processing.
> Defaults to the number of available hardware threads.

**-x**, **--tiles** *tiles.txt*

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

Extract water areas and save as a shapefile:

	$ las2land --water --width 10 *.las water.shp

Pipe GeoJSON output to another command for conversion:

	$ las2land --width 8 *.las - | ogr2ogr land.kml /vsistdin/

Apply line smoothing:

	$ las2land --width 8 --smooth *.las land.json

Add 'bridge deck' points (class 17) to water areas:

	$ las2land --width 8 --discard 0,1,7,9,12,17,18 *.las land.json

# AUTHORS

Matthew Hollingworth

macOS 11.1 - May 31, 2021
