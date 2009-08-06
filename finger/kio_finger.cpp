
/***************************************************************************
                          kio_finger.cpp  -  description
                             -------------------
    begin                : Sun Aug 12 2000
    copyright            : (C) 2000 by Andreas Schlapbach
    email                : schlpbch@iam.unibe.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kio_finger.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include <QByteArray>
#include <QRegExp>

#include <kdebug.h>
#include <kcomponentdata.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kurl.h>
#include <KProcess>


using namespace KIO;

static const QString defaultRefreshRate = "60";

extern "C"
{
  KDE_EXPORT int kdemain( int argc, char **argv )
  {
    KComponentData componentData( "kio_finger" );

    //kDebug() << "*** Starting kio_finger " << getpid();

    if (argc != 4)
      {
	fprintf(stderr, "Usage: kio_finger protocol domain-socket1 domain-socket2\n");
	exit(-1);
      }

    FingerProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    //kDebug() << "*** kio_finger Done";
    return 0;
  }
}


/* ---------------------------------------------------------------------------------- */


FingerProtocol::FingerProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
  : QObject(), SlaveBase("finger", pool_socket, app_socket)
{
  getProgramPath();
}


/* ---------------------------------------------------------------------------------- */


FingerProtocol::~FingerProtocol()
{
  //kDebug() << "FingerProtocol::~FingerProtocol()";
  delete myURL;
  delete myPerlPath;
  delete myFingerPath;
  delete myFingerPerlScript;
  delete myFingerCSSFile;
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::get(const KUrl& url )
{
  //kDebug() << "kio_finger::get(const KUrl& url)";

  this->parseCommandLine(url);

  //kDebug() << "myURL: " << myURL->prettyUrl();

  QString query = myURL->query();
  QString refreshRate = defaultRefreshRate;

  //kDebug() << "query: " << query;

  // Check the validity of the query

  QRegExp regExp("?refreshRate=[0-9][0-9]*", Qt::CaseSensitive, QRegExp::Wildcard);
  if (query.contains(regExp)) {
    //kDebug() << "looks like a valid query";
    QRegExp regExp( "([0-9]+)" );
    regExp.indexIn(query);
    refreshRate = regExp.cap(0);
  }

  //kDebug() << "Refresh rate: " << refreshRate;

  KProcess proc;
  proc << *myPerlPath << *myFingerPerlScript
       << *myFingerPath << *myFingerCSSFile
       << refreshRate << myURL->host() << myURL->user();

  proc.setOutputChannelMode(KProcess::MergedChannels);
  proc.execute();
  data(proc.readAllStandardOutput());
  data(QByteArray());
  finished();
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::mimetype(const KUrl & /*url*/)
{
  mimeType("text/html");
  finished();
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::getProgramPath()
{
  //kDebug() << "kfingerMainWindow::getProgramPath()";
  // Not to sure whether I'm using the right error number here. - schlpbch -

  myPerlPath = new QString(KGlobal::dirs()->findExe("perl"));
  if (myPerlPath->isEmpty())
    {
      //kDebug() << "Perl command not found";
      this->error(ERR_CANNOT_LAUNCH_PROCESS,
		  i18n("Could not find the Perl program on your system, please install."));
      exit();
    }
  else
    {
      //kDebug() << "Perl command found:" << *myPerlPath;
    }

  myFingerPath = new QString(KGlobal::dirs()->findExe("finger"));
  if ((myFingerPath->isEmpty()))
    {
      //kDebug() << "Finger command not found";
      this->error(ERR_CANNOT_LAUNCH_PROCESS,
		  i18n("Could not find the Finger program on your system, please install."));
      exit();
    }
  else
    {
      //kDebug() << "Finger command found:" << *myFingerPath;
    }

  myFingerPerlScript = new QString(KStandardDirs::locate("data","kio_finger/kio_finger.pl"));
  if (myFingerPerlScript->isEmpty())
    {
      //kDebug() << "kio_finger.pl script not found";
      this->error(ERR_CANNOT_LAUNCH_PROCESS,
		  i18n("kio_finger Perl script not found."));
      exit();
    }
  else
    {
      //kDebug() << "kio_finger perl script found: " << *myFingerPerlScript;
    }

  myFingerCSSFile = new QString(KStandardDirs::locate("data","kio_finger/kio_finger.css"));
  if (myFingerCSSFile->isEmpty())
    {
      //kDebug() << "kio_finger.css file not found";
      this->warning(i18n("kio_finger CSS script not found. Output will look ugly."));
    }
  else
    {
      //kDebug() << "kio_finger CSS file found: " << *myFingerCSSFile;
    }
}


/* --------------------------------------------------------------------------- */


void FingerProtocol::parseCommandLine(const KUrl& url)
{
  myURL = new KUrl(url);

  /*
   * Generate a valid finger url
   */

  if(myURL->isEmpty() || !myURL->isValid() ||
     (myURL->user().isEmpty() && myURL->host().isEmpty()))
    {
      myURL->setProtocol("finger");
      myURL->setUser("");
      myURL->setHost("localhost");
    }

  /*
   * If no specific port is specified, set it to 79.
   */

  if(myURL->port() == 0) {
    myURL->setPort(79);
  }

  /*
   * If no refresh rate is given, set it to defaultRefreshRate
   */

  if (myURL->query().isEmpty()) {
    myURL->setQuery("?refreshRate="+defaultRefreshRate);
  }
}

/* ---------------------------------------------------------------------------------- */
#include "kio_finger.moc"
/* ---------------------------------------------------------------------------------- */

