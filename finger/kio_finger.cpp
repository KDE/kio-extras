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
  delete myPerlPath;
  delete myHTMLHeader;
  delete myHTMLTail;
  delete myURL;
  delete myStdStream;
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::get(const KURL& url )
{
  kdDebug() << "kio_finger::get(const KURL& url): " << url.url() << endl;
  
  this->parseCommandLine(url);

  kdDebug() << "kio_finger: "<< myURL->prettyURL() << endl;

  //Reset the stream
  *myStdStream="";

  // Emit the header
  data(QCString(*myHTMLHeader));
  
  myKProcess = new KShellProcess();  
  *myKProcess << *myPerlPath << *myFingerScript  << myURL->host() << myURL->user();
  	
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
  delete myKProcess;
  delete myURL;
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
  
#warning Currently relies on /usr/bin/finger. Will be removed later. - schlpbch -
  *myPerlPath = QString(KGlobal::dirs()->findExe("perl"));
  if (myPerlPath->isEmpty())
    {
      kdDebug() << "Perl command not found" << endl; 	
      //myRawFingerMode=true;
    } else {
      kdDebug() << "Perl command found" << endl; 
    }
  
  myFingerScript = new QString(locate("data","kfinger/kio_finger/finger.pl"));
  if (myFingerScript->isEmpty())
    {
      kdDebug() << "Default finger script not found" << endl;    
      //myRawFingerMode=true;
    } else {
      kdDebug() << "Default finger script found" << endl;  
    }
  
  QFile headFile(locate("data","kfinger/kio_finger/fingerHead"));
  myHTMLHeader = new QString();
  if( !headFile.open( IO_ReadOnly ) ) 
    {
    kdDebug() << "Couldn't read file:" << headFile.name() << endl;
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
    kdDebug() << "Couldn't read file:" << tailFile.name() <<  endl;
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
