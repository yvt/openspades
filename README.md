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
   sudo apt-get install pkg-config libglew-dev libcurl3-openssl-dev libsdl2-dev libsdl2-image-dev libalut-dev xdg-utils libfreetype6-dev libopus-dev libopusfile-dev
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

6. Install OpenSpades: 

   `sudo make install`
   
   **note**: If you have a previous installation of OpenSpades, you have to uninstall it manually by `sudo rm -rf /usr/local/share/games/openspades` before installing a new one.

7. Get shared files: 
   * If you're compiling certain release (0.0.10 for example): 

      Download windows binary of OpenSpades from
      https://github.com/yvt/openspades/releases, extract it, and copy the .pak files
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

   `openspades` or `cd $REPO_DIRECTORY/openspades.mk; bin/OpenSpades` and enjoy


### On Windows (with Visual Studio)
1. Get the required software if you haven't already:
  * CMake 2.8+
  * PowerShell 5.0
    * Integrated with Windows 10.
    * Older versions are not tested, but might work
  * *Visual Studio 2015* 
    * VS2013 is no longer supported, but might work
    * VS2017 is not yet supported, but might work
2. Grab the source code:
  * From a release: https://github.com/yvt/openspades/releases
  * Latest development version (0.1.0): https://github.com/yvt/openspades/archive/master.zip
3. Extract or checkout the source
  * All examples will assume `E:/Projects/openspades`, update paths in the examples to reflect yours
4. Get (pre-compiled) copies of glew, curl, sdl2 and zlib, and place them in `E:/Projects/openspades/Sources/Externals`
  * See the file `E:/Projects/openspades/Sources/Externals/readme.txt` for details and links to pre-compiled sets of libraries for your version of Visual Studio
5. Run CMake:
  * Source: `E:/Projects/openspades`
  * Binaries: `E:/Projects/openspades/OpenSpades.msvc`
  * Generator: Visual Studio 14 (2015) (not Win64!)
  
6. Open `E:/Projects/openspades/OpenSpades.msvc/OpenSpades.sln` in Visual Studio.
7. Build the solution. 
 * The recommended build configuration is `MinSizeRel` or `Release` if you're not an developer
 * The default build output directory is `E:/projects/OpenSpades/OpenSpades.msvc/bin/BUILD_TYPE/`
8. Copy all `.dll` files from `Source/Externals/lib` to the build output directory.
9. To get audio working, download a [Windows release of OpenSpades](https://github.com/yvt/openspades/releases), extract it, and copy the following dlls to the build output directory:
 * For OpenAL audio: `openal32.dll` 
 * For YSR audio: `YSRSpades.dll`, `libgcc_s_dw2-1.dll`, `libstdc++-6.dll`, `pthreadGC2.dll`
10. Download the [Non-free pak](https://github.com/yvt/openspades-paks/releases/download/r32/OpenSpadesDevPackage-r32.zip) and copy it to the `Resources` folder inside your build output directory, which is probably `E:/Projects/openspades/openspades.msvc/bin/BUILD_TYPE/Resources`. You can also copy the paks contained in `Official Mods/` folder of OpenSpades 0.0.12b to add more fonts and improve localization support of your build.

### On Mac OS X (with Xcode)
1. Get the latest version of Xcode and OpenSpades source.
2. Get (pre-compiled) copies of libraries, and place them in `Sources/Externals`
  * See the file `Sources/Externals/readme.txt` for details
4. Download [OS X release of OpenSpades](https://github.com/yvt/openspades/releases), show the package contents, and copy `libysrspades.dylib` to `Sources/Externals/lib`.
5. Download and extract the [Non-free pak](https://github.com/yvt/openspades-paks/releases/download/r32/OpenSpadesDevPackage-r32.zip). After that, copy `pak000-Nonfree.pak` and `font-unifont.pak` to `Resources/`.
6. Open `OpenSpades.xcodeproj` and build.

## Troubleshooting
For troubleshooting and common problems see [TROUBLESHOOTING](TROUBLESHOOTING.md).

## Licensing
Please see the file named LICENSE.
