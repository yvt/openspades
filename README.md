openspades
==========

What is it?
-----------

OpenSpades is a compatible client of Ace of Spades 0.75. 

* Can connect to a vanilla/pyspades/pysnip server.
* Uses OpenGL/AL for better visuals.
* Open source, and cross platform.

Installation
------------

### On Linux

1. ```sudo apt-get install pkg-config libsdl1.2-dev libfltk1.3-dev libglew-dev``` (because of a bug in some distributions, 
you might also have to install more packages by ```sudo apt-get install libjpeg-dev libxinerama-dev libxft-dev```)
2. ```./autogen.sh && ./configure PKG_CONFIG=pkg-config && make```
3. ```sudo make install```
4. Download windows binary of OpenSpades from https://sites.google.com/a/yvt.jp/openspades/downloads, extract it,
and copy the .pak files inside Resources directory into /usr/local/share/openspades/Resources or ~/.openspades/Resources
5. ```openspades``` and enjoy

Licensing
---------
Please see the file called LICENSE.

Note that other assets including sounds and models are not open source.
