#!/bin/sh

SRC_DIR="$1"
OUTPUT_DIR="hicolor"

if [ -d "$OUTPUT_DIR" ]; then
	exit 0
fi

echo "Generating FHS icons"

mkdir tmp/

convert "$SRC_DIR/OpenSpades.ico" \
	-set filename:res '%wx%h' \
	'tmp/%[filename:res].png'

convert tmp/256x256.png -resize 128x128 tmp/128x128.png
rm tmp/40x40.png # nobody uses icons with such res

for fn in tmp/*.png; do
	RES=$( basename $fn | sed 's/.png//' )
	mkdir -p "$OUTPUT_DIR/$RES/apps"
	mv $fn "$OUTPUT_DIR/$RES/apps/openspades.png"
done

rm -rf tmp/

echo "Done generating FHS icons"
