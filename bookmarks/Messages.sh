#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc -o -name \*.ui -o -name \*.kcfg` >> rc.cpp
$XGETTEXT `find -iname \*.cpp -o -iname \*.h` -o $podir/kio_bookmarks.pot
