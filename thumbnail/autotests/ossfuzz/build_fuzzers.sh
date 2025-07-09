#!/bin/bash -eu
#
# SPDX-FileCopyrightText: 2025 Azhar Momin <azhar.momin@kdemail.net>
# SPDX-License-Identifier: LGPL-2.0-or-later

# ============== Base Dependencies ==============

cd $SRC/qtbase
./configure -platform linux-clang-libc++ -prefix /usr -qpa minimal -default-qpa minimal \
    -static -opensource -confirm-license -debug \
    -qt-pcre -qt-zlib \
    -no-glib -no-icu -no-opengl -no-feature-sql \
    -no-feature-dbus -no-feature-printsupport
ninja install -j$(nproc)

cd $SRC/extra-cmake-modules
cmake . -G Ninja \
    -DBUILD_TESTING=OFF \
    -DBUILD_DOC=OFF
ninja install -j$(nproc)

cd $SRC/kcoreaddons
rm -r poqm
cmake . -G Ninja \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=OFF \
  -DBUILD_PYTHON_BINDINGS=OFF \
  -DUSE_DBUS=OFF \
  -DKCOREADDONS_USE_QML=OFF
ninja install -j$(nproc)

# ============== Thumbnailer Dependencies ==============

# For AppImageCreator
CFLAGS_SAVE="$CFLAGS"
CXXFLAGS_SAVE="$CXXFLAGS"

export CFLAGS="-O1 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION"
export CXXFLAGS="${CFLAGS} ${CXXFLAGS_EXTRA}"
export AFL_NOOPT=1

cd $SRC/glib
meson setup build \
    --default-library=static \
    --buildtype=plain \
    -Dtests=false \
    -Dman-pages=disabled \
    -Dsysprof=disabled
ninja install -C build -j$(nproc)

cd $SRC/libxml2
cmake . -G Ninja \
  -DBUILD_SHARED_LIBS=OFF
ninja install -j$(nproc)

cd $SRC/freetype
./autogen.sh
./configure --disable-shared
make install -j$(nproc)

cd $SRC/fontconfig
meson setup build \
    --default-library=static \
    --buildtype=plain \
    -Dtests=disabled \
    -Dtools=disabled
ninja install -C build -j$(nproc)

cd $SRC/cairo
meson setup build \
  --default-library=static \
  --buildtype=plain \
  -Dpixman:tests=disabled \
  -Dtests=disabled
ninja install -C build -j$(nproc)

cd $SRC/harfbuzz
meson setup build \
    --default-library=static \
    --buildtype=plain \
    -Dtests=disabled
ninja install -C build -j$(nproc)

cd $SRC/pango
meson setup build \
    --default-library=static \
    --buildtype=plain
ninja install -C build -j$(nproc)

# disable instrumentation for librsvg (because enabling causes the build to fail)
export RUSTUP_TOOLCHAIN=nightly-2025-02-20
export RUSTFLAGS=""

cd $SRC/librsvg
meson setup build \
    --default-library=static \
    --buildtype=plain \
    -Ddocs=disabled \
    -Dintrospection=disabled \
    -Dvala=disabled \
    -Dpixbuf=disabled \
    -Dtests=false
ninja install -C build -j$(nproc)

export CFLAGS="${CFLAGS_SAVE}"
export CXXFLAGS="${CXXFLAGS_SAVE}"
unset AFL_NOOPT

cd $SRC/zstd
cmake -S build/cmake -G Ninja \
    -DBUILD_SHARED_LIBS=OFF
ninja install -j$(nproc)

cd $SRC/libarchive
cmake . -G Ninja \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF
ninja install -j$(nproc)

