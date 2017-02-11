#!/bin/sh
# This file should be run from the repository root (e.g. ~/openspades)
# TODO: Optimize and error-checking

  FILES_H=`find . -iname *.h`
FILES_CPP=`find . -iname *.cpp`
  FILES_C=`find . -iname *.c`
 FILES_AS=`find . -iname *.as`

FILES="${FILES_H} ${FILES_CPP} ${FILES_C} ${FILES_AS}"
echo $FILES| tr " " "\n" > .translate.this # Convert spaces to newlines

OPTIONS_OUTPUT="-o Resources/Locales/pot/openspades.pot"
OPTIONS_CPP="--c++"
OPTIONS_KEYWORD="-k_Tr:2,1c -k_TrN:2,1c,3" # Have no idea how this works
OPTIONS_COMMENTS="-c!" # comments for translators
OPTIONS="-j ${OPTIONS_OUTPUT} ${OPTIONS_CPP} ${OPTIONS_KEYWORD} ${OPTIONS_COMMENTS}"

META_PKG="--package-name=OpenSpades"
META_COPYRIGHT="--copyright-holder=yvt"
META_BUGS="--msgid-bugs-address=i@yvt.jp"
METADATA="$META_PKG $META_COPYRIGHT $META_BUGS --omit-header"

xgettext $OPTIONS $METADATA -f .translate.this

echo "Gettext template file is now up-to-date."
echo
echo "Now you can run 'crowdin upload sources' to upload it to Crowdin!"
echo "(provided that you have an API key to do that)"
