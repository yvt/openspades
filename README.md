openspades
====================================================================================================

What is it?
----------------------------------------------------------------------------------------------------

OpenSpades is a compatible client of Ace of Spades 0.75.

* Can connect to a vanilla/pyspades/pysnip server.
* Uses OpenGL/AL for better visuals.
* Open source, and cross platform.

Installation
----------------------------------------------------------------------------------------------------

### On Linux

1. *On Debian-derived distributions*: ```sudo apt-get install pkg-config libsdl1.2-dev
   libfltk1.3-dev libglew-dev libcurl3-openssl-dev``` (because of a bug in some distributions, you
   might also have to install more packages by ```sudo apt-get install libjpeg-dev libxinerama-dev
   libxft-dev```)
2. ```mkdir openspades.mk && cd openspades.mk && cmake .. && make OpenSpades```
3. ```sudo make install```
4. Two choices:
    - Download windows binary of OpenSpades from
      https://sites.google.com/a/yvt.jp/openspades/downloads, extract it, and copy the .pak files
      inside Resources directory into `/usr/local/share/openspades/Resources` or
      `~/.openspades/Resources`
    - Download CMake, and run the cmake-gui (don't set the binary dir to be the same dir as your
      project dir, but preferably set it to something like `OpenSpades.msvc` (this directory is
      ignored by git).
5. ```openspades``` and enjoy


### On Windows (with visual studio)
1. Get CMake, Visual studio, and the OpenSpades source.
   Official: https://github.com/yvt/openspades
   Unofficial: https://github.com/learn-more/openspades
2. Extract or checkout the source (all examples will assume E:/Projects/openspades, update paths in the examples to reflect yours)
3. Get (pre-compiled) copies of fltk, glew, curl, sdl and zlib, and place them in E:/Projects/openspades/Sources/Externals.
   See the file E:/Projects/openspades/Sources/Externals/readme.txt for details (and a pre-compiled set of libraries).
4. Run CMake, using the paths:
   Source:   E:/Projects/openspades
   Binaries: E:/Projects/openspades/OpenSpades.msvc
   For your convenience, create the directory: E:/Projects/openspades/OpenSpades.msvc/os.Resources, extract the resources (.pak files) to this dir,
   and set OPENSPADES_RESDIR to point to this directory. (Run CMake again, now when running debug builds openspades will also read resources from this dir)
5. Open E:/Projects/openspades/OpenSpades.msvc/OpenSpades.sln in visual studio.

Licensing
----------------------------------------------------------------------------------------------------
Please see the file called LICENSE.

Note that other assets including sounds and models are not open source.
