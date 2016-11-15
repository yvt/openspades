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

GCC 4.8 / Clang 3.2 or later is recommended because OpenSpades relies on C++11 features heavily.

1. Install dependencies:

   *On Debian-derived distributions*: 
    ```sudo apt-get install pkg-config libglew-dev libcurl3-openssl-dev libsdl2-dev libsdl2-image-dev libalut-dev``` (because of a bug in some distributions, you might also
   have to install more packages by ```sudo apt-get install libjpeg-dev libxinerama-dev libxft-dev```)


   *On other distributions*: 
   Install corresponding packages from your repository (or compile from source).
   
2. Download & install `SDL-2.0.2`
   ```bash
   wget http://www.libsdl.org/release/SDL2-2.0.2.tar.gz
   tar -zxvf SDL2-2.0.2.tar.gz
   cd SDL2-2.0.2/
   ./configure && make && sudo make install
   cd ../
   ```
   Additional dependencies may be required.

3. Download & install `SDL2_image-2.0.0` 
   ```bash
   wget https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.0.tar.gz
   tar -zxvf SDL2_image-2.0.0.tar.gz
   cd SDL2_image-2.0.0/
   ./configure --disable-png-shared && make && sudo make install
   cd ../
   ```
   Additional dependencies may be required.

4. Clone OpenSpades repository:

   ```git clone https://github.com/yvt/openspades.git && cd openspades```

5. Create directory `openspades.mk` in cloned/downloaded openspades repo and compile:

   ```bash
   mkdir openspades.mk
   cd openspades.mk
   cmake .. -DCMAKE_BUILD_TYPE=Release && make
   ```

6. Install openspades: 

   ```sudo make install```
   
   **note**: `make install` broken in old releases

7. Get shared files: 
   * If you compiling certain release (0.0.10 for example): 

      Download windows binary of OpenSpades from
      https://sites.google.com/a/yvt.jp/openspades/downloads, extract it, and copy the .pak files
      inside Resources directory into `/usr/local/share/games/openspades/Resources` or
      `$XDG_DATA_HOME/openspades/Resources`. If there were some files in these directories, you probably
      should remove them.

      **note**: use `~/.openspades/Resources` in old releases.

   * If you compiling straight from source (git clone):

      All needed resources would be downloaded while `make install`, so no need to worry.

      If you didn't launched `make install`, you can install resources manually. 
      See `Resources/PakLocation.txt` to find out where get latest shared files.
      Download, extract, and place them into `/usr/local/share/games/openspades/Resources` or
      `$XDG_DATA_HOME/openspades/Resources`

8. Launch:

   `openspades` or `cd $REPO_DIRECTORY; ./openspades.mk/bin/OpenSpades` and enjoy


### On Windows (with Visual Studio)

1. Get CMake, Visual Studio 2013 (Express) or Visual Studio 2015 (not very supported), and the OpenSpades source.
   Official: https://github.com/yvt/openspades
   Unofficial: https://github.com/learn-more/openspades
2. Extract or checkout the source (all examples will assume ```E:/Projects/openspades```, update paths in the examples to reflect yours)
3. Get (pre-compiled) copies of fltk, glew, curl, sdl2 and zlib, and place them in ```E:/Projects/openspades/Sources/Externals```.
   See the file ```E:/Projects/openspades/Sources/Externals/readme.txt``` for details (and a pre-compiled set of libraries, make sure to pick the right one for your version of VS).
4. Run CMake, using the paths:
   ```Source:   E:/Projects/openspades```
   ```Binaries: E:/Projects/openspades/OpenSpades.msvc```
   For your convenience, create the directory: ```E:/Projects/openspades/OpenSpades.msvc/os.Resources```, extract the resources (.pak files) to this dir,
   set ```OPENSPADES_RESDIR``` to point to this directory. (Run CMake again, now when running debug builds openspades will also read resources from this dir)
5. Open ```E:/Projects/openspades/OpenSpades.msvc/OpenSpades.sln``` in Visual Studio.

### On Mac OS X (with Xcode)

1. Get the latest version of Xcode and OpenSpades source.
2. Download and install [SDL2 development libraries for OS X](http://www.libsdl.org/download-2.0.php) to `/Library/Frameworks`.
3. Download and install [SDL2_image development libraries for OS X](https://www.libsdl.org/projects/SDL_image/) to `/Library/Frameworks`.
4. Download [OS X release of OpenSpades](https://github.com/yvt/openspades/releases), show the package contents, and copy `libysrspades.dylib` to the root of OpenSpades source tree.
5. Open `OpenSpades.xcodeproj` and build.

## Troubleshooting
For troubleshooting and common problems see [TROUBLESHOOTING](TROUBLESHOOTING.md).

Licensing
----------------------------------------------------------------------------------------------------
Please see the file named LICENSE.

Note that other assets including sounds and models are not open source.
