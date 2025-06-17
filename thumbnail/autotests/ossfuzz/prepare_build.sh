#!/bin/bash -eu
#
# SPDX-FileCopyrightText: 2025 Azhar Momin <azhar.momin@kdemail.net>
# SPDX-License-Identifier: LGPL-2.0-or-later

apt-get update && \
    apt-get install -y cmake make autoconf automake autopoint libtool \
    wget po4a pkg-config perl python3 gperf

pip3 install meson ninja

rustup default nightly-2025-02-20
RUSTUP_TOOLCHAIN=nightly-2025-02-20 cargo install cargo-c

# Base
git clone --depth 1 -b dev git://code.qt.io/qt/qtbase.git
git clone --depth 1 https://invent.kde.org/frameworks/extra-cmake-modules.git
git clone --depth 1 https://invent.kde.org/frameworks/kcoreaddons.git
# For AppImageCreator
git clone --depth 1 https://github.com/madler/zlib.git
git clone --depth 1 https://gitlab.gnome.org/GNOME/glib.git
git clone --depth 1 https://gitlab.freedesktop.org/freetype/freetype.git
git clone --depth 1 https://gitlab.gnome.org/GNOME/libxml2.git
git clone --depth 1 https://gitlab.freedesktop.org/fontconfig/fontconfig.git
git clone --depth 1 https://gitlab.freedesktop.org/cairo/cairo.git
git clone --depth 1 https://github.com/harfbuzz/harfbuzz.git
git clone --depth 1 https://gitlab.gnome.org/GNOME/pango.git
git clone --depth 1 https://gitlab.gnome.org/GNOME/librsvg.git
git clone --depth 1 https://github.com/facebook/zstd.git
git clone --depth 1 https://github.com/tukaani-project/xz.git
git clone --depth 1 https://github.com/libarchive/libarchive.git
git clone --depth 1 https://github.com/libfuse/libfuse.git
git clone --depth 1 https://github.com/vasi/squashfuse.git
git clone --depth 1 https://github.com/azubieta/xdg-utils-cxx.git
wget https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
git clone --depth 1 https://github.com/AppImageCommunity/libappimage.git
# For AudioCreator
wget https://github.com/nemtrif/utfcpp/archive/refs/tags/v4.0.6.tar.gz -O utfcpp-4.0.6.tar.gz
git clone --depth 1 https://github.com/taglib/taglib.git
# For ComicCreator
wget https://www.rarlab.com/rar/unrarsrc-7.1.7.tar.gz
# For CursorCreator
wget https://www.x.org/releases/individual/proto/xcb-proto-1.17.0.tar.gz
wget https://www.x.org/releases/individual/proto/xorgproto-2024.1.tar.gz
wget https://www.x.org/releases/individual/util/util-macros-1.20.2.tar.gz
wget https://www.x.org/releases/individual/lib/xtrans-1.6.0.tar.gz
wget https://www.x.org/releases/individual/lib/libXau-1.0.12.tar.gz
wget https://www.x.org/releases/individual/lib/libxcb-1.17.0.tar.gz
wget https://www.x.org/releases/individual/lib/libX11-1.8.12.tar.gz
wget https://www.x.org/releases/individual/lib/libXrender-0.9.12.tar.gz
wget https://www.x.org/releases/individual/lib/libXfixes-6.0.1.tar.gz
wget https://www.x.org/releases/individual/lib/libXcursor-1.2.3.tar.gz
# For DjVuCreator
wget http://downloads.sourceforge.net/djvu/djvulibre-3.5.28.tar.gz
# For ComicCreator, EbookCreator, KritaCreator, and OpenDocumentCreator
wget https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz
git clone --depth 1 https://invent.kde.org/frameworks/karchive.git
# For EXRCreator
git clone --depth 1 --branch=RB-3.3 https://github.com/AcademySoftwareFoundation/openexr.git
# For JpegCreator
git clone --depth 1 https://github.com/google/brotli.git
git clone --depth 1 https://github.com/libexpat/libexpat.git
git clone --depth 1 https://github.com/fmtlib/fmt.git
git clone --depth 1 https://github.com/exiv2/exiv2.git
git clone --depth 1 https://invent.kde.org/graphics/libkexiv2.git
# For TextCreator
git clone --depth 1 https://invent.kde.org/frameworks/kconfig.git
git clone --depth 1 https://invent.kde.org/frameworks/syntax-highlighting.git
# For SvgCreator
git clone --depth 1 -b dev git://code.qt.io/qt/qtsvg.git
# KIO
git clone --depth 1 https://invent.kde.org/frameworks/kio.git

# Files for corpus
CORPUS_DIR=$SRC/kio-extras/thumbnail/autotests/data/corpus

wget -P $CORPUS_DIR https://download.kde.org/stable/kdenlive/24.12/linux/kdenlive-24.12.3-x86_64.AppImage || echo "Downloading appimage failed"
wget -P $CORPUS_DIR https://www.sndjvu.org/DjVu3Spec.djvu || echo "Downloading djvu spec failed"
git clone --depth 1 https://github.com/AcademySoftwareFoundation/openexr-images.git $CORPUS_DIR/openexr-images || echo "Downloading openexr-images failed"
wget -P $CORPUS_DIR https://kde.org/favicon.ico || echo "Downloading favicon failed"
