#!/bin/sh

android update project --path .
ndk-build -j8
