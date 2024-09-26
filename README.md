# ROTATING CUBE

This project is the display of a 3D geometry rotating in the console. The geometry is rendered using colored characters. For now, perspective projection is not implemented yet. It is an isometric projection.

## Building the project

This repository does not use any other library thant the C99 standard library and the C math standard library.
Every step needed to build the project is specified in the Makefile.

- `make` or `make build` to build
- `make run GEOMETRY={geometry_path}` to build and run using a specified geometry
- `make clean` to remove build-related files

The default GEOMETRY is a cube and other geometries are provided within the geometry folder.
