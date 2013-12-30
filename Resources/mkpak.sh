#!/bin/sh

rm -f pak000-Base.pak pak001-Sounds.pak pak002-Models.pak pak010-BaseSkin.pak pak999-References.pak

zip -r pak000-Base.pak Gfx Scripts/Main.as Scripts/Base Shaders Textures

zip -r pak001-Sounds.pak Sounds/Feedback Sounds/Misc Sounds/Player

zip -r pak002-Models.pak Models/MapObjects Maps

zip -r pak010-BaseSkin.pak Models/Weapons Models/Player Scripts/Skin \
Sounds/Weapons

zip -r pak011-Gui.pak Scripts/Gui

zip -r pak999-References.pak Scripts/Reference


zip DevPaks.zip pak000-Base.pak pak001-Sounds.pak pak002-Models.pak pak010-BaseSkin.pak pak999-References.pak

