// $Id$

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <iostream.h>

#include <qvaluelist.h>
#include <qfileinfo.h>

#include <kshred.h>
#include <kdebug.h>
#include <kurl.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kprotocolmanager.h>
#include <kinstance.h>
#include <qfile.h>
#include <limits.h>


#include "kio_help.h"


using namespace KIO;


void addList(QStringList &dest, const QStringList &source)
{
  QStringList::ConstIterator it;
  for (it = source.begin(); it != source.end(); ++it)
    dest.append(*it);
}


QString HelpProtocol::langLookup(QString fname)
{
  QStringList search;

  // assemble the local search paths
  QStringList const &localDoc = KGlobal::dirs()->findDirs("html", "");

  // try to strip the doc path prefix
  bool found=false;
  for (unsigned int id=0; id<localDoc.count(); id++)
    {
      if (fname.find(localDoc[id]) == 0)
	{
	  kdDebug() << "Cutting off " << localDoc[id] << endl;
	  fname = fname.mid(localDoc[id].length());
	  found = true;
	  break;
	}
    }

  // cut of the language
  if (found)
    {
      int pos = fname.find('/');
      if (pos > 0)
	fname = fname.mid(pos+1);
    }

  // look up the different languages
  for (int id=localDoc.count()-1; id >= 0; --id)
    {      
      QStringList langs = KGlobal::locale()->languageList();
      langs.append("default");
      langs.append("en");
      QStringList::ConstIterator lang;
      for (lang = langs.begin(); lang != langs.end(); ++lang)
	search.append(QString("%1/%2/%3").arg(localDoc[id]).arg(*lang).arg(fname));
    }

  // try to locate the file
  QStringList::Iterator it;
  for (it = search.begin(); it != search.end(); ++it)
    {      
      kdDebug() << "Looking for help in: " << *it << endl;

      QFileInfo info(*it);
      if (info.exists() && info.isFile() && info.isReadable())
	return *it;
    }

  return QString::null;
}


QString HelpProtocol::lookupFile(QString fname, QString query)
{
  kdDebug() << "lookupFile: filename=" << fname << " query=" << query << endl;

  QString anchor, path, result;

  // if we have a query, look if it contains an anchor
  //  if (!query.isEmpty())
    //    if (query.left(8) == "?anchor=")
  //      anchor = query.mid(9);

  path = fname;

  kdDebug() << "lookupFile: path=" << path << " anchor=" << anchor << endl;

  result = langLookup(path);
  if (result.isEmpty())
    {
      result = langLookup(path+"/index.html");
      if (!result.isEmpty())
	redirection(KURL(QString("help:%1/index.html").arg(path)));
      else
	{
	  result = langLookup("khelpcenter/main.html");
	  redirection(KURL("help:/khelpcenter/main.html")); // FIXME! Use the right anchor, or the right page
	}
    }

  return result;
}



HelpProtocol::HelpProtocol( const QCString &pool, const QCString &app ) 
  : SlaveBase( "help", pool, app )
{
}


void HelpProtocol::get( const QString& path, const QString& query, bool /* reload */)
{
  kdDebug() << "get: path=" << path << " query=" << query << endl;

  QString doc = lookupFile(path, query);
  if (doc.isEmpty())
    {
      error( KIO::ERR_DOES_NOT_EXIST, path );
      return;
    }

    struct stat buff;
    if ( ::stat( doc, &buff ) == -1 ) {
        if ( errno == EACCES )
           error( KIO::ERR_ACCESS_DENIED, doc );
        else
           error( KIO::ERR_DOES_NOT_EXIST, doc );
	return;
    }

    if ( S_ISDIR( buff.st_mode ) ) {
	error( KIO::ERR_IS_DIRECTORY, doc );
	return;
    }

    FILE *f = fopen( doc, "rb" );
    if ( f == 0L ) {
	error( KIO::ERR_CANNOT_OPEN_FOR_READING, doc );
	return;
    }

    totalSize( buff.st_size );
    int processed_size = 0;
    time_t t_start = time( 0L );
    time_t t_last = t_start;

    char buffer[ 4090 ];
    QByteArray array;

    mimeType("text/html");

    while( !feof( f ) )
	{
	    int n = fread( buffer, 1, 2048, f );
            if (n == -1)
            {
               error( KIO::ERR_COULD_NOT_READ, doc);
               fclose(f);
               return;
            }

	    array.setRawData(buffer, n);
	    data( array );
            array.resetRawData(buffer, n);

	    processed_size += n;
	    time_t t = time( 0L );
	    if ( t - t_last >= 1 )
		{
		    processedSize( processed_size );
		    speed( processed_size / ( t - t_start ) );
		    t_last = t;
		}
	}

    data( QByteArray() );

    fclose( f );

    processedSize( buff.st_size );
    time_t t = time( 0L );
    if ( t - t_start >= 1 )
	speed( processed_size / ( t - t_start ) );

    finished();
}


void HelpProtocol::mimetype( const QString& , const QString& )
{
  mimeType("text/html");
  finished();
}


extern "C" 
{
  int kdemain( int argc, char **argv )
  {
    KInstance instance( "kio_help" );
    
    kdDebug(7101) << "Starting " << getpid() << endl;
    
    if (argc != 4)
      {
	fprintf(stderr, "Usage: kio_help protocol domain-socket1 domain-socket2\n");
	exit(-1);
      }
    
    HelpProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();
    
    kdDebug(7101) << "Done" << endl;
    return 0;
  }
}


