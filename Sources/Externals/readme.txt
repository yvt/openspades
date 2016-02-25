
When building (on windows) you can put dependencies in this folder.
Headers go in the include folder (grouped by product, f.e. 'SDL', 'FL', 'curl') except for zlib, this goes directly in the include.
Libs all go in the lib folder, without subfolders.

this should be the resulting directory structure:

Externals
|   fluid.exe
|   out.txt
|   readme.txt
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
|   \---SDL/
|       \---(All SDL headers)
|           
\---lib/
        (All libs here)

A set of precompiled libraries (packed with the correct layout, built with Visual Studio 2010):
https://dl.dropboxusercontent.com/u/37804131/OpenSpades-Externals-Windows.zip
Libraries for Visual Studio 2015:
https://www.dropbox.com/s/pfvk8tns1qgkxpr/OpenSpades-Externals-Windows-VS2015.rar?dl=1
