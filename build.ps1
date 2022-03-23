# For AppVeyor CI

$ErrorActionPreference = 'Stop';

Write-Host Build start...

if ($isWindows)
{
  # Windows 10
  
  Write-Host Building for Windows...
  
  # Based off of https://github.com/Conticop/OpenSpades-assets/blob/main/build-openspades.ps1
  
  $RepoRoot = "" + (Get-Location)
  $BinaryDir = Join-Path "$RepoRoot" build bin MinSizeRel

  vcpkg/bootstrap-vcpkg.bat -disableMetrics

  vcpkg/vcpkg install "@vcpkg_x86-windows.txt"

  cmake -A Win32 -D "CMAKE_BUILD_TYPE=MinSizeRel" -D "CMAKE_TOOLCHAIN_FILE=$RepoRoot/vcpkg/scripts/buildsystems/vcpkg.cmake" -D "VCPKG_TARGET_TRIPLET=x86-windows-static" "-S$RepoRoot" "-B$RepoRoot/build"

  cmake --build "$RepoRoot/build" --config MinSizeRel --parallel 8

  Push-Location -Path "$BinaryDir"
}
else
{
  # Linux or Mac
  # We are currently not building for Mac on AppVeyor
  
  # It's kind of a pain anyways,
  # and most users don't play on Mac,
  # Developers on Mac usually have experience building on such
  # Supposedly the binary grabbed from AV doesn't work anyways
  
  Write-Host Building for GNU/Linux...
  
  apt-get update
  apt-get install pkg-config libglew-dev libcurl3-openssl-dev libsdl2-dev libsdl2-image-dev libalut-dev xdg-utils libfreetype6-dev libopus-dev libopusfile-dev cmake imagemagick libjpeg-dev libxinerama-dev libxft-dev
  apt-get upgrade
  
  mkdir build
  cd build
  
  cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel
  make -j 8
}
