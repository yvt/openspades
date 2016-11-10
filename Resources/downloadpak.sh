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

SRC_DIR="`dirname "$0"`"
PAK_URL=$(grep --max-count=1 --no-filename --context=0 --color=never \
	"DevPaks" "$SRC_DIR/PakLocation.txt")
PAK_NAME=$(basename "$PAK_URL")
OUTPUT_DIR="."

if [ -f "$PAK_NAME" ]; then
	exit 0
fi

wget "$PAK_URL" -O "$PAK_NAME"
unzip -u  -o "$PAK_NAME" -d "$OUTPUT_DIR"
