
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


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include <qtextstream.h>
#include <qdict.h>
#include <qcstring.h>
#include <qfile.h>

#include <kdebug.h>
#include <kinstance.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kurl.h>
#include <kregexp.h>

#include "kio_finger.h"

using namespace KIO;

static const QString defaultRefreshRate = "60";

extern "C"
{
  int kdemain( int argc, char **argv )
  {
    KInstance instance( "kio_finger" );
    
    kdDebug(7101) << "*** Starting kio_finger " << getpid() << endl;
    
    if (argc != 4)
      {
	fprintf(stderr, "Usage: kio_finger protocol domain-socket1 domain-socket2\n");
	exit(-1);
      }
    
    FingerProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();
    
    kdDebug(7101) << "*** kio_finger Done" << endl;
    return 0;
  }
} 

   
/* ---------------------------------------------------------------------------------- */


FingerProtocol::FingerProtocol(const QCString &pool_socket, const QCString &app_socket)
  : QObject(), SlaveBase("finger", pool_socket, app_socket)
{
  myStdStream = new QString();
  getProgramPath();
}


/* ---------------------------------------------------------------------------------- */


FingerProtocol::~FingerProtocol()
{
  kdDebug() << "FingerProtocol::~FingerProtocol()" << endl;
  delete myURL;
  delete myPerlPath;
  delete myFingerPath;
  delete myStdStream;
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::get(const KURL& url )
{
  kdDebug() << "kio_finger::get(const KURL& url): " << url.url() << endl;
  
  this->parseCommandLine(url);

  kdDebug() << "kio_finger: "<< myURL->prettyURL() << endl;

  // Reset the stream
  *myStdStream="";
  
  QString query = myURL->query();
  QString refreshRate = defaultRefreshRate;

  kdDebug() << "query: " << query << endl;
  
  // Check the validity of the query 

  QRegExp regExp("?refreshRate=[0-9][0-9]*", true, true);
  if (query.contains(regExp)) {
    kdDebug() << "looks like a valid query" << endl;
    KRegExp regExp( "([0-9]+)" );
    regExp.match(query.local8Bit());
    refreshRate = regExp.group(0);
  }
  
  kdDebug() << "Refresh rate: " << refreshRate << endl;
 
  myKProcess = new KShellProcess();  
  *myKProcess << *myPerlPath << *myFingerScript << *myFingerPath
	      << refreshRate << myURL->host() << myURL->user() ;
  	
  connect(myKProcess, SIGNAL(receivedStdout(KProcess *, char *, int)), 
	  this, SLOT(slotGetStdOutput(KProcess *, char *, int)));
  //connect(myKProcess, SIGNAL(receivedStderr(KProcess *, char *, int)), 
  //	  this, SLOT(slotGetStdOutput(KProcess *, char *, int)));
  
  myKProcess->start(KProcess::Block, KProcess::All);

  data(QCString(myStdStream->local8Bit()));
 
  data(QByteArray());
  finished();  

  //clean up
  
  delete myKProcess;
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::slotGetStdOutput(KProcess* /* p */, char *stdout, int len) {
  //kdDebug() <<  "void FingerProtocol::slotGetStdoutOutput()" << endl;		
  *myStdStream += QString::fromLocal8Bit(stdout, len);
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::mimetype(const KURL & /*url*/)
{
  mimeType("text/html");
  finished();
}
     

/* ---------------------------------------------------------------------------------- */


void FingerProtocol::getProgramPath()
{
  //kdDebug() << "kfingerMainWindow::getProgramPath()" << endl;
  
  myPerlPath = new QString();
  *myPerlPath = QString(KGlobal::dirs()->findExe("perl"));
 
  // Not to sure wether I'm using the right error number here. - schlpbch -   
  if (myPerlPath->isEmpty())
    {
      kdDebug() << "Perl command not found" << endl; 	
      this->error(ERR_CANNOT_LAUNCH_PROCESS,
		  i18n("Could not find the Perl program on your system, please install.")); 
      exit(-1);
    } 
  else 
    {
      kdDebug() << "Perl command found:" << *myPerlPath << endl; 
    }

  myFingerPath = new QString();  
  *myFingerPath = QString(KGlobal::dirs()->findExe("finger"));
  
  /*
   * I decided not to provide my finger implementation:
   *  1. It's very unlikely that it's not installed.
   *  2. If it's not installed, then there might be reasons for it (security).
   */

  if ((myFingerPath->isEmpty()))
    {   
      kdDebug() << "Finger command not found" << endl;
      this->error(ERR_CANNOT_LAUNCH_PROCESS, 
		  i18n("Could not find the Finger program on your system, please install."));
      exit(-1);

      /*
	kdDebug() << "Finger command not found, trying KDEfinger instead" << endl;
	*myFingerPath = QString(KGlobal::dirs()->findExe("KDEfinger"));   
	if (myFingerPath->isEmpty())
	{
	kdDebug() << "KDEFinger command not found" << endl;
	this->error(ERR_CANNOT_LAUNCH_PROCESS, 
	i18n("Neither finger nor KDEfinger command found"));
	exit(-1);
      
	} 
	else
	{
	kdDebug() << "KDEfinger command found:" << *myPerlPath << endl; 
	}
      */
    }
  else
    {
      kdDebug() << "Finger command found:" << *myFingerPath << endl; 
    }
  
  myFingerScript = new QString(locate("data","kio_finger/kio_finger.pl"));
  if (myFingerScript->isEmpty())
    {
      kdDebug() << "kio_finger perl script not found" << endl;     
      this->error(ERR_CANNOT_LAUNCH_PROCESS,
		  i18n("kio_finger perl script not found."));
      exit(-1);
    } else {
      kdDebug() << "kio_finger perl script found: " << *myFingerScript << endl;  
    }
}


/* --------------------------------------------------------------------------- */


void FingerProtocol::parseCommandLine(const KURL& url) {
  myURL = new KURL(url); 
  
  /*
   * Generate a valid finger url
   */

  if(myURL->isEmpty() ||
     myURL->isMalformed() || 
     (myURL->user().isEmpty() && myURL->host().isEmpty())) 
    {
      myURL->setProtocol("finger");
      myURL->setUser("");
      myURL->setHost("localhost");
    }
  
  /*
   * If no specifc port is specified, set it to 79.
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


#include "kio_finger.moc"
