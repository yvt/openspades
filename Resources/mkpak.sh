#!/bin/sh

EXCLUDELIST=/tmp/mkpak.exclude.txt
find . | grep _Assets_ > $EXCLUDELIST
file . | grep ".DS_Store" >> $EXCLUDELIST
ZIPARGS=-x@${EXCLUDELIST}

rm -f pak000-Base.pak pak001-Sounds.pak pak002-Models.pak pak010-BaseSkin.pak pak999-References.pak

zip -r pak000-Base.pak Gfx Scripts/Main.as Scripts/Base Shaders Textures $ZIPARGS 

zip -r pak001-Sounds.pak Sounds/Feedback Sounds/Misc Sounds/Player $ZIPARGS

zip -r pak002-Models.pak Models/MapObjects Maps $ZIPARGS

zip -r pak010-BaseSkin.pak Models/Weapons Models/Player Scripts/Skin \
Sounds/Weapons $ZIPARGS

zip -r pak011-Gui.pak Scripts/Gui $ZIPARGS

zip -r pak999-References.pak Scripts/Reference $ZIPARGS


zip DevPaks.zip pak000-Base.pak pak001-Sounds.pak pak002-Models.pak pak010-BaseSkin.pak pak999-References.pak

