#!/bin/sh

rm -f Base.pak Sounds.pak Models.pak DevPaks.pak

zip -r pak000-Base.pak Gfx Textures
zip -r pak001-Models.pak Models
zip -r pak002-Sounds.pak Sounds

zip DevPaks.zip pak000-Base.pak pak001-Models.pak pak002-Sounds.pak
