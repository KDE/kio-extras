#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>


#include <qtextstream.h>


#include <kdebug.h>
#include <kinstance.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kprocess.h>
#include <klocale.h>


#include "kio_man.h"
#include "kio_man.moc"


using namespace KIO;


bool parseUrl(QString url, QString &title, int &section)
{
  section = 0; // 0 means we have to search :)
  
  title = url;

  int pos = url.find('(');
  if (pos < 0)
    return true;
    
  title = title.left(pos);

  QString s = url.mid(pos+1);
  s = s.left(s.length()-1);

  bool ok;  
  section = s.toInt(&ok);

  return true;
}


MANProtocol::MANProtocol(const QCString &pool_socket, const QCString &app_socket)
  : QObject(), SlaveBase("man", pool_socket, app_socket)
{
}


MANProtocol::~MANProtocol()
{
}


void MANProtocol::get(const QString& path, const QString& query, bool /*reload*/)
{
  kdDebug(7107) << "GET " << path << endl;

  QString title;
  int section;

  if (!parseUrl(path, title, section))
    {
      error(KIO::ERR_MALFORMED_URL, path);
      return;
    }


  // tell we are getting the file
  gettingFile(path);
  mimeType("text/html");


  // assemble shell command
  QString cmd, exec;
  exec = KGlobal::dirs()->findExe("man");
  if (exec.isEmpty())
    {
      outputError(i18n("man command not found!"));
      return;
    }
  cmd = exec;

  if (query.isEmpty())
    {
      cmd += " " + title;
      if (section > 0)
	cmd += QString(" %1").arg(section);
    }
  else
    cmd += " -k " + query;

  cmd += " | ";
  exec = KGlobal::dirs()->findExe("perl");
  if (exec.isEmpty())
    {
      outputError(i18n("perl command not found!"));
      return;
    }
  cmd += " " + exec;
  exec = KGlobal::dirs()->findExe("man2html");
  if (exec.isEmpty())
    {
      outputError(i18n("man2html command not found!"));
      return;
    }
  cmd += " " + exec + " -cgiurl 'man:${title}(${section})' -compress -bare ";
  if (!query.isEmpty())
    cmd += " -k";
  
  // create shell process
  KProcess *shell = new KProcess;

  exec = KGlobal::dirs()->findExe("sh");
  if (exec.isEmpty())
    {
      outputError(i18n("sh command not found!"));
      return;
    }

  *shell << exec << "-c" << cmd;

  connect(shell, SIGNAL(receivedStdout(KProcess *,char *,int)), this, SLOT(shellStdout(KProcess *,char *,int)));


  // run shell command
  _shellStdout.truncate(0);
  shell->start(KProcess::Block, KProcess::Stdout);


  // publish the output
  QCString header, footer;
  header = "<html><body>";
  footer = "</body></html>";

  data(header);
  data(_shellStdout);
  data(footer);

  // tell we are done
  data(QByteArray());
  finished();

  // clean up
  delete shell;
  _shellStdout.truncate(0);
}


void MANProtocol::shellStdout(KProcess */*proc*/, char *buffer, int buflen)
{
  _shellStdout += QCString(buffer).left(buflen);
}


void MANProtocol::outputError(QString errmsg)
{
  QByteArray output;
  
  QTextStream os(output, IO_WriteOnly);
  
  os << "<html>" << endl;
  os << "<head><title>Man output</title></head>" << endl;
  os << "<body><h1>KDE Man Viewer Error</h1>" << errmsg << "</body>" << endl;
  os << "</html>" << endl;
  
  data(output);
  finished();
}


void MANProtocol::stat( const QString & path )
{  
  kdDebug(7107) << "ENTERING STAT " << path;

  QString title;
  int section;

  if (!parseUrl(path, title, section))
    {
      error(KIO::ERR_MALFORMED_URL, path);
      return;
    }

  kdDebug(7107) << "URL " << path << " parsed to title='" << title << "' section=" << section << endl;


  UDSEntry entry;
  UDSAtom atom;

  int pos;
  atom.m_uds = UDS_NAME;
  atom.m_long = 0;
  atom.m_str = title;
  entry.append(atom);

  atom.m_uds = UDS_FILE_TYPE;
  atom.m_str = "";
  atom.m_long = S_IFREG;
  entry.append(atom);
    
  atom.m_uds = UDS_URL;
  atom.m_long = 0;
  QString url = "man:"+title;
  if (section != 0)
    url += QString("(%1)").arg(section);
  atom.m_str = url;
  entry.append(atom);

  atom.m_uds = UDS_MIME_TYPE;
  atom.m_long = 0;
  atom.m_str = "text/html";
  entry.append(atom);

  statEntry(entry);

  finished();
}


extern "C" 
{

  int kdemain( int argc, char **argv )
  {
    KInstance instance("kio_man");
    
    kdDebug(7107) <<  "STARTING " << getpid() << endl;
  
    if (argc != 4)
      {
	fprintf(stderr, "Usage: kio_man protocol domain-socket1 domain-socket2\n");
	exit(-1);
      }
 
    MANProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();
 
    kdDebug(7107) << "Done" << endl;

    return 0;
  }

}


void MANProtocol::mimetype(const QString & /*path*/)
{
  mimeType("text/html");
  finished();
}
