#!/bin/sh

cwd=$(pwd)
DESKTOP_LIBS_DIR="libs"

LEPTONICA_PATCH_DIR="patch/leptonica"
LEPTONICA_SRC_DIR="../jni/com_googlecode_leptonica_android/src/src"

cp "$LEPTONICA_SRC_DIR/environ.h" "environ.h"
cp "$LEPTONICA_PATCH_DIR/environ.h" $LEPTONICA_SRC_DIR
cp "$LEPTONICA_PATCH_DIR/Makefile" $LEPTONICA_SRC_DIR
cd $LEPTONICA_SRC_DIR
make SHARED=yes shared
cp "../lib/shared/liblept.so" "$cwd/$DESKTOP_LIBS_DIR"
make clean
cd $cwd
mv "environ.h" "$LEPTONICA_SRC_DIR/environ.h"
rm "$LEPTONICA_SRC_DIR/Makefile"
