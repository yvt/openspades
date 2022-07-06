#!/usr/bin/env bash

# This script downloads proprietary part of official .paks.
#
# note: the proprietary part contains following files:
#
#       1. sound files based on commercial sound libraries. Individual redistri-
#       bution of such files are disallowed by the publishers of sound libraries
#       (thus must be bundled with a software).
#
#       2. kv6 files from the original AoS 0.75.

OS_BASE="`uname`"
SRC_DIR="`dirname "$0"`"
# no color, not -m flag for OpenBSD
if [[ "$OS_BASE" != 'OpenBSD' ]] && [[ "$OS_BASE" != 'SunOS' ]]; then
	PAK_URL=$(grep --max-count=1 --no-filename --context=0 --color=never \
		"OpenSpadesDevPackage" "$SRC_DIR/PakLocation.txt")
else
	PAK_URL=$(grep -h \
		"OpenSpadesDevPackage" "$SRC_DIR/PakLocation.txt" | head -n1)
fi
echo "BASEURL ************ $PAK_URL"
PAK_NAME=$(basename "$PAK_URL")
OUTPUT_DIR="."

if [ -f "$PAK_NAME" ]; then
	exit 0
fi
# TODO: Check if the extracted files are present and up-to-date (#988)

if [ -n "$OPENSPADES_DEVPAK_PATH" ]; then
	cp "$OPENSPADES_DEVPAK_PATH" "$PAK_NAME"
else
	wget "$PAK_URL" -O "$PAK_NAME"
fi
unzip -o "$PAK_NAME" -d "$OUTPUT_DIR"

# relocate paks to the proper location
mv "$OUTPUT_DIR/Nonfree/pak000-Nonfree.pak" "$OUTPUT_DIR/"
mv "$OUTPUT_DIR/OfficialMods/font-unifont.pak" "$OUTPUT_DIR/"
mv "$OUTPUT_DIR/Nonfree/LICENSE.md" "$OUTPUT_DIR/LICENSE.pak000.md"
mv "$OUTPUT_DIR/OfficialMods/LICENSE" "$OUTPUT_DIR/LICENSE.unifont.txt"
