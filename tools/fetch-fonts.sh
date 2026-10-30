#!/bin/sh
# Download the large and third-party fonts into fonts/ at pinned
# versions. They stay out of git: each update would add a full copy to
# the history (Inksans alone is 24 MB per weight).
#
# Usage: sh tools/fetch-fonts.sh
set -eu

cd "$(dirname "$0")/../fonts"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

sha256() {
    if command -v sha256sum >/dev/null; then
        sha256sum "$1" | cut -d' ' -f1
    else
        shasum -a 256 "$1" | cut -d' ' -f1
    fi
}

# get URL SHA256 FILE: download into $tmp/FILE and verify it
get() {
    curl -fsSL -o "$tmp/$3" "$1"
    if [ "$(sha256 "$tmp/$3")" != "$2" ]; then
        echo "fetch-fonts: checksum mismatch for $1" >&2
        exit 1
    fi
}

# a font is installed when its marker file exists; delete it to refetch.
# Each block copies its marker last, so an interrupted run fetches again.
want() {
    if [ -e "$1" ]; then
        echo "have $1"
        return 1
    fi
    echo "fetch $1"
}

U=https://unifoundry.com/pub/unifont/unifont-18.0.01/font-builds
if want unifont.otf; then
    get $U/unifont-18.0.01.otf \
        88d0a14d4aa9a96419720b39ba5da921a59560d76d4ec7d523acc06ed85cd3c2 u.otf
    mv "$tmp/u.otf" unifont.otf
fi
if want unifont-upper.otf; then
    get $U/unifont_upper-18.0.01.otf \
        472201e45a050bff4c7658208b8c73e34ebce3764b287a5a46c7774038ef9391 u.otf
    mv "$tmp/u.otf" unifont-upper.otf
fi

# google/fonts has no releases; pin the commit
N=https://raw.githubusercontent.com/google/fonts/b979dba422e445492b0eb9951ac52ee0b4d648c3/ofl/notoemoji
if want notoemoji/NotoEmoji.ttf; then
    get "$N/NotoEmoji%5Bwght%5D.ttf" \
        de6c18832938afc99caf132b39d6a30a19bac7f2e812e28db2535b4608d27551 e.ttf
    get $N/OFL.txt \
        500bb1ccf43df7bbb522112f9133a52b16e1c35e809632f5d8609b179152de5b OFL.txt
    mkdir -p notoemoji
    mv "$tmp/OFL.txt" notoemoji/
    mv "$tmp/e.ttf" notoemoji/NotoEmoji.ttf
fi

if want freefont/README; then
    get https://ftp.gnu.org/gnu/freefont/freefont-otf-20120503.tar.gz \
        3a6c51868c71b006c33c4bcde63d90927e6fcca8f51c965b8ad62d021614a860 f.tgz
    tar -xzf "$tmp/f.tgz" -C "$tmp"
    mkdir -p freefont
    cp "$tmp"/freefont-20120503/*.otf "$tmp"/freefont-20120503/COPYING freefont/
    cp "$tmp"/freefont-20120503/README freefont/
fi

# Courier 10 Pitch (Bitstream Type 1, donated to the X Consortium)
if want courier/COPYING; then
    get https://xorg.freedesktop.org/releases/individual/font/font-bitstream-type1-1.0.4.tar.xz \
        de2f238b4cd72db4228a0ba67829d76a2b7c039e22993d66a722ee385248c628 c.txz
    tar -xJf "$tmp/c.txz" -C "$tmp"
    mkdir -p courier
    for f in c0419bt_ c0582bt_ c0583bt_ c0611bt_; do
        cp "$tmp/font-bitstream-type1-1.0.4/$f.pfb" \
            "$tmp/font-bitstream-type1-1.0.4/$f.afm" courier/
    done
    cp "$tmp/font-bitstream-type1-1.0.4/COPYING" courier/
fi

R=https://github.com/salmonization/caffeine-type/releases/download/inksans-2.000
if want inksans/InksansCJK-W3.ttc; then
    get $R/inksans-2.000-W3.zip \
        d0dcb83b5394cb10f5d2144b6c571c9aaf10a98505d9947467577e2c8bc73957 w3.zip
    get $R/inksans-2.000-W6.zip \
        07307f566e55d13f024b65eabeb7f7b5ebf2a3ec189cc56aa313a296a85426a6 w6.zip
    unzip -q -o "$tmp/w3.zip" -d "$tmp"
    unzip -q -o "$tmp/w6.zip" -d "$tmp"
    mkdir -p inksans
    cp "$tmp"/inksans-2.000-W3/OFL.txt "$tmp"/inksans-2.000-W3/README.txt \
        "$tmp"/inksans-2.000-W6/InksansCJK-W6.ttc inksans/
    cp "$tmp"/inksans-2.000-W3/InksansCJK-W3.ttc inksans/
fi
