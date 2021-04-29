with import <nixpkgs> {};

with pkgs.xorg;

runCommand "dummy" rec {
  nativeBuildInputs = [ cmake imagemagick unzip zip file gcc ];

  buildInputs = [
    freetype SDL2 SDL2_image libGL zlib curl glew opusfile openal libogg pkgconfig libopus libGLU
    libXext pkg-config
  ];

  NIX_CFLAGS_LINK = [ "-L${libXext}/lib" ];
  NIX_CFLAGS_COMPILE= ["-I${libogg.dev}/include" "-I${libopus.dev}/include" "-I${libGLU.dev}/include" ];

  LD_LIBRARY_PATH = [ "${openal}/lib" ];
} ""
