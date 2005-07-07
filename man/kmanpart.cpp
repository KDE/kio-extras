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
    the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kmanpart.h"
#include <qstring.h>

#include <kinstance.h>
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

KInstance* KManPartFactory::s_instance = 0L;
KAboutData* KManPartFactory::s_about = 0L;

KManPartFactory::KManPartFactory( QObject* parent, const char* name )
   : KParts::Factory( parent, name )
{}

KManPartFactory::~KManPartFactory()
{
   delete s_instance;
   s_instance = 0L;
   delete s_about;
}

KParts::Part* KManPartFactory::createPartObject( QWidget * parentWidget, const char* /*widgetName*/, QObject *,
                                 const char* name, const char* /*className*/,const QStringList & )
{
   KManPart* part = new KManPart(parentWidget, name );
   return part;
}

KInstance* KManPartFactory::instance()
{
   if( !s_instance )
   {
      s_about = new KAboutData( "kmanpart",
                                I18N_NOOP( "KMan" ), KDE_VERSION_STRING );
      s_instance = new KInstance( s_about );
   }
   return s_instance;
}


KManPart::KManPart( QWidget * parent, const char * name )
: KHTMLPart( parent, name )
,m_job(0)
{
   KInstance * instance = new KInstance( "kmanpart" );
   setInstance( instance );
   m_extension=new KParts::BrowserExtension(this);
}

bool KManPart::openURL( const KURL &url )
{
   return KParts::ReadOnlyPart::openURL(url);
}

bool KManPart::openFile()
{
   if (m_job!=0)
      m_job->kill();

   begin();

   KURL url;
   url.setProtocol( "man" );
   url.setPath( m_file );

   m_job = KIO::get( url, true, false );
   connect( m_job, SIGNAL( data( KIO::Job *, const QByteArray &) ), SLOT( readData( KIO::Job *, const QByteArray &) ) );
   connect( m_job, SIGNAL( result( KIO::Job * ) ), SLOT( jobDone( KIO::Job * ) ) );
   return true;
}

void KManPart::readData(KIO::Job * , const QByteArray & data)
{
   write(data,data.size());
}

void KManPart::jobDone( KIO::Job *)
{
   m_job=0;
   end();
}

#include "kmanpart.moc"