CFLAGS_SAVE="${CFLAGS}"
CXXFLAGS_SAVE="${CXXFLAGS}"
export AFL_NOOPT=1
unset CFLAGS
unset CXXFLAGS
cd $SRC/xz
./autogen.sh --no-po4a --no-doxygen
./configure --enable-static --disable-debug --disable-shared --disable-xz --disable-xzdec --disable-lzmainfo
make install -j$(nproc)
export CFLAGS="${CFLAGS_SAVE}"
export CXXFLAGS="${CXXFLAGS_SAVE}"
unset AFL_NOOPT

cd $SRC/libfuse
meson setup build \
    -Ddefault_library=static \
    -Dbuildtype=plain \
    -Dtests=false \
    -Dexamples=false
ninja install -C build -j$(nproc)

# Inspired by https://github.com/AppImageCommunity/libappimage/blob/master/cmake/dependencies.cmake
cd $SRC/squashfuse
./autogen.sh
# off_t's size might differ, see https://stackoverflow.com/a/9073762
sed -i "s/typedef off_t sqfs_off_t/typedef int64_t sqfs_off_t/g" common.h
./configure --disable-shared --enable-static --disable-demo --disable-high-level --without-lzo --without-lz4
make install -j$(nproc)

cd $SRC/xdg-utils-cxx
cmake . -G Ninja \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
ninja install -j$(nproc)

cd $SRC/
tar xzf boost_*.tar.gz && rm -f boost_*.tar.gz
cd boost_*
./bootstrap.sh
./b2 headers
cp -r boost/ /usr/local/include/

cd $SRC/libappimage

# Build only static libappimage
sed -i '/add_library(libappimage SHARED/d' src/libappimage/CMakeLists.txt
sed -i 's/foreach(target libappimage libappimage_static)/foreach(target libappimage_static)/' src/libappimage/CMakeLists.txt
sed -i '/# install libappimage/,/)/d' src/libappimage/CMakeLists.txt
sed -i 's/libappimage libappimage_shared/libappimage_shared/' src/libappimage/CMakeLists.txt
sed -i 's/TARGET libappimage /TARGET ${target} /' src/libappimage/CMakeLists.txt

cmake . -G Ninja \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DLIBAPPIMAGE_DESKTOP_INTEGRATION_ENABLED=OFF \
    -DLIBAPPIMAGE_THUMBNAILER_ENABLED=ON \
    -DLIBAPPIMAGE_STANDALONE=ON \
    -DUSE_SYSTEM_SQUASHFUSE=ON \
    -DUSE_SYSTEM_XDGUTILS=ON
ninja install -j$(nproc)
cp src/libappimage/libappimage_static.a /usr/local/lib/

