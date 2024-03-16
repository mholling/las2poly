LAS2POLY(1) - General Commands Manual

# NAME

**las2poly** - extract waterbodies from lidar tiles

# SYNOPSIS

**las2poly**
\[options]
*tile.las*
\[*tile.las&nbsp;...*]
*water.shp*

# DESCRIPTION

**las2poly**
delineates waterbodies and land areas from a classified lidar point cloud.
Airborne lidar data exhibits voids in areas where surface water is present.
These voids are detected by triangulating the lidar point cloud: groups of large triangles are likely to indicate the presence of water.
Slope analysis of each group is performed to reject non-horizontal voids, which can occur in steep terrain.
Waterbody polygons are formed from the outline of the remaining triangles.

Input to the program is a list of lidar tiles in LAS format.
The tiles should share a common projected SRS, typically a UTM projection, and should preferably be contiguous.
Points are thinned before triangulation, so no pre-processing is required.

The lidar tiles should be classified to indicate ground points.
Ground heights of non-ground points are interpolated from neighbouring ground points before analysis of flatness.
Spurious points, including low- and high-noise points
(classes 7 and 18)
and specular water reflections
(class 9)
are discarded.

Output from the program is a polygon file in GeoJSON or shapefile format, representing all waterbodies found in the lidar tiles.
File format is chosen according to the filename extension: .json for GeoJSON and .shp for shapefile.

For GeoJSON output, polygons conform to OGC standard, with anticlockwise exteriors and non-self-intersecting rings which may touch at vertices.
For shapefile output, polygons conform to ESRI standard, with clockwise exteriors and non-touching rings which may self-intersect at vertices.

A single
**--width**
option calibrates the resolution of the void detection process.
If a
**--width**
value is not provided, the point density of the tiles is examined to estimate the smallest usable width.

# OPTIONS

**-w**, **--width** *metres*

> Specify the minimum width of waterbodies to be detected.
> The value should be comfortably larger than the linear point density of the lidar data.
> Choose a value according to the scale you're working at.

**--area** *metres&#178;*

> Set a minimum area for waterbodies and islands to be retained.
> A sensible default value is used according to the minimum waterbody width.

**--delta** *metres*

> Set an allowable average height difference across opposing edges of a watercourse or waterbody.
> (Some leeway is required to accommodate situations such as steep-sided river banks.)

**--slope** *degrees*

> Set the maximum slope for waterbodies.
> This secondary filter ensures remaining non-horizontal voids are ignored.

**--land**

> Extract polygons for land areas instead of waterbodies.

**--simplify**

> Simplify the output polygons using Visvalingam's algorithm.

**--raw**

> Don't smooth the output polygons.

**--discard** *class,...*

> Choose a list of lidar point classes to discard.
> The default value of 0,1,7,9,12,18 discards unclassified, overlap, water and noise points.

**--multi**

> Collect polygons into a single multipolygon.

**--lines**

> Extract polygon boundaries as linestrings.

**--epsg** *number*

> Specify an EPSG code to set in the output file.
> Use this option to override missing or incorrect georeferencing in the lidar tiles.

**--threads** *number*

> Select the number of threads to use when processing.
> Defaults to the number of available hardware threads.

**--tiles** *tiles.txt*

> Provide a text file containing a list of lidar tiles to be processed, in place of command-line arguments.

**-o**, **--overwrite**

> Allow the output file to be overwritten if it already exists.

**-q**, **--quiet**

> Don't show progress information.

**-v**, **--version**

> Show the program version.

**-h**, **--help**

> Show a brief help summary.

# EXAMPLES

Process all lidar tiles in current directory:

	las2poly *.las water.shp

Extract land areas as GeoJSON:

	las2poly --land *.las land.json

Process tiles at a given width threshold:

	las2poly --width 8 *.las water.shp

Process tiles from an input file list:

	las2poly --tiles tiles.txt water.shp

Pipe GeoJSON output to another command:

	las2poly *.las - | ogr2ogr water.kml /vsistdin/

Apply line simplification:

	las2poly --simplify *.las water.shp

Add 'bridge deck' points (class 17) to water areas:

	las2poly --discard 0,1,7,9,12,17,18 *.las water.shp

Examine raw output for artifacts:

	las2poly --area 0 --raw *.las water.shp

# PERFORMANCE

Processing numerous or high-density tiles can result in significant memory consumption.
Observation of memory use is recommended.
Increase the
**--width**
parameter to mitigate this problem by thinning points more aggressively.

A low value for the
**--width**
parameter can also cause normal ground-point separation to be mistaken for water voids.
This will cause final boundary extraction phase to proceed slowly, with scanline artifacts appearing in the output.

# LIMITATIONS

Most voids in lidar data result from pulse absorption by waterbodies.
However, other low-albedo surfaces can also produce voids: most commonly, roads, asphalt surfaces and rooftops.
These may be incorrectly outlined as waterbodies if they are horizontal.

Conversely, a true waterbody void may fail to be detected if its outline is insufficiently flat.
This can occur in steep or densely vegetated terrain, where there is a paucity of ground points along the edge of the waterbody.
Relaxing either or both of the
**--delta**
and
**--slope**
parameters may help in such situations.

Along riverbanks and shorelines, lidar returns from overhanging vegetation can cause deviations in the water outline.
Discontinuities may also result in the outlines of narrow waterways.

Steep terrain features such as cliffs can occlude lidar pulses, producing shadows in the point cloud.
However, such voids are unlikely to appear horizontal and will likely be eliminated during processing.

Finally, poor quality lidar data can contain void artifacts in areas of inconsistent point density.
Increasing the
**--width**
threshold can eliminate such problems, at the cost of reduced fidelity.
Using
**--area** *0*
and
**--raw**
options can help to expose any problems and artifacts in the lidar data.

# AUTHORS

Matthew Hollingworth

macOS 13.1 - March 17, 2024
