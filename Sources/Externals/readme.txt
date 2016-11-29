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
|   |   
|   +---curl/
|   |   \---(All curl headers)
|   |       
|   +---GL/
|   |   \---(All GL headers)
|   |       
|   \---SDL2/
|       \---(All SD2L headers)
|           
\---lib/
        (All libs here)

You may not need to hunt and compile the headers and libraries all by yourself:
 * There are pre-compiled zip files containing all required files for some versions of Visual Studio
 * They're already packed in the correct layout. You just need to merge.

Visual Studio 2010:
https://dl.dropboxusercontent.com/u/37804131/OpenSpades-Externals-Windows.zip

Visual Studio 2015:
https://www.dropbox.com/s/pfvk8tns1qgkxpr/OpenSpades-Externals-Windows-VS2015.rar?dl=1
