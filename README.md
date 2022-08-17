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
Once installed, you'll be able to launch OpenSpades from inside the desktop menu or from your terminal with the `openspades`

#### Flatpak package
On [flatpak enabled](https://flatpak.org/setup/) systems, OpenSpades can be installed with:

```bash
flatpak install flathub jp.yvt.OpenSpades
```

Once installed, you'll be able to launch OpenSpades from inside the desktop menu or from your terminal with `flatpak run jp.yvt.OpenSpades`

### On Unixes (from source)

#### Building
1. Install dependencies:

   *On Debian-derived distributions*:
   ```
   sudo apt-get install build-essential pkg-config libglew-dev libcurl3-openssl-dev libsdl2-dev \
     libsdl2-image-dev libalut-dev xdg-utils libfreetype6-dev libopus-dev \
     libopusfile-dev cmake imagemagick
   ```
   (because of a bug in some distributions, you might also
   have to install more packages by `sudo apt-get install libjpeg-dev libxinerama-dev libxft-dev`)
   
   *On Fedora or other RHEL-derived distributions*:
   ```
   sudo dnf install make automake gcc gcc-c++ kernel-devel pkgconf-pkg-config glew-devel \
     openssl-devel libcurl-devel SDL2-devel SDL2_image-devel \
     freealut-devel xdg-utils freetype-devel opus-devel opusfile-devel \
     libjpeg-devel libXinerama-devel libXft-devel cmake ImageMagick
   ```

   *On FreeBSD*:
   ```
   sudo pkg install gmake automake pkgconf glew openssl curl sdl2 sdl2-image \
     freealut xdg-utils freetype2 opus opusfile jpeg-turbo libXinerama libXft \
     cmake ImageMagick7
   ```

   *On other distributions*:
   Install corresponding packages from your repository (or compile from source).
   
   Building OpenSpades requires a C++ compiler, which is included in the dependencies above in case you don't have one installed yet.

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

#### Installing and launching

To launch the built game without installing:
```
cd $REPO_DIRECTORY/openspades.mk; bin/openspades
```

To install the game to your system (recommended), take the following steps:
   1. Execute the following command:
      ```
      sudo make install
      ```
      **note**: If you have a previous installation of OpenSpades, you have to uninstall it manually by `sudo rm -rf /usr/local/share/games/openspades` before installing a new one, or else it might load old resources.

   2. Launch the game by typing `openspades` into command line, or search for it from start menu.

Alternatively, to install the game to a different directory, take the following steps:

   1. Copy the Resources directory into bin (or else the game won't launch):

      ```
      cp -r ./Resources ./bin/
      ```
      **note**: If you plan on distributing it, remember to remove CMake files and folders from Resources.

   2. Move the "/openspades.mk" folder somewhere else, for example `/home/user/Games`, or `/opt/games` and rename it to "/OpenSpades".

   3. The game's launcher is located at `bin/openspades`. You can create a shortcut for it on the desktop or a `.desktop` file placed in `/usr/share/applications/` for it to appear in Start Menu. Make sure to set the `bin` directory as the shortcut's working directory, or else you will get an error about missing resources.

      **note**: If you choose a directory outside of your `/home/user`, for example `/opt/games`, remember to *chmod*  the game launcher's permissions to 755.

After successful installation, optionally you can remove the source code and build outputs to save disk space (~100MB).

#### On Linux (from source, by Nix Flakes)
To build and run OpenSpades from the latest source code:

```bash
nix shell github:yvt/openspades -c openspades
```

To build and run OpenSpades for development:

```bash
git clone https://github.com/yvt/openspades.git && cd openspades
nix develop
# note: this will patch CMake files in the source tree
cmakeBuildType=RelWithDebInfo cmakeConfigurePhase
buildPhase
bin/openspades
```

**note**: Nix Flakes are an experimental feature of Nix and must be enabled manually. See [this wiki article](https://nixos.wiki/wiki/Flakes) for how to do that.

### On Windows (with Visual Studio)
1. Get the required software if you haven't already:
  * CMake 2.8+
  * PowerShell 5.0
    * Integrated with Windows 10.
    * Older versions are not tested, but might work
  * *Visual Studio 2017* or later
    * VS2015 is also supported, but VS2017 is more recommended
    * VS2013 is no longer supported, but might work
2. Grab the source code:
  * Stable version: https://github.com/yvt/openspades/releases
  * Latest development version (0.1.x): https://github.com/yvt/openspades/archive/master.zip
3. Checkout the source
  * All examples will assume `E:/Projects/openspades`, update paths in the examples to reflect yours
  * Make sure to update all git submodules, e.g., by `git clone ... --recurse-submodules`). Note that the GitHub website's ZIP download currently does not support submodules.
4. Build libraries using vcpkg:
   ```bat
   cd E:/Projects/openspades
   vcpkg/bootstrap-vcpkg.bat
   vcpkg/vcpkg install @vcpkg_x86-windows.txt
   ```

5. Run CMake:
  * Source: `E:/Projects/openspades`
  * Binaries: `E:/Projects/openspades/OpenSpades.msvc`
  * Generator:
    * For VS2019: `Visual Studio 16 (2019)`
    * For VS2017: `Visual Studio 15 (2017)`
    * For VS2015: `Visual Studio 14 (2015)`
  * Platform: `Win32`
  * Toolchain file: `E:/Projects/openspades/vcpkg/scripts/buildsystems/vcpkg.cmake`
  * Add a new string entry `VCPKG_TARGET_TRIPLET=x86-windows-static`

6. Open `E:/Projects/openspades/OpenSpades.msvc/OpenSpades.sln` in Visual Studio.
7. Build the solution.
 * The recommended build configuration is `MinSizeRel` or `Release` if you're not an developer
 * The default build output directory is `E:/projects/OpenSpades/OpenSpades.msvc/bin/BUILD_TYPE/`
8. To get audio working, download a [Windows release of OpenSpades](https://github.com/yvt/openspades/releases), extract it, and copy the following dlls to the build output directory:
 * For OpenAL audio: `openal32.dll`
 * For YSR audio: `YSRSpades.dll`, `libgcc_s_dw2-1.dll`, `libstdc++-6.dll`, `pthreadGC2.dll`
9. Download the [Non-free pak](https://github.com/yvt/openspades-paks/releases/download/r33/OpenSpadesDevPackage-r33.zip), extract it, and copy `Nonfree/pak000-Nonfree.pak` to the `Resources` folder inside your build output directory, which is probably `E:/Projects/openspades/openspades.msvc/bin/BUILD_TYPE/Resources`. You can also copy the paks contained in `Official Mods/` folder of OpenSpades 0.0.12b to add more fonts and improve localization support of your build.

### On macOS (with Ninja)

#### Requirements
- Xcode Command Line Tools
- CMake
- pkg-config
- gcc 6 or newer (not clang!) — macOS 10.14 (Mojave) or earlier only
- ninja

#### Building
1. Install the Xcode Command Line Tools and other required build tools.

    ```bash
    xcode-select --install
    ```
    Using Homebrew:
    ```
    brew install cmake pkg-config ninja
    
    # If you are using macOS 10.14 (Mojave) or earlier:
    brew install gcc
    ```
    Using Nix:
    - Add `$(nix-build '<nixpkgs>' -A pkg-config-unwrapped --no-out-link)/bin` to `PATH`.
	
	
2. Clone the Openspades repository:

	```bash
	git clone https://github.com/yvt/openspades.git --recurse-submodules && cd openspades
	```
	
3. Bootstrap `vcpkg` and install the required packages:

	```bash
	vcpkg/bootstrap-vcpkg.sh
	vcpkg/vcpkg install @vcpkg_x86_64-darwin.txt
	```

4. Create directory `openspades.mk` in the cloned/downloaded openspades repo and compile:

   ```bash
   mkdir openspades.mk
   cd openspades.mk
   cmake -G Ninja .. -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_OSX_ARCHITECTURES=x86_64 -D CMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -D VCPKG_TARGET_TRIPLET=x64-osx
   ninja
   ```

5. Launch:

    ```bash
    open bin/OpenSpades.app
    ```

    (Universal builds aren't supported yet.)

### Network usage during building

OpenSpades' build process automatically downloads prebuilt game assets and libraries as needed. Specifically:

- `pak000-Nonfree.pak` and `font-uniform.pak` from <https://github.com/yvt/openspades-paks>. Can be disabled by passing `-D OPENSPADES_NONFREE_RESOURCES=NO` to CMake.
- The prebuilt binaries of YSRSpades (audio engine) from <https://github.com/yvt/openspades-media>. Can be disabled by passing `-D OPENSPADES_YSR=NO` to CMake.

In addition, vcpkg (sort of package manager only used for Windows and macOS builds) [collects and sends telemetry data to Microsoft](https://vcpkg.readthedocs.io/en/latest/about/privacy/). You can opt out of this behavior by passing `-disableMetrics` option when running `vcpkg/bootstrap-vcpkg.sh` command.

## Troubleshooting
For troubleshooting and common problems see [TROUBLESHOOTING](TROUBLESHOOTING.md).

## Licensing
Please see the file named LICENSE.
