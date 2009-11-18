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
#include <QFile>
#include <QImage>
#include <kdemacros.h>

#ifdef HAVE_EXIV2
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#endif

extern "C"
{
    #include <jpeglib.h>

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

#ifdef Q_CC_MSVC
typedef struct
{
    struct jpeg_source_mgr pub;
    JOCTET eoi[2];
} jpeg_custom_source_mgr;

void init_source (j_decompress_ptr cinfo)
{
}

boolean fill_input_buffer (j_decompress_ptr cinfo)
{
    jpeg_custom_source_mgr* src = (jpeg_custom_source_mgr*) cinfo->src;

    /* Create a fake EOI marker */
    src->eoi[0] = (JOCTET) 0xFF;
    src->eoi[1] = (JOCTET) JPEG_EOI;
    src->pub.next_input_byte = src->eoi;
    src->pub.bytes_in_buffer = 2;

    return true;
}

void skip_input_data (j_decompress_ptr cinfo, long nbytes)
{
    jpeg_custom_source_mgr* src = (jpeg_custom_source_mgr*) cinfo->src;

    if (nbytes > 0)
    {
        while (nbytes > (long) src->pub.bytes_in_buffer)
        {
            nbytes -= (long) src->pub.bytes_in_buffer;
            (void) fill_input_buffer(cinfo);
        }
        src->pub.next_input_byte += (size_t) nbytes;
        src->pub.bytes_in_buffer -= (size_t) nbytes;
    }
}

void term_source (j_decompress_ptr cinfo)
{
}

void jpeg_memory_src (j_decompress_ptr cinfo, const JOCTET * buffer, size_t bufsize)
{
    jpeg_custom_source_mgr* src;

    if (cinfo->src == NULL)
    {
        cinfo->src = (struct jpeg_source_mgr *) (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
        sizeof(jpeg_custom_source_mgr));
    }

    src = (jpeg_custom_source_mgr*) cinfo->src;
    src->pub.init_source = init_source;
    src->pub.fill_input_buffer = fill_input_buffer;
    src->pub.skip_input_data = skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart;    // default
    src->pub.term_source = term_source;

    src->pub.next_input_byte = buffer;
    src->pub.bytes_in_buffer = bufsize;
}
#endif

JpegCreator::JpegCreator()
{
}

QTransform JpegCreator::orientationMatrix(int exivOrientation) const
{
    //Check (e.g.) man jpegexiforient for an explanation
    switch (exivOrientation) {
    case 2:
        return QTransform(-1, 0, 0, 1, 0, 0);
    case 3:
        return QTransform(-1, 0, 0, -1, 0, 0);
    case 4:
        return QTransform(1, 0, 0, -1, 0, 0);
    case 5:
        return QTransform(0, 1, 1, 0, 0, 0);
    case 6:
        return QTransform(0, 1, -1, 0, 0, 0);
    case 7:
        return QTransform(0, -1, -1, 0, 0, 0);
    case 8:
        return QTransform(0, -1, 1, 0, 0, 0);
    case 1:
    default:
        return QTransform(1, 0, 0, 1, 0, 0);
    }
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
bool JpegCreator::create(const QString &path, int width, int height, QImage &image)
{
    QImage img;
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
#ifdef Q_CC_MSVC
    QFile inFile(path);
    QByteArray buf;
    if(inFile.open(QIODevice::ReadOnly)) {
        while(!inFile.atEnd()) {
            buf += inFile.readLine();
        }
        inFile.close();
    }

    jpeg_memory_src(&jpegDecompress, (JOCTET*)buf.data(), buf.size());
#else
    jpeg_stdio_src(&jpegDecompress, jpegFile);
#endif
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
    } else {
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
        for (int i = 0; i < jpegDecompress.output_height; ++i) {
            uchar *in = img.scanLine(i) + jpegDecompress.output_width * 3;
            QRgb *out = (QRgb*)img.scanLine(i);
            for (int j = jpegDecompress.output_width - 1; j >= 0; --j) {
                in -= 3;
                out[j] = qRgb(in[0], in[1], in[2]);
            }
        }
        fclose(jpegFile);
        jpeg_destroy_decompress(&jpegDecompress);
    }

#ifdef HAVE_EXIV2
    //Handle exif rotation
//    try {
        Exiv2::Image::AutoPtr exivImg = Exiv2::ImageFactory::open(name.constData());
        if (exivImg.get()) {
            exivImg->readMetadata();
            Exiv2::ExifData exifData = exivImg->exifData();
            if (!exifData.empty()) {
                Exiv2::ExifKey key("Exif.Image.Orientation");
                Exiv2::ExifData::iterator it = exifData.findKey(key);
                if (it != exifData.end()) {
                    int orient = it->toLong();
                    image = img.transformed(orientationMatrix(orient));
                    return true;
                }
            }
        }
//    } catch (...) {
        // Apparently libexiv changed its API at some point, a different exception is thrown
        // depending on the version. an ifdef could make it work, but since we just ignore the exception
        // there is no point in doing that
//    }
#endif

    image = img;
    return true;
}

ThumbCreator::Flags JpegCreator::flags() const
{
    return None;
}
