#!/bin/sh

rm -rf hicolor/
mkdir tmp/

convert OpenSpades.ico \
	-set filename:res '%wx%h' \
	'tmp/%[filename:res].png'

convert tmp/256x256.png -resize 128x128 tmp/128x128.png
rm tmp/40x40.png # nobody uses icons with such res

for fn in tmp/*.png; do 
	RES=$( basename $fn | sed 's/.png//' )
	mkdir -p "hicolor/$RES/apps"
	mv $fn "hicolor/$RES/apps/openspades.png"
done

rm -rf tmp/
