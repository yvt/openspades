#!/bin/sh

# This script updates GPL part of official .paks to match the latest git version.
# You need to download DevPaksXX.zip before using this script.

EXCLUDELIST=/tmp/mkpak.exclude.txt
find . | grep _Assets_ > $EXCLUDELIST
file . | grep ".DS_Store" >> $EXCLUDELIST
ZIPARGS=-x@${EXCLUDELIST}

zip -r pak000-Base.pak Gfx Scripts/Main.as Scripts/Base Shaders $ZIPARGS

zip -r pak010-BaseSkin.pak Scripts/Skin

zip -r pak011-Gui.pak Scripts/Gui

zip -r pak012-Locales.pak Locales

zip -r pak999-References.pak Scripts/Reference
