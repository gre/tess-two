#!/bin/sh

LEPTONICA_SRC_DIR="../jni/com_googlecode_leptonica_android/src/src"
cp environ.h $LEPTONICA_SRC_DIR
cp Makefile $LEPTONICA_SRC_DIR
cwd=$(pwd)
cd $LEPTONICA_SRC_DIR
make SHARED=yes shared
cp "../lib/shared/liblept.so" $cwd/libs
cd $cwd
