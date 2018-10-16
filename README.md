# OpenSpades [![Build status](https://travis-ci.org/yvt/openspades.svg?branch=master)](https://travis-ci.org/yvt/openspades) [![All releases downloads](https://img.shields.io/github/downloads/yvt/openspades/total.svg)](https://github.com/yvt/openspades/releases) [![Latest release](https://img.shields.io/github/release/yvt/openspades.svg)](https://github.com/yvt/openspades/releases) [![Crowdin](https://d322cqt584bo4o.cloudfront.net/openspades/localized.svg)](https://crowdin.com/project/openspades)


![OpenSpades banner](https://openspadesmedia.yvt.jp/brand/OpenSpadesBanner.jpg)

[Official website](https://openspades.yvt.jp) — [Download](https://github.com/yvt/openspades/releases) — [Community](https://buildandshoot.com) — [Trello board](https://trello.com/b/3rfpvODj/openspades-roadmap)

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
#### Snap package
On [snap enabled](https://snapcraft.io/docs/core/install) systems, the latest pre-built stable release of OpenSpades can be installed with:

```bash
sudo snap install openspades
```

Once installed, you'll be able to launch OpenSpades from inside the desktop menu or from your terminal with the `openspades` command.

#### Building and installing from source
GCC 4.9 / Clang 3.2 or later is recommended because OpenSpades relies on C++11 features heavily.

1. Install dependencies:

   *On Debian-derived distributions*:
   ```
   sudo apt-get install pkg-config libglew-dev libcurl3-openssl-dev libsdl2-dev \
     libsdl2-image-dev libalut-dev xdg-utils libfreetype6-dev libopus-dev \
     libopusfile-dev cmake imagemagick
   ```
   (because of a bug in some distributions, you might also
   have to install more packages by `sudo apt-get install libjpeg-dev libxinerama-dev libxft-dev`)

   *On other distributions*:
   Install corresponding packages from your repository (or compile from source).

2. Clone OpenSpades repository:

   ```bash
   git clone https://github.com/yvt/openspades.git && cd openspades
   ```

3. Create directory `openspades.mk` in cloned/downloaded openspades repo and compile:

   ```bash
   mkdir openspades.mk
   cd openspades.mk
   cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo && make
   ```

4. Install OpenSpades (optional but recommended):

   `sudo make install`

   **note**: If you have a previous installation of OpenSpades, you have to uninstall it manually by `sudo rm -rf /usr/local/share/games/openspades` before installing a new one.

5. Launch:

   `openspades` (if installed) or `cd $REPO_DIRECTORY/openspades.mk; bin/OpenSpades` and enjoy


### On Windows (with Visual Studio)
1. Get the required software if you haven't already:
  * CMake 2.8+
  * PowerShell 5.0
    * Integrated with Windows 10.
    * Older versions are not tested, but might work
  * *Visual Studio 2017*
    * VS2015 is also supported, but VS2017 is more recommended
    * VS2013 is no longer supported, but might work
2. Grab the source code:
  * Stable version: https://github.com/yvt/openspades/releases
  * Latest development version (0.1.x): https://github.com/yvt/openspades/archive/master.zip
3. Extract or checkout the source
  * All examples will assume `E:/Projects/openspades`, update paths in the examples to reflect yours
4. Get (pre-compiled) copies of glew, curl, sdl2 and zlib, and place them in `E:/Projects/openspades/Sources/Externals`
  * See the file `E:/Projects/openspades/Sources/Externals/readme.txt` for details and links to pre-compiled sets of libraries for your version of Visual Studio
5. Run CMake:
  * Source: `E:/Projects/openspades`
  * Binaries: `E:/Projects/openspades/OpenSpades.msvc`
  * Generator:
    * For VS2017: `Visual Studio 15 (2017)` (not Win64!)
    * For VS2015: `Visual Studio 14 (2015)` (not Win64!)

6. Open `E:/Projects/openspades/OpenSpades.msvc/OpenSpades.sln` in Visual Studio.
7. Build the solution.
 * The recommended build configuration is `MinSizeRel` or `Release` if you're not an developer
 * The default build output directory is `E:/projects/OpenSpades/OpenSpades.msvc/bin/BUILD_TYPE/`
8. Copy all `.dll` files from `Source/Externals/lib` to the build output directory.
9. To get audio working, download a [Windows release of OpenSpades](https://github.com/yvt/openspades/releases), extract it, and copy the following dlls to the build output directory:
 * For OpenAL audio: `openal32.dll`
 * For YSR audio: `YSRSpades.dll`, `libgcc_s_dw2-1.dll`, `libstdc++-6.dll`, `pthreadGC2.dll`
10. Download the [Non-free pak](https://github.com/yvt/openspades-paks/releases/download/r33/OpenSpadesDevPackage-r33.zip), extract it, and copy `Nonfree/pak000-Nonfree.pak` to the `Resources` folder inside your build output directory, which is probably `E:/Projects/openspades/openspades.msvc/bin/BUILD_TYPE/Resources`. You can also copy the paks contained in `Official Mods/` folder of OpenSpades 0.0.12b to add more fonts and improve localization support of your build.
11. Copy `Resources/PackageInfo.json` to the `Resources` folder inside your build output directory.

### On macOS (with Xcode)
1. Get the latest version of Xcode and OpenSpades source.
2. Get (pre-compiled) copies of libraries, and place them in `Sources/Externals`
  * See the file `Sources/Externals/readme.txt` for details
4. Download [macOS release of OpenSpades](https://github.com/yvt/openspades/releases), show the package contents, and copy `libysrspades.dylib` to `Sources/Externals/lib`.
5. Download and extract the [Non-free pak](https://github.com/yvt/openspades-paks/releases/download/r33/OpenSpadesDevPackage-r33.zip). After that, copy `Nonfree/pak000-Nonfree.pak` and `font-unifont.pak` to `Resources/`.
6. Open `OpenSpades.xcodeproj` and build.

## Troubleshooting
For troubleshooting and common problems see [TROUBLESHOOTING](TROUBLESHOOTING.md).

## Licensing
Please see the file named LICENSE.
