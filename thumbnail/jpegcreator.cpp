/*  This file is part of the KDE libraries
    Copyright (C) 2008 Andre Gemünd <scroogie@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "jpegcreator.h"

#include <cstdio>
#include <csetjmp>
#include <jpeglib.h>
#include <QFile>
#include <QImage>
#include <kdemacros.h>

extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new JpegCreator;
    }
}

struct jpeg_custom_error_mgr
{
   struct jpeg_error_mgr builtin;
   jmp_buf setjmp_buffer;
};

void jpeg_custom_error_callback(j_common_ptr jpegDecompress)
{
   jpeg_custom_error_mgr *custom_err = (jpeg_custom_error_mgr *)jpegDecompress->err;

   // jump to error recovery (fallback to old method)
   longjmp(custom_err->setjmp_buffer, 1);
}

JpegCreator::JpegCreator()
{
}

/**
 * This is a faster thumbnail creation specifically for JPEG images, as it uses the libjpeg feature of 
 * calculating the inverse dct for a part of coefficients for lower resolutions.
 * Interesting parameters are the quality settings of libjpeg
 *         jpegDecompress.do_fancy_upsampling (TRUE, FALSE)
 *         jpegDecompress.do_block_smoothing (TRUE, FALSE)
 *         jpegDecompress.dct_method (JDCT_IFAST, JDCT_ISLOW, JDCT_IFLOAT)
 * and the resampling parameter of QImage.
 *
 * Important: We do not need to scaled to exact dimesions, as thumbnail.cpp will check dimensions and 
 * rescale anyway.
 */
bool JpegCreator::create(const QString &path, int width, int height, QImage &img)
{
    const QByteArray name = QFile::encodeName(path);
    FILE *jpegFile = fopen(name.constData(), "rb");
    if (jpegFile  == 0) {
       return false;
    }

    // create jpeglib data structures and calculate scale denominator
    struct jpeg_decompress_struct jpegDecompress;
    struct jpeg_custom_error_mgr jpegError;
    jpegDecompress.err = jpeg_std_error(&jpegError.builtin);
    jpeg_create_decompress(&jpegDecompress);
    jpeg_stdio_src(&jpegDecompress, jpegFile);
    jpeg_read_header(&jpegDecompress, TRUE);

    const double ratioWidth = jpegDecompress.image_width / (double)width;
    const double ratioHeight = jpegDecompress.image_height / (double)height;
    int scale = 1;
    if (ratioWidth > 7 || ratioHeight > 7) {
        scale = 8;
    } else if (ratioWidth > 3.5 || ratioHeight > 3.5) {
        scale = 4;
    } else if (ratioWidth > 1.75 || ratioHeight > 1.75) {
        scale = 2;
    }

    // set jpeglib decompression parameters
    jpegDecompress.scale_num           = 1;
    jpegDecompress.scale_denom         = scale;	
    jpegDecompress.do_fancy_upsampling = FALSE;
    jpegDecompress.do_block_smoothing  = FALSE;
    jpegDecompress.dct_method          = JDCT_IFAST;
    jpegDecompress.err->error_exit     = jpeg_custom_error_callback;
    jpegDecompress.out_color_space     = JCS_RGB;

    jpeg_calc_output_dimensions(&jpegDecompress);

    if (setjmp(jpegError.setjmp_buffer)) {
        jpeg_abort_decompress(&jpegDecompress);
        fclose(jpegFile);
        // libjpeg version failed, fall back to direct loading of QImage
        if (!img.load(path)) {
            return false;
        }
        if (img.depth() != 32) {
            img = img.convertToFormat(QImage::Format_RGB32);
        }
        return true;
    }

    jpeg_start_decompress(&jpegDecompress);
    img = QImage(jpegDecompress.output_width, jpegDecompress.output_height, QImage::Format_RGB32);
    uchar *buffer = img.bits();
    const int bpl = img.bytesPerLine();
    while (jpegDecompress.output_scanline < jpegDecompress.output_height) {
        // advance line-pointer to next line
        uchar *line = buffer + jpegDecompress.output_scanline * bpl;
        jpeg_read_scanlines(&jpegDecompress, &line, 1);
    }
    jpeg_finish_decompress(&jpegDecompress);

    // align correctly for QImage
    // code copied from Gwenview and digiKam
    for (uint i = 0; i < jpegDecompress.output_height; ++i) {
        uchar *in = img.scanLine(i) + jpegDecompress.output_width * 3;
        QRgb *out = (QRgb*)img.scanLine(i);
        for (uint j = jpegDecompress.output_width; j > 0; --j) {
            in -= 3;
            out[j] = qRgb(in[0], in[1], in[2]);
        }
    }
    
    fclose(jpegFile);
    jpeg_destroy_decompress(&jpegDecompress);
    return true;
}

ThumbCreator::Flags JpegCreator::flags() const
{
    return None;
}
