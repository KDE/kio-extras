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
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kaboutdata.h>
#include <kdeversion.h>

extern "C"
{
   KDE_EXPORT void* init_libkmanpart()
   {
      return new KManPartFactory;
   }
}

KComponentData *KManPartFactory::s_instance = 0L;
KAboutData* KManPartFactory::s_about = 0L;

KManPartFactory::KManPartFactory( QObject* parent )
   : KParts::Factory( parent )
{}

KManPartFactory::~KManPartFactory()
{
   delete s_instance;
   s_instance = 0L;
   delete s_about;
}

KParts::Part* KManPartFactory::createPartObject( QWidget * parentWidget, QObject *,
                                 const char* /*className*/,const QStringList & )
{
   KManPart* part = new KManPart(parentWidget);
   return part;
}

const KComponentData &KManPartFactory::componentData()
{
   if( !s_instance )
   {
      s_about = new KAboutData( "kmanpart", 0,
                                ki18n( "KMan" ), KDE_VERSION_STRING );
      s_instance = new KComponentData(s_about);
   }
   return *s_instance;
}


KManPart::KManPart( QWidget * parent )
: KHTMLPart( parent )
,m_job(0)
{
   setComponentData(KComponentData("kmanpart"));
   m_extension=new KParts::BrowserExtension(this);
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

