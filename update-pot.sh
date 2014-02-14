#!/bin/sh

cd Resources/Locales/pot

INFILES=" `find ../../../Sources -iname \*.cpp`"
INFILES+=" `find ../../../Sources -iname \*.c`"
INFILES+=" `find ../../../Sources -iname \*.h`"
INFILES+=" `find ../../Scripts -iname \*.as`"

xgettext -o openspades.pot -j -d openspades -k_Tr:2,1c -k_TrN:2,1c,3 $INFILES \
--package-name=OpenSpades --copyright-holder=yvt --msgid-bugs-address=i\ at\ yvt.jp \
--omit-header

