// $Id$

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream.h>

#include <qcstring.h>
#include <qglobal.h>
#include <qvaluelist.h>

#include <kurl.h>
#include <kshred.h>
#include <kprotocolmanager.h>
#include <kdebug.h>
#include <ksock.h>
#include <kinstance.h>

#include "nntp.h"

using namespace KIO;

int main(int , char **)
{
  KInstance instance( "kio_nntp" );
  QString user="molnarc";
  QString pass="whocares";
  QString host="nebsllc.com";
  int ip=119;

}

NNTPProtocol::~NNTPProtocol()
	{
	int x=0;
	}

NNTPProtocol::NNTPProtocol (Connection *_conn ): SlaveBase ("nntp",_conn)
	{
	kDebugInfo (7106, "Constructor");
	currentHost=QString::null;
	currentIP=QString::null;
	currentUser=QString::null;
	currentPass=QString::null;
	}


void NNTPProtocol::openConnection(const QString& host, int ip, const QString& user, const QString& pass)
	{
	kDebugInfo( 7106, "in open connection method");
	currentHost=host;
	currentIP=ip;
	currentUser=user;
	currentPass=pass;
	connected();
	}

void NNTPProtocol::closeConnection()
	{
	kDebugInfo( 7106, "in close connection method");
	currentHost=QString::null;
	currentIP=QString::null;
	currentUser=QString::null;
	currentPass=QString::null;
	}

void NNTPProtocol::special(const QByteArray &)
	{
	kDebugInfo( 7106, "in special method");
	//currentHost=host;
	//currentIP=ip;
	//currentUser=user;
	//currentPass=pass;
	}

