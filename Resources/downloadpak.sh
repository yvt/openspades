#!/bin/sh

SRC_DIR="$1"
PAK_URL=$(grep --max-count=1 --no-filename --context=0 --color=never \
	"DevPaks" "$SRC_DIR/PakLocation.txt")
PAK_NAME=$(basename "$PAK_URL")
OUTPUT_DIR="DevPak"

if [ -f "$PAK_NAME" ]; then
	exit 0
fi

wget "$PAK_URL" -O "$PAK_NAME"
unzip -u  -o "$PAK_NAME" -d "$OUTPUT_DIR"
