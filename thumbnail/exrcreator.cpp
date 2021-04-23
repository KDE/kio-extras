/*  This file is part of the KDE libraries
    Copyright (C) 2004 Brad Hards <bradh@frogmouth.net>

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

#include "exrcreator.h"
#include "thumbnail-exr-logsettings.h"

#include <QImage>
#include <QFile>

#include <ImfHeader.h>
#include <ImfInputFile.h>
#include <ImfPreviewImage.h>

#include <ksharedconfig.h>
#include <kconfiggroup.h>

#include <limits>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new EXRCreator;
    }
}

bool EXRCreator::create(const QString &path, int, int, QImage &img)
{
    Imf::InputFile in ( QFile::encodeName( path ) );
    const Imf::Header &h = in.header();

    if ( h.hasPreviewImage() ) {
	qCDebug(KIO_THUMBNAIL_EXR_LOG) << "EXRcreator - using preview";
	const Imf::PreviewImage &preview = in.header().previewImage();
	QImage qpreview(preview.width(), preview.height(), QImage::Format_RGB32);
	for ( unsigned int y=0; y < preview.height(); y++ ) {
	    for ( unsigned int x=0; x < preview.width(); x++ ) {
		const Imf::PreviewRgba &q = preview.pixels()[x+(y*preview.width())];
		qpreview.setPixel( x, y, qRgba(q.r, q.g, q.b, q.a) );
	    }
	}
	img = qpreview;
	return true;
    } else {
        // do it the hard way
	// We ignore maximum size when just extracting the thumbnail
	// from the header, but it is very expensive to render large
	// EXR images just to turn it into an icon, so we go back
	// to honoring it in here.
	qCDebug(KIO_THUMBNAIL_EXR_LOG) << "EXRcreator - using original image";
	KSharedConfig::Ptr config = KSharedConfig::openConfig();
	KConfigGroup configGroup( config, "PreviewSettings" );
	const qint64 maxSize = configGroup.readEntry( "MaximumSize", std::numeric_limits<qint64>::max() );
	const qint64 fileSize = QFile( path ).size();
	if ( (fileSize > 0) && (fileSize < maxSize) ) {
	    if (!img.load( path )) {
		return false;
	    }
	    if (img.depth() != 32)
		img = img.convertToFormat( QImage::Format_RGB32 );
	    return true;
	} else {
	    return false;
	}
    }
}
