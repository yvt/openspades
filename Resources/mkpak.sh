#!/bin/sh

# This script updates GPL part of official .paks to match the latest git version.

EXCLUDELIST=/tmp/mkpak.exclude.txt
find . | grep _Assets_ > $EXCLUDELIST
file . | grep ".DS_Store" >> $EXCLUDELIST
ZIPARGS=-x@${EXCLUDELIST}

rm -f pak002-Base.pak pak005-Models.pak pak010-BaseSkin.pak pak050-Locales.pak pak999-References.pak

zip -r pak002-Base.pak \
License/Credits-pak002-Base.md Gfx Scripts/Main.as \
Scripts/Gui Scripts/Base Shaders \
Sounds/Feedback Sounds/Misc Sounds/Player Textures $ZIPARGS

zip -r pak005-Models.pak Maps $ZIPARGS

zip -r pak010-BaseSkin.pak \
License/Credits-pak010-BaseSkin.md \
Scripts/Skin Sounds/Weapons $ZIPARGS

zip -r pak050-Locales.pak License/Credits-pak050-Locales.md Locales $ZIPARGS

zip -r pak999-References.pak License/Credits-pak999-References.md Scripts/Reference $ZIPARGS
