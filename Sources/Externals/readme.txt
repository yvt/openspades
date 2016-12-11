When building on Windows, external dependencies should be placed in this folder.

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
|   \---SDL2/
|       \---(All SD2L headers)
|           
\---lib/
        (All libs here)

You may not need to hunt and compile the headers and libraries all by yourself:
 * There are pre-compiled zip files containing all required files for some versions of Visual Studio
 * They're already packed in the correct layout. You just need to merge.

Visual Studio 2015:
https://dl.dropboxusercontent.com/u/37804131/openspades/OpenSpades-Externals-Windows-VS2015-2.zip

Visual Studio 2010 (FreeType is missing):
https://dl.dropboxusercontent.com/u/37804131/OpenSpades-Externals-Windows.zip
