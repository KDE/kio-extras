#!/bin/bash -eu
#
# SPDX-FileCopyrightText: 2025 Azhar Momin <azhar.momin@kdemail.net>
# SPDX-License-Identifier: LGPL-2.0-or-later

# GLib, Cairo, and RSVG are required for building libappimage.
# However, the part that interacts with those libraries is not used by the thumbnailer,
# so it is alright to install them from the package manager instead of building them from source.
# ---------------------------------------------------------------------------------------------------
# Flex and Bison are required for building Solid, but since the thumbnailers do not use that library,
# we do not need to instrument them.
# The same goes for libmount and libacl.
apt-get update && \
    apt-get install -y cmake make automake autopoint libtool \
    wget po4a pkg-config perl python3 gperf texinfo \
    flex bison libmount-dev libacl1-dev
    # libglib2.0-dev libcairo-dev librsvg2-dev

pip3 install meson ninja

# libX11 requires autoconf >= 2.70 and the base builder image has autoconf 2.69
wget https://ftp.gnu.org/gnu/autoconf/autoconf-2.70.tar.gz
tar -xzf autoconf-2.70.tar.gz
cd $SRC/autoconf-2.70
./configure
make install -j$(nproc)
rm -rf $SRC/autoconf-2.70*
cd $SRC

# Base
git clone --depth 1 https://github.com/madler/zlib.git
git clone --depth 1 https://gitlab.freedesktop.org/freetype/freetype.git
git clone --depth 1 https://github.com/libexpat/libexpat.git
wget https://sourceforge.net/projects/libpng/files/libpng16/1.6.50/libpng-1.6.50.tar.gz
git clone --depth 1 https://gitlab.freedesktop.org/fontconfig/fontconfig.git
git clone --depth 1 -b dev git://code.qt.io/qt/qtbase.git
git clone --depth 1 https://invent.kde.org/frameworks/extra-cmake-modules.git
git clone --depth 1 https://github.com/facebook/zstd.git
git clone --depth 1 https://github.com/tukaani-project/xz.git
wget https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz
git clone --depth 1 https://invent.kde.org/frameworks/karchive.git
git clone --depth 1 https://invent.kde.org/frameworks/kconfig.git
git clone --depth 1 https://invent.kde.org/frameworks/kcoreaddons.git
git clone --depth 1 https://invent.kde.org/frameworks/ki18n.git
git clone --depth 1 https://invent.kde.org/frameworks/kservice.git
git clone --depth 1 https://invent.kde.org/frameworks/solid.git
git clone --depth 1 https://invent.kde.org/frameworks/kcrash.git
git clone --depth 1 https://invent.kde.org/frameworks/kwindowsystem.git
git clone --depth 1 https://invent.kde.org/frameworks/kauth.git
git clone --depth 1 https://invent.kde.org/frameworks/kio.git

