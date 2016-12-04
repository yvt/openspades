# OpenSpades [![Build status](https://travis-ci.org/yvt/openspades.png?branch=master)](https://travis-ci.org/yvt/openspades)

![OpenSpades banner](https://dl.dropboxusercontent.com/u/37804131/github/OpenSpadesBanner.jpg)

[Official website](http://openspades.yvt.jp) — [Download](https://github.com/yvt/openspades/releases) — [Community](http://buildandshoot.com) — [Trello board](https://trello.com/b/3rfpvODj/openspades-roadmap)

## What is it?
OpenSpades is a compatible client of Ace of Spades 0.75.

* Can connect to a vanilla/pyspades/pysnip server.
* Uses OpenGL/AL for better experience.
* Open source, and cross platform.

## How to Build/Install?
**Before you start:** In case you're having issues to build OpenSpades, it may be because this README file is outdated, if so:

 1. See the [Building Guide](https://github.com/yvt/openspades/wiki/Building), which may be up to date
 2. Or [open an issue](https://github.com/yvt/openspades/issues) if the problem persists

### On Linux
GCC 4.9 / Clang 3.2 or later is recommended because OpenSpades relies on C++11 features heavily.

1. Install dependencies:

   *On Debian-derived distributions*: 
   ```
   sudo apt-get install pkg-config libglew-dev libcurl3-openssl-dev libsdl2-dev libsdl2-image-dev libalut-dev
   ```
   (because of a bug in some distributions, you might also
   have to install more packages by `sudo apt-get install libjpeg-dev libxinerama-dev libxft-dev`)

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

   ```bash
   git clone https://github.com/yvt/openspades.git && cd openspades
   ```

5. Create directory `openspades.mk` in cloned/downloaded openspades repo and compile:

   ```bash
   mkdir openspades.mk
   cd openspades.mk
   cmake .. -DCMAKE_BUILD_TYPE=Release && make
   ```

6. Install OpensSpades: 

   `sudo make install`
   
   **note**: If you have a previous installation of OpenSpades, you have to uninstall it manually by `sudo rm -rf /usr/local/share/games/openspades` before installing a new one.

7. Get shared files: 
   * If you're compiling certain release (0.0.10 for example): 

      Download windows binary of OpenSpades from
      https://sites.google.com/a/yvt.jp/openspades/downloads, extract it, and copy the .pak files
      inside Resources directory into `/usr/local/share/games/openspades/Resources` or
      `$XDG_DATA_HOME/openspades/Resources`. If there were some files in these directories, you probably
      should remove them.

      **note**: use `~/.openspades/Resources` in old releases.

   * If you're compiling straight from source (git clone):

      All needed resources would be downloaded while `make install`, so no need to worry.

      If you didn't launch `make install`, you can install resources manually. 
      See `Resources/PakLocation.txt` to find out where get latest shared files.
      Download, extract, and place them into `/usr/local/share/games/openspades/Resources` or
      `$XDG_DATA_HOME/openspades/Resources`

8. Launch:

   `openspades` or `cd $REPO_DIRECTORY; ./openspades.mk/bin/OpenSpades` and enjoy


### On Windows (with Visual Studio)
1. Get the required software.
  * CMake 2.8+
  * *Visual Studio 2015* 
    * VS2013 is no longer supported, but might work
2. Grab the source code:
  * From a release: https://github.com/yvt/openspades/releases
  * Latest development version (0.1.0): https://github.com/yvt/openspades/archive/master.zip
3. Extract or checkout the source
  * All examples will assume `E:/Projects/openspades`, update paths in the examples to reflect yours
4. Get (pre-compiled) copies of glew, curl, sdl2 and zlib, and place them in `E:/Projects/openspades/Sources/Externals`.
   See the file `E:/Projects/openspades/Sources/Externals/readme.txt` for details (and a pre-compiled set of libraries, make sure to pick the right one for your version of VS).
5. Run CMake, using the paths:
   
   Source: `E:/Projects/openspades`,
   Binaries: `E:/Projects/openspades/OpenSpades.msvc`
   Generator: Visual Studio 12 (2013) or 14 (2015) (not Win64!)
   
   For your convenience, create the directory: `E:/Projects/openspades/OpenSpades.msvc/os.Resources`, and extract the [Non-free pak](https://dl.dropboxusercontent.com/u/37804131/openspades/DevPaks29.zip) (`pak000-Nonfree.pak`) into it. Also, please note you can't distribute this pak separately from OpenSpades releases or binaries, as noted on `Resources/PakLocation.txt`
   
   set `OPENSPADES_RESDIR` to point to `os.Resources`. (Run CMake again, now when running debug builds openspades will also read resources from this directory)
   
   **Note:** `OPENSPADES_RESDIR` must be set using slashes instead of backslashes (`E:/Projects/openspades/os.Resources` instead of `E:\Projects\openspades\os.Resources`). Also, no slashes at end.
   
6. Open `E:/Projects/openspades/OpenSpades.msvc/OpenSpades.sln` in Visual Studio.
7. Build the solution.
8. Copy all `.dll` files from `Source/Externals/lib` to the build output directory.
9. Download [Windows release of OpenSpades](https://github.com/yvt/openspades/releases), extract it, and copy `openal32.dll` and `YSRSpades.dll` to the build output directory.

    **Note:** In case OpenSpades still fails to find any dll, copy all the remaing dlls which aren't there yet, it should solve the problem.
    
10. Copy `E:/Projects/openspades/Resources` folder to your build directory, which is probably `E:/Projects/openspades/openspades.msvc/bin/BUILD_TYPE`

    In case you haven't set OPENSPADES_RESDIR and extracted the [Non-free pak](https://dl.dropboxusercontent.com/u/37804131/openspades/DevPaks29.zip) into it, extract it and merge it inside the `Resources` folder you just copied. You can also copy the paks contained in `Official Mods/` folder of OpenSpades 0.0.12b to add more fonts and improve localization support of your build.

### On Mac OS X (with Xcode)
1. Get the latest version of Xcode and OpenSpades source.
2. Download and install [SDL2 development libraries for OS X](http://www.libsdl.org/download-2.0.php) to `/Library/Frameworks`.
3. Download and install [SDL2_image development libraries for OS X](https://www.libsdl.org/projects/SDL_image/) to `/Library/Frameworks`.
4. Download [OS X release of OpenSpades](https://github.com/yvt/openspades/releases), show the package contents, and copy `libysrspades.dylib` to the root of OpenSpades source tree.
5. Open `OpenSpades.xcodeproj` and build.

## Troubleshooting
For troubleshooting and common problems see [TROUBLESHOOTING](TROUBLESHOOTING.md).

## Licensing
Please see the file named LICENSE.
