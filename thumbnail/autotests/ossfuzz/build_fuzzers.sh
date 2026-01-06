#!/bin/bash -eu
#
# SPDX-FileCopyrightText: 2025 Azhar Momin <azhar.momin@kdemail.net>
# SPDX-License-Identifier: LGPL-2.0-or-later

# ============== Base Dependencies ==============

rm -rf $WORK/*

export PATH="$WORK/bin:$WORK/libexec:$PATH"
export PKG_CONFIG="$(which pkg-config) --static"
export PKG_CONFIG_PATH="$WORK/lib/pkgconfig:$WORK/share/pkgconfig:$WORK/lib/x86_64-linux-gnu/pkgconfig"
if [[ $FUZZING_ENGINE == "afl" ]]; then
    export LDFLAGS="-fuse-ld=lld"
fi

cd $SRC/zlib
./configure \
    --static --prefix $WORK
make install -j$(nproc)

cd $SRC/freetype
./autogen.sh
CFLAGS="$CFLAGS -fPIC" \
CXXFLAGS="$CXXFLAGS -fPIC" \
./configure \
    --enable-static \
    --disable-shared \
    --prefix $WORK
make install -j$(nproc)

cd $SRC/libexpat/expat
cmake . -G Ninja \
  -DCMAKE_INSTALL_PREFIX=$WORK \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DEXPAT_BUILD_TESTS=OFF \
  -DEXPAT_BUILD_EXAMPLES=OFF
ninja install -j$(nproc)

cd $SRC/
tar xzf libpng-*.tar.gz && rm -f libpng-*.tar.gz
cd libpng-*
cmake . -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DPNG_SHARED=OFF \
    -DPNG_STATIC=ON \
    -DPNG_TESTS=OFF \
    -DPNG_TOOLS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
ninja install -j$(nproc)

cd $SRC/fontconfig
meson setup builddir \
  --prefix=$WORK \
  --default-library=static \
  --buildtype=plain \
  -Dtests=disabled \
  -Dtools=disabled \
  -Db_staticpic=true
ninja install -C builddir -j$(nproc)

# A workaround that lets QtGui find all transitive dependencies of Fontconfig correctly
sed -i 's|INTERFACE_INCLUDE_DIRECTORIES "${Fontconfig_INCLUDE_DIR}"|INTERFACE_INCLUDE_DIRECTORIES "${Fontconfig_INCLUDE_DIR}"\nINTERFACE_LINK_LIBRARIES "${PKG_FONTCONFIG_LINK_LIBRARIES}"|' \
    /usr/local/share/cmake-3.29/Modules/FindFontconfig.cmake

cd $SRC/qtbase
./configure -platform linux-clang-libc++ -prefix $WORK -qpa minimal -default-qpa minimal \
    -static -opensource -confirm-license -debug \
    -qt-pcre \
    -no-glib -no-icu -no-opengl -no-feature-sql \
    -no-feature-printsupport
ninja install -j$(nproc)

cd $SRC/extra-cmake-modules
cmake . -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_TESTING=OFF \
    -DBUILD_DOC=OFF
ninja install -j$(nproc)

cd $SRC/zstd
cmake -S build/cmake -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DZSTD_BUILD_SHARED=OFF \
    -DZSTD_BUILD_STATIC=ON \
    -DZSTD_BUILD_TESTS=OFF \
    -DZSTD_BUILD_PROGRAMS=OFF
ninja install -j$(nproc)

CFLAGS_SAVE="${CFLAGS}"
CXXFLAGS_SAVE="${CXXFLAGS}"
export AFL_NOOPT=1
unset CFLAGS
unset CXXFLAGS
cd $SRC/xz
./autogen.sh --no-po4a --no-doxygen
./configure --enable-static --disable-debug --disable-shared --disable-xz --disable-xzdec --disable-lzmainfo --prefix $WORK
make install -j$(nproc)
export CFLAGS="${CFLAGS_SAVE}"
export CXXFLAGS="${CXXFLAGS_SAVE}"
unset AFL_NOOPT

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
cp -f bzlib.h $WORK/include
cp -f libbz2.a $WORK/lib

cd $SRC/karchive
rm -rf poqm
cmake . -G Ninja \
  -DCMAKE_INSTALL_PREFIX=$WORK \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=OFF \
  -DWITH_OPENSSL=OFF
ninja install -j$(nproc)

cd $SRC/kconfig
rm -rf poqm
cmake . -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DKCONFIG_USE_QML=OFF \
    -DUSE_DBUS=OFF
ninja install -j$(nproc)

cd $SRC/kcoreaddons
rm -r poqm
cmake . -G Ninja \
  -DCMAKE_INSTALL_PREFIX=$WORK \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=OFF \
  -DBUILD_PYTHON_BINDINGS=OFF \
  -DUSE_DBUS=OFF \
  -DKCOREADDONS_USE_QML=OFF
ninja install -j$(nproc)

cd $SRC/ki18n
rm -r po
cmake . -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DBUILD_WITH_QML=OFF
ninja install -j$(nproc)

cd $SRC/kservice
rm -r po
cmake -B build -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF
ninja install -C build -j$(nproc)

cd $SRC/solid
rm -r poqm
cmake . -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DUSE_DBUS=OFF \
    -DUDEV_DISABLED=ON
ninja install -j$(nproc)

cd $SRC/kcrash
cmake . -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DWITH_X11=OFF
ninja install -j$(nproc)

cd $SRC/kwindowsystem
rm -r poqm
cmake . -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DKWINDOWSYSTEM_QML=OFF \
    -DKWINDOWSYSTEM_X11=OFF \
    -DKWINDOWSYSTEM_WAYLAND=OFF
ninja install -j$(nproc)

cd $SRC/kauth
rm -r poqm
cmake . -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF
ninja install -j$(nproc)

cd $SRC/kio
rm -r po
cmake . -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DKIOGUI_ONLY=ON \
    -DUSE_DBUS=OFF \
    -DWITH_X11=OFF \
    -DWITH_WAYLAND=OFF
ninja install -j$(nproc)

# ============== Thumbnailer Dependencies ==============

# For AppImageCreator
# cd $SRC/libarchive
# cmake . -G Ninja \
#     -DCMAKE_INSTALL_PREFIX=$WORK \
#     -DBUILD_SHARED_LIBS=OFF \
#     -DBUILD_TESTING=OFF
# ninja install -j$(nproc)

# cd $SRC/libfuse
# meson setup build \
#     --prefix=$WORK \
#     -Ddefault_library=static \
#     -Dbuildtype=plain \
#     -Dtests=false \
#     -Dexamples=false
# ninja install -C build -j$(nproc)

# # Inspired by https://github.com/AppImageCommunity/libappimage/blob/master/cmake/dependencies.cmake
# cd $SRC/squashfuse
# ./autogen.sh
# # off_t's size might differ, see https://stackoverflow.com/a/9073762
# sed -i "s/typedef off_t sqfs_off_t/typedef int64_t sqfs_off_t/g" common.h
# ./configure --disable-shared --enable-static --disable-demo --disable-high-level --without-lzo --without-lz4 --prefix $WORK
# make install -j$(nproc)

# cd $SRC/xdg-utils-cxx
# cmake . -G Ninja \
#     -DCMAKE_INSTALL_PREFIX=$WORK \
#     -DBUILD_SHARED_LIBS=OFF \
#     -DCMAKE_POSITION_INDEPENDENT_CODE=ON
# ninja install -j$(nproc)

# cd $SRC/
# tar xzf boost_*.tar.gz && rm -f boost_*.tar.gz
# cd boost_*
# ./bootstrap.sh
# ./b2 headers
# cp -r boost/ $WORK/include/

# cd $SRC/libappimage
# cmake . -G Ninja \
#     -DBUILD_SHARED_LIBS=OFF \
#     -DBUILD_TESTING=OFF \
#     -DCMAKE_INSTALL_PREFIX=$WORK \
#     -DLIBAPPIMAGE_DESKTOP_INTEGRATION_ENABLED=OFF \
#     -DLIBAPPIMAGE_THUMBNAILER_ENABLED=OFF \
#     -DLIBAPPIMAGE_STANDALONE=ON \
#     -DUSE_SYSTEM_SQUASHFUSE=ON \
#     -DUSE_SYSTEM_XDGUTILS=ON
# ninja install -j$(nproc)

# For AudioCreator
cd $SRC
tar -xzf utfcpp-*.tar.gz && rm -f utfcpp-*.tar.gz
cp -r utfcpp-*/source/* $WORK/include/

cd $SRC/taglib
cmake . -G Ninja \
  -DCMAKE_INSTALL_PREFIX=$WORK \
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
CXXFLAGS="-std=c++14" ./configure --disable-shared --enable-static --disable-desktopfiles --prefix $WORK
make install -j$(nproc)
cp $WORK/bin/ddjvu $OUT

export CFLAGS="${CFLAGS_SAVE}"
export CXXFLAGS="${CXXFLAGS_SAVE}"
unset AFL_NOOPT
# -- End Disable Instrumentation --

# For CursorCreator
cd $SRC
tar xzf xcb-proto-*.tar.gz && rm -f xcb-proto-*.tar.gz
cd xcb-proto-*
./configure --prefix $WORK
make install -j$(nproc)

cd $SRC
tar xzf xorgproto-*.tar.gz && rm -f xorgproto-*.tar.gz
cd xorgproto-*
./configure --disable-specs --without-xmlto --without-fop --without-xsltproc --prefix $WORK
make install -j$(nproc)

cd $SRC
tar xzf util-macros-*.tar.gz && rm -f util-macros-*.tar.gz
cd util-macros-*
./configure
make install -j$(nproc)

cd $SRC
tar xzf xtrans-*.tar.gz && rm -f xtrans-*.tar.gz
cd xtrans-*
./configure --disable-specs --disable-docs --without-xmlto --without-fop --without-xsltproc --prefix $WORK
make install -j$(nproc)

cd $SRC
tar xzf libXau-*.tar.gz && rm -f libXau-*.tar.gz
cd libXau-*
./configure --disable-shared --enable-static --prefix $WORK
make install -j$(nproc)

cd $SRC
tar xzf libxcb-*.tar.gz && rm -f libxcb-*.tar.gz
cd libxcb-*
./configure --disable-shared --enable-static --disable-devel-docs --without-doxygen --prefix $WORK
make install -j$(nproc)

cd $SRC
tar xzf libX11-*.tar.gz && rm -f libX11-*.tar.gz
cd libX11-*
./configure --disable-shared --enable-static --disable-specs --without-xmlto --without-fop --without-xsltproc --without-launchd --prefix $WORK
make install -j$(nproc)

cd $SRC
tar xzf libXrender-*.tar.gz && rm -f libXrender-*.tar.gz
cd libXrender-*
./configure --disable-shared --enable-static --prefix $WORK
make install -j$(nproc)

cd $SRC
tar xzf libXfixes-*.tar.gz && rm -f libXfixes-*.tar.gz
cd libXfixes-*
./configure --disable-shared --enable-static --prefix $WORK
make install -j$(nproc)

cd $SRC
tar xzf libXcursor-*.tar.gz && rm -f libXcursor-*.tar.gz
cd libXcursor-*
./configure --disable-shared --enable-static --prefix $WORK
make install -j$(nproc)

# For EXRCreator
cd $SRC/openexr
cmake -S . -B build -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF
ninja install -C build -j$(nproc)

# For JpegCreator
cd $SRC/brotli
cmake . -G Ninja \
  -DCMAKE_INSTALL_PREFIX=$WORK \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=OFF
ninja install -j$(nproc)

cd $SRC/fmt
cmake . -G Ninja \
  -DCMAKE_INSTALL_PREFIX=$WORK \
  -DBUILD_SHARED_LIBS=OFF \
  -DFMT_DOC=OFF \
  -DFMT_TEST=OFF
ninja install -j$(nproc)

cd $SRC/exiv2
cmake . -G Ninja \
  -DCMAKE_INSTALL_PREFIX=$WORK \
  -DBUILD_SHARED_LIBS=OFF \
  -DEXIV2_ENABLE_DYNAMIC_RUNTIME=OFF \
  -DEXIV2_BUILD_EXIV2_COMMAND=OFF \
  -DEXIV2_ENABLE_INIH=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON
ninja install -j$(nproc)

# This is later required for building jpegcreator_fuzzer and libkexiv2
# Ideally we should be passing CMAKE_MODULE_PATH but for some reason that doesn't work
cp cmake/FindBrotli.cmake $WORK/share/ECM/find-modules/

cd $SRC/libkexiv2
cmake . -G Ninja \
  -DCMAKE_INSTALL_PREFIX=$WORK \
  -DBUILD_SHARED_LIBS=OFF \
  -DQT_MAJOR_VERSION=6 \
  -DBUILD_TESTING=OFF
ninja install -j$(nproc)

# For TextCreator
cd $SRC/syntax-highlighting
rm -rf poqm
# disable binaries
sed -i '/add_subdirectory(cli)/d' src/CMakeLists.txt
sed -i '/add_subdirectory(examples)/d' CMakeLists.txt
cmake . -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF
ninja install -j$(nproc)

# For SvgCreator
cd $SRC/qtsvg
cmake . -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_SHARED_LIBS=OFF
ninja install -j$(nproc)

# ============== Building Fuzzers ==============

cd $SRC/kio-extras
# Disable some of the modules which are not needed for fuzzing
disabled_subs=(filter info archive fish man)
for sub in "${disabled_subs[@]}"; do
    sed -i "/^[[:space:]]*add_subdirectory( ${sub} )/d" CMakeLists.txt
done
rm -r po
cmake -B build -G Ninja \
    -DCMAKE_PREFIX_PATH=$WORK \
    -DCMAKE_INSTALL_PREFIX=$WORK \
    -DBUILD_FUZZERS=ON \
    -DFUZZERS_USE_QT_MINIMAL_INTEGRATION_PLUGIN=ON \
    -DBUILD_TESTING=OFF \
    -DBUILD_ACTIVITIES=OFF \
    -DBUILD_KCMS=OFF \
    -DUSE_DBUS=OFF \
    -DWITH_LIBPROXY=OFF \
    -DBUILD_DOC=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
ninja -C build -j$(nproc)

# EXTENSIONS="appimagethumbnail_fuzzer AppImage
EXTENSIONS="audiothumbnail_fuzzer flac mp3 m4a ogg wav aif ape wma mpc opus spx wv mod s3m xm it aax aaxc
            comicbookthumbnail_fuzzer cbz cbr cb7 cbt
            cursorthumbnail_fuzzer cursor
            djvuthumbnail_fuzzer djvu djv
            ebookthumbnail_fuzzer epub fb2 fbz
            exrthumbnail_fuzzer exr
            imagethumbnail_fuzzer bmp gif png pbm ppm xbm xpm webp heif avif jp2 jxl psd xcf dds tga qoi tiff
            jpegthumbnail_fuzzer jpg jpeg
            kraorathumbnail_fuzzer kra krz ora
            opendocumentthumbnail_fuzzer odt ods odp odg odf docx xlsx pptx ppsx xps oxps 3mf printticket
            svgthumbnail_fuzzer svg svgz
            textthumbnail_fuzzer txt text log md ini conf cfg csv sh
            windowsexethumbnail_fuzzer exe dll cpl
            windowsimagethumbnail_fuzzer ico cur ani"

echo "$EXTENSIONS" | while read fuzzer_name extensions; do
    # copy the fuzzer binary
    cp build/bin/fuzzers/$fuzzer_name $OUT

    # create seed corpus
    for extension in $extensions; do
        find . -name "*.$extension" -exec zip -q "$OUT/${fuzzer_name}_seed_corpus.zip" {} +
    done

    # copy dict if it exists
    if [ -f "autotests/data/dict/$fuzzer_name.dict" ]; then
      cp "autotests/data/dict/$fuzzer_name.dict" $OUT
    fi
done
