#! /usr/bin/env bash
$EXTRACTRC *.rc *.ui *.kcfg > rc.cpp
$XGETTEXT `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $podir/kio_media.pot
