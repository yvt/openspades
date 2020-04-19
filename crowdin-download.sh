#!/bin/sh

# work-around for the issue that `languages_mapping` does not work for some reason
# this script performs a destructive operation - use carefully!

function remap() {
    if [ -f "Resources/Locales/$1/openspades.po" ]; then
        echo "Relocating Resources/Locales/$1 to Resources/Locales/$2"

        # work-around for case-insensitive file systems (e.g., HFS+, NTFS)
        mv "Resources/Locales/$1" "Resources/Locales/temp"
        rm -rf "Resources/Locales/$2"
        mv "Resources/Locales/temp" "Resources/Locales/$2"
    fi
}

crowdin download || exit 1

rm -rf "Resources/Locales/temp"

remap de_DE de
remap el_GR el
remap es_ES es
remap fr_FR fr
remap id_ID id
remap it_IT it
remap ja_JP ja
remap ko_KR ko
remap nl_NL nl
remap pl_PL pl
remap pt_PT pt_pt
remap pt_BR pt_br
remap ru_RU ru
remap uk_UA uk
remap vi_VN vi
remap hu_HU hu
remap lt_LT lt
remap nb_NO nb
remap nn_NO nn
remap tr_TR tr
remap jbo_EN jbo
