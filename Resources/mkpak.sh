#!/bin/sh

rm -f Base.pak Sounds.pak Models.pak DevPaks.pak

zip -r Base.pak Gfx Textures
zip -r Models.pak Models
zip -r Sounds.pak Sounds

zip DevPaks.zip Base.pak Models.pak Sounds.pak
