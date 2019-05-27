#!/usr/bin/env bash

# This script updates GPL part of official .paks to match the latest git version.

rm -f pak002-Base.pak pak005-Models.pak pak010-BaseSkin.pak pak050-Locales.pak pak999-References.pak

OUTPUT_DIR="`pwd`"
SRC_DIR="`dirname "$0"`"
LOG_FILE="/dev/null"

if [ ! "$MKPAK_LOG_FILE" = "" ]; then
	LOG_FILE="$MKPAK_LOG_FILE"
fi

pushd "$SRC_DIR"

EXCLUDELIST=/tmp/mkpak.exclude.txt
find . | grep _Assets_ > $EXCLUDELIST
file . | grep ".DS_Store" >> $EXCLUDELIST
ZIPARGS=-x@${EXCLUDELIST}

zip -r "$OUTPUT_DIR/pak002-Base.pak" \
License/Credits-pak002-Base.md Gfx Scripts/Main.as \
Scripts/Gui Scripts/Base Shaders \
Sounds/Feedback Sounds/Misc Sounds/Player Textures $ZIPARGS > "$LOG_FILE"

zip -r "$OUTPUT_DIR/pak005-Models.pak" Maps Models/MapObjects Models/Player $ZIPARGS > "$LOG_FILE"

zip -r "$OUTPUT_DIR/pak010-BaseSkin.pak" \
License/Credits-pak010-BaseSkin.md \
Scripts/Skin Sounds/Weapons Models/Weapons Models/MapObjects $ZIPARGS > "$LOG_FILE"

zip -r "$OUTPUT_DIR/pak050-Locales.pak" License/Credits-pak050-Locales.md Locales $ZIPARGS > "$LOG_FILE"

zip -r "$OUTPUT_DIR/pak999-References.pak" License/Credits-pak999-References.md Scripts/Reference $ZIPARGS > "$LOG_FILE"

popd
