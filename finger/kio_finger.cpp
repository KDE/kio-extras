
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


#include "kio_finger.h"

using namespace KIO;


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

  // Emit the header
  data(QCString(*myHTMLHeader));
  
  myKProcess = new KShellProcess();  
  *myKProcess << *myPerlPath << *myFingerScript << *myFingerPath << myURL->host() << myURL->user();
  	
  connect(myKProcess, SIGNAL(receivedStdout(KProcess *, char *, int)), 
	  this, SLOT(slotGetStdOutput(KProcess *, char *, int)));
  connect(myKProcess, SIGNAL(receivedStderr(KProcess *, char *, int)), 
	  this, SLOT(slotGetStdOutput(KProcess *, char *, int)));
  
  myKProcess->start(KProcess::Block, KProcess::All);

  data(QCString(*myStdStream));
  data(QCString(*myHTMLTail));
 
  data(QByteArray());
  finished();  

  //clean up
  /*
  delete myPerlPath;
  delete myFingerPath;
  delete myHTMLHeader;
  delete myHTMLTail;
  delete myKProcess;
  delete myURL;
  */
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::redirection(const KURL& url){
  kdDebug() << "Redirection to: "  << url.url() << endl;
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::slotGetStdOutput(KProcess* /* p */, char *stdout, int len) {
  kdDebug() <<  "void FingerProtocol::slotGetStdoutOutput()" << endl;		
  *myStdStream += QString::fromLocal8Bit(stdout, len);
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::listDir(const KURL& url)
{
  error(KIO::ERR_CANNOT_ENTER_DIRECTORY,url.url());
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::mimetype(const KURL & /*url*/)
{
  mimeType("text/html");
  finished();
}


/* ------------------------------------------------------------------------------------- */


void FingerProtocol::getProgramPath()
{
  //kdDebug() << "kfingerMainWindow::getProgramPath()" << endl;
  myPerlPath = new QString();

  // Not to sure wether I'm using the right error number here. - schlpbch -  

  *myPerlPath = QString(KGlobal::dirs()->findExe("perl"));
  if (myPerlPath->isEmpty())
    {
      this->error(ERR_CANNOT_LAUNCH_PROCESS, i18n("Perl command not found"));
      kdDebug() << "Perl command not found" << endl; 	
      exit(-1);
    } else {
      kdDebug() << "Perl command found:" << *myPerlPath << endl; 
    }

  *myFingerPath = QString(KGlobal::dirs()->findExe("finger"));
  if (myFingerPath->isEmpty())
    {
      this->error(ERR_CANNOT_LAUNCH_PROCESS, i18n("finger command not found"));
      kdDebug() << "Finger command not found" << endl; 	
      exit(-1);
    } else {
      kdDebug() << "Finger command found:" << *myFingerPath << endl; 
    }
  
  myFingerScript = new QString(locate("data","kfinger/kio_finger/finger.pl"));
  if (myFingerScript->isEmpty())
    {
      this->error(ERR_CANNOT_LAUNCH_PROCESS, i18n("Default finger script not found"));
      kdDebug() << "Default finger script not found" << endl; 
      exit(-1);
    } else {
      kdDebug() << "Default finger script found: " << *myFingerScript << endl;  
    }
  
  QFile headFile(locate("data","kfinger/kio_finger/fingerHead"));
  myHTMLHeader = new QString();
  if( !headFile.open( IO_ReadOnly ) ) 
    {
      this->warning(i18n("Couldn't read file: ") + headFile.name());
      kdDebug() << "Couldn't read file: " << headFile.name() << endl;
      *myHTMLHeader="<HTML><BODY><PRE>";
    }
  else 
    {
      QTextStream headStream(&headFile);
      while( !headStream.eof() ) {
	myHTMLHeader->append(headStream.readLine());
      }
      headFile.close();
      kdDebug() << "Read file:" << headFile.name() << endl;
    }
  
  QFile tailFile(locate("data","kfinger/kio_finger/fingerTail"));
  myHTMLTail = new QString() ;
  if( !tailFile.open( IO_ReadOnly ) ) 
    {
      this->warning(i18n("Couldn't read file: ") + tailFile.name());   
      kdDebug() << "Couldn't read file: " << tailFile.name() <<  endl;
      *myHTMLTail = "</PRE></BODY></HTML>";
    }
  else
    {
      QTextStream tailStream(&tailFile);
      while( !tailStream.eof() ) {
	myHTMLTail->append(tailStream.readLine());
      }
      tailFile.close();
      kdDebug() << "Read file:" << tailFile.name() << endl;
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
}


#include "kio_finger.moc"
