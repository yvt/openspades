
When building (on windows) you can put dependencies in this folder.
Headers go in the include folder (grouped by product, f.e. 'SDL', 'FL', 'curl') except for zlib, this goes directly in the include.
Libs all go in the lib folder, without subfolders.

Example:

Externals/
	readme.txt
	include/
		zlib.h
		zconf.h
		curl/
			* all curl headers
		FL/
			* all FL headers
		SDL/
			* all SDL headers
	lib/
		zlib.lib
		libcurl.lib
		fltk.lib
		SDL.lib
