
When building on Windows or macOS, external dependencies should be placed in this folder.


## For Windows users:

C++ headers go in the 'include' folder, as shown in the file tree below.
Libraries go in the 'lib' folder, without any subfolders.

This should be the resulting directory structure:

Externals
|   readme.txt (this file)
|
+---include/
|   +---zconf.h
|   +---zlib.h
|   +---ft2build.h
|   |
|   +---curl/
|   |   \---(All curl headers)
|   |
|   +---GL/
|   |   \---(All GL headers)
|   |
|   +---freetype/
|   |   \---(All FreeType 2.7 headers)
|   |
|   +---opus/
|   |   \---(All libopus/libopusfile/libogg headers)
|   |
|   \---SDL2/
|       \---(All SDL2 headers)
|
\---lib/
        (All libs here)

You may not need to hunt and compile the headers and libraries all by yourself:
 * There are pre-compiled zip files containing all required files for some versions of Visual Studio
 * They're already packed in the correct layout. You just need to merge.

Visual Studio 2015:
https://openspadesmedia.yvt.jp/development-packages/OpenSpades-Externals-Windows-VS2015-3.zip

Visual Studio 2010 (FreeType, libopus and libopusfile is missing):
https://dl.dropboxusercontent.com/u/37804131/OpenSpades-Externals-Windows.zip


## For macOS users:

C++ headers go in the 'include' folder, as shown in the file tree below.
Libraries and frameworks go in the 'lib' folder, without any subfolders.

Externals
|   readme.txt (this file)
|
+---include/
|   +---ft2build.h
|   +---opusfile.h
|   |
|   +---ogg/
|   |   \---(All libogg headers)
|   |
|   \---freetype/
|       \---(All FreeType 2.7 headers)
|
\---lib/
        (All libs here)

The following URL provides a pre-compiled zip file containing all required files.

https://openspadesmedia.yvt.jp/development-packages/OpenSpades-Externals-Darwin-4.zip
