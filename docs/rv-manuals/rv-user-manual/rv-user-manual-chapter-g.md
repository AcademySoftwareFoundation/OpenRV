# G - Rising Sun Research CineSpace .csp File Format

This is RSR's spec of their file format:

The cineSpace LUT format contains three main sections.

### Header

This section contains the LUT identifier and the LUT type, 3D or 1D.

It is made up of the first two (2) valid lines in the file. See Notes below for a definition of a valid line.

A 3D LUT:

```
CSPLUTV100
3D
```

or a 1D (channel) LUT:

```
CSPLUTV100
1D
```

### Metadata

This section contains metadata not defined by the CSP format itself. RV will ignore everything here except a line containing a conditioningGamma. For example:

```
BEGIN METADATA
conditioningGamma=0.4545
END METADATA
```

The conditioningGamma value is applied (in GLSL) to both the input lattice points and the input data to the LUT (so does not change the mathematical “meaning” of the LUT). Especially for LUTs expecting linear input, this can compensate for the fact that, when the LUT is applied in hardware the even spacing of the lattice points can cause artifacts. In particular, we've found that a conditioningGamma value of 0.4545 allows a ACES-to-ACESlog LUT applied in RV (in HW) to match the results of applying the same LUT in Nuke (in SW).

### 1D preLUT data

This section is designed to allow for unevenly spaced data and also to accommodate input data that maybe outside the 0.0 <-> 1.0 range. Each primary channel, red, green and blue has each own 3 line entry. The first line is the number of preLUT data entries for that channel. The second line is the input and the third line is the mapped output that will then become the input for the LUT data section.

It is made up of the valid lines 3 to 11 in the LUT. See Notes below for a definition of a valid line.

Examples …

Map extended input (max. 4.0) into top 10% of LUT

```
11
0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 4.0
0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0
11
0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 4.0
0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0
11
0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 4.0
0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0
```

Access LUT data via a gamma lookup Red channel has gamma 2.0 Green channel has gamma 3.0 but also has fewer points Blue channel has gamma 2.0 but also has fewer points

```
11
0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0
0.0 0.01 0.04 0.09 0.16 0.25 0.36 0.49 0.64 0.81 1.0
6
0.0 0.2 0.4 0.6 0.8 1.0
0.0 0.008 0.064 0.216 0.512 1.0
6
0.0 0.2 0.4 0.6 0.8 1.0
0.0 0.04 0.16 0.36 0.64 1.0 
```

### LUT data

This section contains the LUT data. The input stimuli for the LUT data is evenly spaced and normalized between 0.0 and 1.0. All data entries are space delimited floats. For 3D LUTs the data is red fastest.

It is made up of the valid lines 12 and onwards in the LUT. See Notes below for a definition of a valid line.

Examples …

Linear LUT with cube sides R,G,B = 2,3,4 (ie. a 2x3x4 data set)

```
2 3 4
0.0 0.0 0.0
1.0 0.0 0.0
0.0 0.5 0.0
1.0 0.5 0.0
0.0 1.0 0.0
1.0 1.0 0.0
0.0 0.0 0.33
1.0 0.0 0.33
0.0 0.5 0.33
1.0 0.5 0.33
0.0 1.0 0.33
1.0 1.0 0.33
0.0 0.0 0.66
1.0 0.0 0.66
0.0 0.5 0.66
1.0 0.5 0.66
0.0 1.0 0.66
1.0 1.0 0.66
0.0 0.0 1.0
1.0 0.0 1.0
0.0 0.5 1.0
1.0 0.5 1.0
0.0 1.0 1.0
1.0 1.0 1.0
```

### Notes

All lines starting with white space are considered not valid and are ignored. Lines can be escaped to the next line with “\\” (but note that RV does not actually implement this part of the spec, so lines should not be broken in CSP files for RV). All values on a single line are space delimited.

The first line must contain the LUT type and version identifier “CSPLUTV100”

The second line must contain either “3D” or “1D”.

The third valid line (after any METADATA section) is the number of entries in the red 1D preLUT. It is an integer.

The fourth valid line contains the input entries for the red 1D preLUT. These are floats and the range is not limited. The number of entries must be equal to the value on the third valid line.

The fifth valid line contains the output entries for the red 1D preLUT. These are floats and the range is limited to 0.0 <-> 1.0. The number of entries must be equal to the value on the third valid line.

The sixth valid line is the number of entries in the green 1D preLUT. It is an integer.

The seventh valid line contains the input entries for the green 1D preLUT. These are floats and the range is not limited. The number of entries must be equal to the value on the sixth valid line.

The eighth valid line contains the output entries for the green 1D preLUT. These are floats and the range is limited to 0.0 <-> 1.0. The number of entries must be equal to the value on the sixth valid line.

The ninth valid line is the number of entries in the blue 1D preLUT. It is an integer.

The tenth valid line contains the input entries for the blue 1D preLUT. These are floats and the range is not limited. The number of entries must be equal to the value on the ninth valid line.

The eleventh valid line contains the output entries for the blue 1D preLUT. These are floats and the range is limited to 0.0 <-> 1.0. The number of entries must be equal to the value on the ninth valid line.

The twelfth valid line in a “3D” LUT contains the axis lengths of the 3D data cube in R G B order.

The twelfth valid line in a “1D” LUT contains the 1D LUT length

The thirteenth valid line and onwards contain the LUT data. For 3D LUTs the order is red fastest. The data are floats and are not range limited. The data is evenly spaced.

The LUT file should be named with the extension .csp
