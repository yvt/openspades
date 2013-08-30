#!/bin/sh

rm -f Base.pak Sounds.pak Models.pak

zip -r Base.pak Gfx Textures
zip -r Models.pak Models
zip -r Sounds.pak Sounds