# KIO-Extras Thumbnailers
# For AppImageCreator
# git clone --depth 1 https://github.com/libarchive/libarchive.git
# git clone --depth 1 https://github.com/libfuse/libfuse.git
# git clone --depth 1 https://github.com/vasi/squashfuse.git
# git clone --depth 1 https://github.com/azubieta/xdg-utils-cxx.git
# wget https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
# git clone --depth 1 https://github.com/AppImageCommunity/libappimage.git
# For AudioCreator
wget https://github.com/nemtrif/utfcpp/archive/refs/tags/v4.0.6.tar.gz -O utfcpp-4.0.6.tar.gz
git clone --depth 1 https://github.com/taglib/taglib.git
# For ComicCreator
wget https://www.rarlab.com/rar/unrarsrc-7.1.7.tar.gz
# For CursorCreator
git clone --depth 1 https://gitlab.freedesktop.org/xorg/proto/xcbproto.git
git clone --depth 1 https://gitlab.freedesktop.org/xorg/util/macros.git util-macros
git clone --depth 1 https://gitlab.freedesktop.org/xorg/proto/xorgproto.git
git clone --depth 1 https://gitlab.freedesktop.org/xorg/lib/libxtrans.git
git clone --depth 1 https://gitlab.freedesktop.org/xorg/lib/libXau.git
git clone --depth 1 https://gitlab.freedesktop.org/xorg/lib/libxcb.git
git clone --depth 1 https://gitlab.freedesktop.org/xorg/lib/libX11.git
git clone --depth 1 https://gitlab.freedesktop.org/xorg/lib/libXrender.git
git clone --depth 1 https://gitlab.freedesktop.org/xorg/lib/libXfixes.git
git clone --depth 1 https://gitlab.freedesktop.org/xorg/lib/libXcursor.git
# For DjVuCreator
wget http://downloads.sourceforge.net/djvu/djvulibre-3.5.28.tar.gz
# For EXRCreator
git clone --depth 1 --branch=RB-3.3 https://github.com/AcademySoftwareFoundation/openexr.git
# For JpegCreator
git clone --depth 1 https://github.com/google/brotli.git
git clone --depth 1 https://github.com/fmtlib/fmt.git
git clone --depth 1 https://github.com/exiv2/exiv2.git
git clone --depth 1 https://invent.kde.org/graphics/libkexiv2.git
# For TextCreator
git clone --depth 1 https://invent.kde.org/frameworks/syntax-highlighting.git
# For SvgCreator
git clone --depth 1 -b dev git://code.qt.io/qt/qtsvg.git

# Files for corpus
CORPUS_DIR=$SRC/kio-extras/thumbnail/autotests/data/corpus

# wget -P $CORPUS_DIR https://download.kde.org/stable/kdenlive/24.12/linux/kdenlive-24.12.3-x86_64.AppImage || echo "Downloading appimage failed"
wget -O $CORPUS_DIR/breeze.cursor https://invent.kde.org/plasma/breeze/-/raw/master/cursors/Breeze/Breeze/cursors/x-cursor || echo "Downloading cursor failed"
wget -P $CORPUS_DIR https://www.sndjvu.org/DjVu3Spec.djvu || echo "Downloading djvu spec failed"
git clone --depth 1 https://github.com/AcademySoftwareFoundation/openexr-images.git $CORPUS_DIR/openexr-images || echo "Downloading openexr-images failed"
wget -P $CORPUS_DIR https://kde.org/favicon.ico || echo "Downloading favicon failed"

# Dict Files
DICT_FILES_DIR=$SRC/kio-extras/thumbnail/autotests/data/dict

wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/exif.dict || echo "Downloading exif dictionary failed"
wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/jpeg.dict || echo "Downloading jpeg dictionary failed"
for dict in exif jpeg; do
    if [ -f "$DICT_FILES_DIR/${dict}.dict" ]; then
        cat "$DICT_FILES_DIR/${dict}.dict" >> $DICT_FILES_DIR/jpegcreator_fuzzer.dict
    fi
done

wget -O $DICT_FILES_DIR/exrcreator_fuzzer.dict https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/openexr.dict || echo "Downloading openexr dictionary failed"

wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/bmp.dict || echo "Downloading bmp dictionary failed"
wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/gif.dict || echo "Downloading gif dictionary failed"
wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/png.dict || echo "Downloading png dictionary failed"
wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/tiff.dict || echo "Downloading tiff dictionary failed"
wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/webp.dict || echo "Downloading webp dictionary failed"
wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/heif.dict || echo "Downloading heif dictionary failed"
wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/dds.dict || echo "Downloading dds dictionary failed"
wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/pbm.dict || echo "Downloading pbm dictionary failed"
wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/jpeg2000.dict || echo "Downloading jpeg2000 dictionary failed"
wget -P $DICT_FILES_DIR https://raw.githubusercontent.com/AFLplusplus/AFLplusplus/stable/dictionaries/psd.dict || echo "Downloading psd dictionary failed"
for dict in bmp gif png tiff webp heif dds pbm jpeg2000 psd; do
    if [ -f "$DICT_FILES_DIR/${dict}.dict" ]; then
        cat "$DICT_FILES_DIR/${dict}.dict" >> $DICT_FILES_DIR/imagecreator_fuzzer.dict
    fi
done
