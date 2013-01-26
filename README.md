Andrew Hurle

CS 4731 - Computer Graphics

Homework 1



PoliBook program for displaying and drawing GRS files.

The program is linked against whatever files are present on the machine.
The Zoo Lab machines contain the needed library files, so they're not
included here.

Running
=====

To compile and run on Linux:

1. `make && ./hw1`


To compile and run on Windows (Zoo Lab machines, tested on FLA21-08):

1. Open "Visual Studio Command Prompt (2010)" from the start menu
2. `cd C:\wherever\these\files\are`
3. `nmake /f Win-Makefile`
4. `hw1.exe`

Notes
=====

The GRS file reading logic is found in GRSReader.cpp.  This class fills
in some structs describing the each file - extents, how many lines,
points in those lines, etc.  All points in all files, plus the free
drawing points, are sent to the GPU at once.  Every time the drawing is
updated, all points are sent to the GPU again.  Each GRSInfo struct
contains an index pointing to the start of its data in the GPU buffer.

The points on the GPU are displayed by iterating through the GRS structs
and calling glDrawArrays on the appropriate subset of points according
to how long each line is supposed to be.

In order to handle aspect ratio preservation, each GRSInfo struct is
contained in a "canvas".  A canvas defines the maximum area the drawing
can take up.  reshape() manages these maximum bounds.  drawCanvas() sets
glViewport such that aspect ratio is preserved within the canvas bounds.
These canvases are also nice since changing their content is as easy as
pointing them to a different GRSInfo struct.

Pressing 'e' won't clear the screen if already in a drawing mode - to
clear it, switch to 't' or 'p' and back to 'e'.

The drawing area starts with a red outline so you can still see its
edges after it has been resized.

