# SPDX-FileCopyrightText: 2025 Azhar Momin <azhar.momin@kdemail.net>
# SPDX-License-Identifier: BSD-2-Clause

if(DEFINED ENV{LIB_FUZZING_ENGINE})
    set(fuzzing_engine $ENV{LIB_FUZZING_ENGINE})
else()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(fuzzing_engine -fsanitize=fuzzer)
    else()
        message(FATAL_ERROR "Fuzzing engine not supported")
    endif()
endif()

function(add_thumbnail_fuzzer creator creator_header target_lib)
    set(gen_src ${CMAKE_CURRENT_BINARY_DIR}/${target_lib}_fuzzer.cc)
    set(CREATOR "${creator}")
    set(CREATOR_HEADER "${creator_header}")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/kde_thumbnailers_fuzzer.cc.in
        ${gen_src}
        @ONLY
    )

    add_executable(${target_lib}_fuzzer
        ${gen_src}
    )

    kcoreaddons_target_static_plugins(${target_lib}_fuzzer
        LINK_OPTION PRIVATE
        TARGETS ${target_lib}
    )

    target_link_libraries(${target_lib}_fuzzer
        PRIVATE
            ${fuzzing_engine}
    )

    if(FUZZERS_USE_QT_MINIMAL_INTEGRATION_PLUGIN)
        qt_import_plugins(${target_lib}_fuzzer INCLUDE Qt::QMinimalIntegrationPlugin)
    endif()

    set_target_properties(${target_lib}_fuzzer PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/fuzzers
    )
endfunction()

add_thumbnail_fuzzer(ImageCreator imagecreator.h imagethumbnail)

add_thumbnail_fuzzer(JpegCreator jpegcreator.h jpegthumbnail)

add_thumbnail_fuzzer(SvgCreator svgcreator.h svgthumbnail)

add_thumbnail_fuzzer(TextCreator textcreator.h textthumbnail)

if(NOT WIN32)
    add_thumbnail_fuzzer(DjVuCreator djvucreator.h djvuthumbnail)
endif()

if(OpenEXR_FOUND)
    add_thumbnail_fuzzer(EXRCreator exrcreator.h exrthumbnail)
endif()

if(X11_Xcursor_FOUND AND NOT WITHOUT_X11)
    add_thumbnail_fuzzer(CursorCreator cursorcreator.h cursorthumbnail)
endif()

add_thumbnail_fuzzer(WindowsExeCreator windowsexecreator.h windowsexethumbnail)

add_thumbnail_fuzzer(WindowsImageCreator windowsimagecreator.h windowsimagethumbnail)

add_thumbnail_fuzzer(ComicCreator comiccreator.h comicbookthumbnail)

add_thumbnail_fuzzer(KritaCreator kritacreator.h kraorathumbnail)

add_thumbnail_fuzzer(AudioCreator audiocreator.h audiothumbnail)

add_thumbnail_fuzzer(OpenDocumentCreator opendocumentcreator.h opendocumentthumbnail)

if(libappimage_FOUND)
    add_thumbnail_fuzzer(AppImageCreator appimagecreator.h appimagethumbnail)
endif()

add_thumbnail_fuzzer(EbookCreator ebookcreator.h ebookthumbnail)
