openspades
====================================================================================================

[![Build Status](https://travis-ci.org/yvt/openspades.png?branch=master)](https://travis-ci.org/yvt/openspades)

What is it?
----------------------------------------------------------------------------------------------------

OpenSpades is a compatible client of Ace of Spades 0.75.

* Can connect to a vanilla/pyspades/pysnip server.
* Uses OpenGL/AL for better visuals.
* Open source, and cross platform.

Installation
----------------------------------------------------------------------------------------------------

### On Linux

1. Install dependencies:

   *On Debian-derived distributions*: 
    ```sudo apt-get install pkg-config libfltk1.3-dev
   libglew-dev libcurl3-openssl-dev``` (because of a bug in some distributions, you might also
   have to install more packages by ```sudo apt-get install libjpeg-dev libxinerama-dev libxft-dev```)


   *On other distributions*: 
   Install corresponding packages from your repository (or compile from source).
   
2. Download & install SDL-2.0.2 from trunk 
   ```
   hg clone http://hg.libsdl.org/SDL
   cd SDL/
   ./configure && make && sudo make install
   cd ../
   ```
   Additional dependencies may be required.

3. Download & install SDL2_image 
   ```
   wget https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.0.tar.gz
   tar xvf SDL2_image-2.0.0.tar.gz
   cd SDL2_image-2.0.0
   ./configure && make && sudo make install
   cd ../
   ```
   Additional dependencies may be required.

4. Clone OpenSpades repository:

   ```git clone https://github.com/yvt/openspades.git && cd openspades```

5. Create directory `openspades.mk` in cloned/downloaded openspades repo and compile:

   ```mkdir openspades.mk && cd openspades.mk && cmake .. && make OpenSpades```

6. Install openspades: 

   ```sudo make install```
   
   **note**: the make install seems broken atm (it puts all files in /urs/local/bin)
   some linux guru / expert (or really, just anyone that used linux more than once) should either fix the
   cmake script, or explain to someone how the build should be installed (what files go where)

7. Get shared files: 
   * If you compiling certain release (0.0.10 for example): 

      Download windows binary of OpenSpades from
      https://sites.google.com/a/yvt.jp/openspades/downloads, extract it, and copy the .pak files
      inside Resources directory into `/usr/local/share/openspades/Resources` or
      `~/.openspades/Resources`
   * If you compiling straight from source (git clone):

      See `Resources/PakLocation.txt` to find out where get latest shared files.
      Download, extract, and place them into `/usr/local/share/openspades/Resources` or
      `~/.openspades/Resources`

8. Launch:

   `openspades` or `cd $REPO_DIRECTORY; ./openspades.mk/bin/OpenSpades` and enjoy


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
Please see the file named LICENSE.

Note that other assets including sounds and models are not open source.
