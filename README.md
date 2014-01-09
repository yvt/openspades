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

1. *On Debian-derived distributions*: ```sudo apt-get install pkg-config libfltk1.3-dev
   libglew-dev libcurl3-openssl-dev``` (because of a bug in some distributions, you might also
   have to install more packages by ```sudo apt-get install libjpeg-dev libxinerama-dev libxft-dev```)
2. Download & install SDL2 ```tar xvf SDL2-2.0.1.tar.gz && cd SDL2-2.0.1/ && ./configure && make && sudo make install```
3. For the people that do not understand it, this is the part where you cd to your openspades directory...
   ```mkdir openspades.mk && cd openspades.mk && cmake .. && make OpenSpades```
4. ~~```sudo make install```~~
   -- note: the make install seems broken atm (it puts all files in /urs/local/bin)
   some linux guru / expert (or really, just anyone that used linux more than once) should either fix the
   cmake script, or explain to someone how the build should be installed (what files go where)
5. Download windows binary of OpenSpades from
   https://sites.google.com/a/yvt.jp/openspades/downloads, extract it, and copy the .pak files
   inside Resources directory into `/usr/local/share/openspades/Resources` or
   `~/.openspades/Resources`
6. ```openspades``` and enjoy


### On Windows (with visual studio)
1. Get CMake, Visual studio, and the OpenSpades source.
   Official: https://github.com/yvt/openspades
   Unofficial: https://github.com/learn-more/openspades
2. Extract or checkout the source (all examples will assume ```E:/Projects/openspades```, update paths in the examples to reflect yours)
3. Get (pre-compiled) copies of fltk, glew, curl, sdl2 and zlib, and place them in ```E:/Projects/openspades/Sources/Externals```.
   See the file ```E:/Projects/openspades/Sources/Externals/readme.txt``` for details (and a pre-compiled set of libraries).
4. Run CMake, using the paths:
   ```Source:   E:/Projects/openspades```
   ```Binaries: E:/Projects/openspades/OpenSpades.msvc```
   For your convenience, create the directory: ```E:/Projects/openspades/OpenSpades.msvc/os.Resources```, extract the resources (.pak files) to this dir,
   set ```OPENSPADES_RESDIR``` to point to this directory. (Run CMake again, now when running debug builds openspades will also read resources from this dir)
5. Open ```E:/Projects/openspades/OpenSpades.msvc/OpenSpades.sln``` in visual studio.

Licensing
----------------------------------------------------------------------------------------------------
Please see the file called LICENSE.

Note that other assets including sounds and models are not open source.
