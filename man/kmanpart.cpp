/*  This file is part of the KDE project
    Copyright (C) 2002 Alexander Neundorf <neundorf@kde.org>

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

#include "kmanpart.h"

#include <kcomponentdata.h>
#include <kpluginfactory.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kaboutdata.h>
#include <kdeversion.h>

static KAboutData createAboutData()
{
   return KAboutData("kmanpart", "kio_man", ki18n("KMan"), KDE_VERSION_STRING);
}

K_PLUGIN_FACTORY(KManPartFactory, registerPlugin<KManPart>();)
K_EXPORT_PLUGIN(KManPartFactory(createAboutData()))


KManPart::KManPart(QWidget * parentWidget, QObject* parent, const QVariantList&)
: KHTMLPart(parentWidget, parent)
,m_job(0)
{
   setComponentData(KManPartFactory::componentData());
   m_extension = new KParts::BrowserExtension(this);
}

bool KManPart::openUrl( const KUrl &url )
{
   return KParts::ReadOnlyPart::openUrl(url);
}

bool KManPart::openFile()
{
   if (m_job!=0)
      m_job->kill();

   begin();

   KUrl url;
   url.setProtocol( "man" );
   url.setPath( localFilePath() );

   m_job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
   connect( m_job, SIGNAL( data( KIO::Job *, const QByteArray &) ), SLOT( readData( KIO::Job *, const QByteArray &) ) );
   connect( m_job, SIGNAL( result(KJob*) ), SLOT( jobDone(KJob*) ) );
   return true;
}

void KManPart::readData(KIO::Job * , const QByteArray & data)
{
   write(data,data.size());
}

void KManPart::jobDone( KJob *)
{
   m_job=0;
   end();
}

#include "kmanpart.moc"