# For AudioCreator
cd $SRC
tar -xzf utfcpp-*.tar.gz && rm -f utfcpp-*.tar.gz
cp -r utfcpp-*/source/* /usr/include/

cd $SRC/taglib
cmake . -G Ninja \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=OFF
ninja install -j$(nproc)

# -- Disable Instrumentation --
CFLAGS_SAVE="$CFLAGS"
CXXFLAGS_SAVE="$CXXFLAGS"
unset CFLAGS
unset CXXFLAGS
export AFL_NOOPT=1

# For ComicCreator
cd $SRC
tar xzf unrarsrc-*.tar.gz && rm -f unrarsrc-*.tar.gz
cd unrar
make -f makefile -j$(nproc) \
  LINK='c++ -static' \
  LDFLAGS='-static -pthread' \
  CXXFLAGS='-static -O2'
cp unrar $OUT

# For DjVuCreator
cd $SRC
tar xzf djvulibre-*.tar.gz && rm -f djvulibre-*.tar.gz
cd djvulibre-*
CXXFLAGS="-std=c++14" ./configure --disable-shared --enable-static --disable-desktopfiles
make install -j$(nproc)
cp /usr/local/bin/ddjvu $OUT

export CFLAGS="${CFLAGS_SAVE}"
export CXXFLAGS="${CXXFLAGS_SAVE}"
unset AFL_NOOPT
# -- End Disable Instrumentation --

# For CursorCreator
cd $SRC
tar xzf xcb-proto-*.tar.gz && rm -f xcb-proto-*.tar.gz
cd xcb-proto-*
./configure
make install -j$(nproc)

cd $SRC
tar xzf xorgproto-*.tar.gz && rm -f xorgproto-*.tar.gz
cd xorgproto-*
./configure --disable-specs --without-xmlto --without-fop --without-xsltproc
make install -j$(nproc)

cd $SRC
tar xzf util-macros-*.tar.gz && rm -f util-macros-*.tar.gz
cd util-macros-*
./configure
make install -j$(nproc)

cd $SRC
tar xzf xtrans-*.tar.gz && rm -f xtrans-*.tar.gz
cd xtrans-*
./configure --disable-specs --disable-docs --without-xmlto --without-fop --without-xsltproc
make install -j$(nproc)

cd $SRC
tar xzf libXau-*.tar.gz && rm -f libXau-*.tar.gz
cd libXau-*
./configure --disable-shared --enable-static
make install -j$(nproc)

cd $SRC
tar xzf libxcb-*.tar.gz && rm -f libxcb-*.tar.gz
cd libxcb-*
./configure --disable-shared --enable-static --disable-devel-docs --without-doxygen
make install -j$(nproc)

cd $SRC
tar xzf libX11-*.tar.gz && rm -f libX11-*.tar.gz
cd libX11-*
./configure --disable-shared --enable-static --disable-specs --without-xmlto --without-fop --without-xsltproc --without-launchd
make install -j$(nproc)

cd $SRC
tar xzf libXrender-*.tar.gz && rm -f libXrender-*.tar.gz
cd libXrender-*
./configure --disable-shared --enable-static
make install -j$(nproc)

cd $SRC
tar xzf libXfixes-*.tar.gz && rm -f libXfixes-*.tar.gz
cd libXfixes-*
./configure --disable-shared --enable-static
make install -j$(nproc)

cd $SRC
tar xzf libXcursor-*.tar.gz && rm -f libXcursor-*.tar.gz
cd libXcursor-*
./configure --disable-shared --enable-static
make install -j$(nproc)

# For ComicCreator, EbookCreator, KritaCreator, and OpenDocumentCreator
cd $SRC
tar xzf bzip2-*.tar.gz && rm -f bzip2-*.tar.gz
cd bzip2-*
SRCL=(blocksort.o huffman.o crctable.o randtable.o compress.o decompress.o bzlib.o)
for source in ${SRCL[@]}; do
    name=$(basename $source .o)
    $CC $CFLAGS -c ${name}.c
done
rm -f libbz2.a
ar cq libbz2.a ${SRCL[@]}
cp -f bzlib.h /usr/local/include
cp -f libbz2.a /usr/local/lib

cd $SRC/karchive
rm -rf poqm
cmake . -G Ninja \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=OFF \
  -DWITH_OPENSSL=OFF
ninja install -j$(nproc)

# For EXRCreator
cd $SRC/openexr
cmake -S . -B build -G Ninja \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF
ninja install -C build -j$(nproc)

# For JpegCreator
cd $SRC/brotli
cmake . -G Ninja \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=OFF
ninja install -j$(nproc)

cd $SRC/libexpat/expat
cmake . -G Ninja \
  -DBUILD_SHARED_LIBS=OFF \
  -DEXPAT_BUILD_TESTS=OFF \
  -DEXPAT_BUILD_EXAMPLES=OFF
ninja install -j$(nproc)

cd $SRC/fmt
cmake . -G Ninja \
  -DBUILD_SHARED_LIBS=OFF \
  -DFMT_DOC=OFF \
  -DFMT_TEST=OFF
ninja install -j$(nproc)

cd $SRC/exiv2
cmake . -G Ninja \
  -DBUILD_SHARED_LIBS=OFF \
  -DEXIV2_ENABLE_DYNAMIC_RUNTIME=OFF \
  -DEXIV2_BUILD_EXIV2_COMMAND=OFF \
  -DEXIV2_ENABLE_INIH=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON
ninja install -j$(nproc)

cd $SRC/libkexiv2
cmake . -G Ninja \
  -DBUILD_SHARED_LIBS=OFF \
  -DQT_MAJOR_VERSION=6 \
  -DBUILD_TESTING=OFF
ninja install -j$(nproc)

# For TextCreator
cd $SRC/kconfig
rm -rf poqm
cmake . -G Ninja \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DKCONFIG_USE_QML=OFF \
    -DUSE_DBUS=OFF
ninja install -j$(nproc)

cd $SRC/syntax-highlighting
rm -rf poqm
cmake . -G Ninja \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF
ninja install -j$(nproc)

# For SvgCreator
cd $SRC/qtsvg
cmake . -G Ninja \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX=/usr
ninja install -j$(nproc)

# ============== Setting Up KIO ==============

mkdir -p /usr/local/include/KF6/KIO/KIO
/usr/libexec/moc $SRC/kio/src/gui/thumbnailcreator.h -o /usr/local/include/KF6/KIO/moc_thumbnailcreator.cpp

cp $SRC/kio/src/gui/thumbnailcreator.h /usr/local/include/KF6/KIO/KIO/ThumbnailCreator

cat <<EOF > /usr/local/include/KF6/KIO/KIO/Global
#ifndef KIO_GLOBAL_H
#define KIO_GLOBAL_H
namespace KIO
{
/// 64-bit file size
typedef qulonglong filesize_t;
}
#endif // KIO_GLOBAL_H
EOF

cat <<EOF > /usr/local/include/KF6/KIO/kiogui_export.h
#ifndef KIOGUI_EXPORT_H
#define KIOGUI_EXPORT_H
// dummy export macros
#define KIOGUI_EXPORT
#define KIOGUI_NO_EXPORT
#endif // KIOGUI_EXPORT_H
EOF

# ============== Building Fuzzers ==============

cd $SRC/kio-extras
THUMBNAIL_CREATORS="AppImageCreator AppImage
        AudioCreator mp3 flac m4a aax ape wv vw mpc ogg aiff aifc wav
        ComicCreator cbz cbr cb7 cbt
        CursorCreator cursor
        DjVuCreator djvu djv
        EbookCreator epub fb2 fbz
        EXRCreator exr
        ImageCreator bmp gif png pbm ppm xbm xpm webp heif avif jp2 jxl psd xcf dds tga qoi tiff
        JpegCreator jpg jpeg
        KritaCreator kra krz ora
        OpenDocumentCreator odt ods op odg odf docx xlsx pptx ppsx xps oxps 3mf printticket
        TextCreator txt text log md ini conf cfg csv sh
        SvgCreator svg svgz
        WindowsExeCreator exe dll cpl
        WindowsImageCreator ico cur ani"

$SRC/kconfig/bin/kconfig_compiler_kf6 \
    $SRC/kio-extras/thumbnail/jpegcreatorsettings5.kcfg \
    $SRC/kio-extras/thumbnail/jpegcreatorsettings5.kcfgc \
    -d $SRC/kio-extras/thumbnail

cat <<EOF > $SRC/kio-extras/thumbnail/config-thumbnail.h
#ifndef CONFIG_THUMBNAIL_H
#define CONFIG_THUMBNAIL_H
#define HAVE_KEXIV2 1
#endif // CONFIG_THUMBNAIL_H
EOF

echo "$THUMBNAIL_CREATORS" | while read creator formats; do
(
    creator_name=$(echo "${creator/Creator/}" | tr '[:upper:]' '[:lower:]')
    creator_filename="${creator_name}creator"
    fuzz_target_name=kde_thumbnailers_${creator_filename}_fuzzer

    /usr/libexec/moc $SRC/kio-extras/thumbnail/${creator_filename}.cpp -o $SRC/kio-extras/thumbnail/${creator_filename}.moc
    /usr/libexec/moc $SRC/kio-extras/thumbnail/${creator_filename}.h -o $SRC/kio-extras/thumbnail/moc_${creator_filename}.cpp

    cat <<EOF > $SRC/kio-extras/thumbnail/thumbnail-${creator_name}-logsettings.h
#pragma once
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(KIO_THUMBNAIL_$(echo "$creator_name" | tr '[:lower:]' '[:upper:]')_LOG)
inline Q_LOGGING_CATEGORY(KIO_THUMBNAIL_$(echo "$creator_name" | tr '[:lower:]' '[:upper:]')_LOG, "kio.thumbnail.${creator_name}")
EOF

    # TODO: -lcrypto shouldn't be needed, fix that in KArchive
    $CXX $CXXFLAGS -std=c++17 -fPIC -DCREATOR=$creator $SRC/kio-extras/thumbnail/autotests/ossfuzz/kde_thumbnailers_fuzzer.cc \
    $SRC/kio/src/gui/thumbnailcreator.cpp $SRC/kio-extras/thumbnail/${creator_filename}.cpp \
    $SRC/kio-extras/thumbnail/exeutils.cpp $SRC/kio-extras/thumbnail/icoutils.cpp \
    $SRC/kio-extras/thumbnail/jpegcreatorsettings5.cpp \
    -o $OUT/$fuzz_target_name \
    -I /usr/include/QtCore -I /usr/include/QtGui \
    -I /usr/local/include/KF6/KCoreAddons -I /usr/local/include/KF6/KIO -I $SRC/kio-extras/thumbnail \
    -I /usr/local/include/taglib -I /usr/local/include/KF6/KArchive \
    -I /usr/local/include/OpenEXR -I /usr/local/include/Imath -I /usr/local/include/KExiv2Qt6 \
    -I /usr/local/include/KF6/KConfig -I /usr/local/include/KF6/KConfigCore -I /usr/local/include/KF6/KConfigGui \
    -I /usr/local/include/KF6/KSyntaxHighlighting -I /usr/include/QtSvg  \
    -L /usr/plugins/platforms -L /usr/local/lib/x86_64-linux-gnu \
    -lappimage_static -lappimage_hashlib -lXdgUtilsDesktopEntry -lXdgUtilsBaseDir \
    -larchive -lsquashfuse -lfuse3 -lcairo -lrsvg-2 \
    -ltag -lKF6Archive -lbz2 -llzma -lzstd -lcrypto \
    -lXcursor -lXfixes -lXrender -lX11 -lxcb -lXau \
    -lOpenEXR-3_3 -lIex-3_3 -lImath-3_1 -lIlmThread-3_3 -lOpenEXRCore-3_3 -lOpenEXRUtil-3_3 \
    -lKExiv2Qt6 -lexiv2 -lfmt -lexpat -lbrotlienc -lbrotlidec -lbrotlicommon -lz \
    -lKF6ConfigGui -lKF6ConfigCore -lKF6SyntaxHighlighting -lQt6Svg \
    -lKF6CoreAddons \
    -lQt6Xml -lQt6Widgets -lQt6Network -lqminimal -lQt6Gui -lQt6Core \
    -lQt6BundledPcre2 -lQt6BundledZLIB \
    -lQt6BundledFreetype -lQt6BundledHarfbuzz -lQt6BundledLibpng \
    -lm -ldl -lpthread $LIB_FUZZING_ENGINE

    echo "$formats" | tr ' ' '\n' | while read format; do
        files=$(find . -name "*.${format}" -type f)
        if [ -n "$files" ]; then
          echo "$files" | zip -q $OUT/${fuzz_target_name}_seed_corpus.zip -@
        else
          echo "no files found with extension .$format for $fuzz_target_name seed corpus"
        fi
    done
)
done
