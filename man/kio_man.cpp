#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>


#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qdict.h>
#include <qcstring.h>


#include <kdebug.h>
#include <kinstance.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kprocess.h>
#include <klocale.h>


#include "kio_man.h"
#include "kio_man.moc"


using namespace KIO;


bool parseUrl(QString url, QString &title, QString &section)
{
  section = "";

  while (url.left(1) == "/")
    url.remove(0,1);

  title = url;

  int pos = url.find('(');
  if (pos < 0)
    return true;
    
  title = title.left(pos);

  section = url.mid(pos+1);
  section = section.left(section.length()-1);

  return true;
}


MANProtocol::MANProtocol(const QCString &pool_socket, const QCString &app_socket)
  : QObject(), SlaveBase("man", pool_socket, app_socket), _cache(0)
{
}


MANProtocol::~MANProtocol()
{
  delete _cache;
}


void MANProtocol::get(const KURL& url )
{
  kdDebug(7107) << "GET " << url.url() << endl;

  QString title, section;

  if (!parseUrl(url.path(), title, section))
    {
      error(KIO::ERR_MALFORMED_URL, url.url());
      return;
    }


  // tell we are getting the file
  gettingFile(url.url());
  mimeType("text/html");


  // see if an index was requested
  if (url.query().isEmpty() && (title.isEmpty() || title == "/"))
    {
      if (section == "index" || section.isEmpty()) 
	showMainIndex();
      else
	showIndex(section);
      return;
    }

  // assemble shell command
  QString cmd, exec;
  exec = KGlobal::dirs()->findExe("man");
  if (exec.isEmpty())
    {
      outputError(i18n("man command not found!"));
      return;
    }
  cmd = QString("LANG=%1 %2").arg(KGlobal::locale()->language()).arg(exec);

  if (url.query().isEmpty())
    {
      cmd += " " + title;
      if (section > 0)
	cmd += QString(" %1").arg(section);
    }
  else
    cmd += " -k " + url.query();

  cmd += " | ";
  exec = KGlobal::dirs()->findExe("perl");
  if (exec.isEmpty())
    {
      outputError(i18n("perl command not found!"));
      return;
    }
  cmd += " " + exec;
  exec = locate("exe", "man2html");
  if (exec.isEmpty())
    {
      outputError(i18n("man2html command not found!"));
      return;
    }
  cmd += " " + exec + " -cgiurl 'man:/${title}(${section})' -compress -bare ";
  if (!url.query().isEmpty())
    cmd += " -k";
  
  // create shell process
  KProcess *shell = new KProcess;

  exec = KGlobal::dirs()->findExe("sh");
  if (exec.isEmpty())
    {
      outputError(i18n("sh command not found!"));
      return;
    }

  kdDebug() << "Command to execute: " << cmd << endl;

  *shell << exec << "-c" << cmd;

  connect(shell, SIGNAL(receivedStdout(KProcess *,char *,int)), this, SLOT(shellStdout(KProcess *,char *,int)));


  // run shell command
  _shellStdout.truncate(0);
  shell->start(KProcess::Block, KProcess::Stdout);


  // publish the output
  QCString header, footer;
  header = "<html><body bgcolor=#ffffff>";
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


void MANProtocol::shellStdout(KProcess * /*proc*/, char *buffer, int buflen)
{
  _shellStdout += QCString(buffer).left(buflen);
}


void MANProtocol::outputError(QString errmsg)
{
  QCString output;
  
  QTextStream os(output, IO_WriteOnly);
  
  os << "<html>" << endl;
  os << i18n("<head><title>Man output</title></head>") << endl;
  os << i18n("<body bgcolor=#ffffff><h1>KDE Man Viewer Error</h1>") << errmsg << "</body>" << endl;
  os << "</html>" << endl;
  
  data(output);
  finished();
}


void MANProtocol::stat( const KURL& url)
{  
  kdDebug(7107) << "ENTERING STAT " << url.url();

  QString title, section;

  if (!parseUrl(url.path(), title, section))
    {
      error(KIO::ERR_MALFORMED_URL, url.url());
      return;
    }

  kdDebug(7107) << "URL " << url.url() << " parsed to title='" << title << "' section=" << section << endl;


  UDSEntry entry;
  UDSAtom atom;

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
  QString newUrl = "man:"+title;
  if (section != 0)
    newUrl += QString("(%1)").arg(section);
  atom.m_str = newUrl;
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


void MANProtocol::mimetype(const KURL & /*url*/)
{
  mimeType("text/html");
  finished();
}


QString sectionName(QString section)
{
  if (section == "1")
    return i18n("User Commands");
  else if (section == "2")
    return i18n("System Calls");
  else if (section == "3")
    return i18n("Subroutines");
  else if (section == "4")
    return i18n("Devices");
  else if (section == "5")
    return i18n("File Formats");
  else if (section == "6")
    return i18n("Games");
  else if (section == "7")
    return i18n("Miscellaneous");
  else if (section == "8")
    return i18n("System Administration");
  else if (section == "9")
    return i18n("Kernel");
  else if (section == "n")
    return i18n("New");

  return QString::null;
}


void MANProtocol::showMainIndex()
{
  QCString output;
  
  QTextStream os(output, IO_WriteOnly);

  // print header
  os << "<html>" << endl;
  os << i18n("<head><title>UNIX Manual Index</title></head>") << endl;
  os << i18n("<body bgcolor=#ffffff><h1>UNIX Manual Index</h1>") << endl;

  QString sectList = getenv("MANSECT");
  if (sectList.isEmpty())
    sectList = "1:2:3:4:5:6:7:8:9:n";
  QStringList sections = QStringList::split(':', sectList);

  os << "<table>" << endl;

  QStringList::ConstIterator it;
  for (it = sections.begin(); it != sections.end(); ++it)
    os << "<tr><td><a href=\"man:(" << *it << ")\">Section " << *it << "</a></td><td>&nbsp;</td><td> " << sectionName(*it) << "</td></tr>" << endl;
  
  os << "</table>" << endl;

  // print footer
  os << "</body></html>" << endl;
  
  data(output);
  finished();
}


QStringList getManPaths()
{
  QStringList manPaths;

  // TODO: GNU man understands "man -w" to give the real man path used
  // by the program. We should use this instead of all this guessing!

  // add MANPATH paths 
  QString envPath = getenv("MANPATH");
  if (!envPath.isEmpty())
    manPaths = QStringList::split(':', envPath);
  
  // add paths from /etc/man.conf
  QRegExp manpath("^MANPATH\\s");
  QFile mc("/etc/man.conf");
  if (mc.open(IO_ReadOnly))
    {
      QTextStream is(&mc);

      while (!is.eof())
	{
	  QString line = is.readLine();
	  if (manpath.match(line) == 0)
	    {
	      QString path = line.mid(8).stripWhiteSpace();
	      if (!manPaths.contains(path))
		manPaths.append(path);
	    }
	}

      mc.close();
    }

  // add default paths
  if (!manPaths.contains("/usr/man"))
    manPaths.append("/usr/man");
  if (!manPaths.contains("/usr/X11R6/man"))
    manPaths.append("/usr/X11R6/man");
  if (!manPaths.contains("/usr/local/man"))
    manPaths.append("/usr/local/man");
  if (!manPaths.contains("/usr/share/man"))
    manPaths.append("/usr/share/man");

  return manPaths;
}


void MANProtocol::initCache(QString section)
{
  delete _cache;
  _cache = new QDict<char>(231, false);
  _cache->setAutoDelete(true);
  
  // locate whatis databases
  QStringList manPaths = getManPaths();
  QStringList::ConstIterator it;
  for (it = manPaths.begin(); it != manPaths.end(); ++it)
    {
      QFile whatis(QString("%1/whatis").arg(*it));
      if (whatis.open(IO_ReadOnly))
	{
	  QTextStream is(&whatis);

	  QString line, tag, desc;
	  while (!is.eof())
	    {
	      line = is.readLine();
	      int pos1 = line.find(QString("(%1").arg(section));
	      if (pos1 <= 0)
		continue;
	      
	      int pos = line.find("-");
	      if (pos <= 0)
		continue;

	      desc = line.mid(pos + 1).stripWhiteSpace();
	      tag = line.left(pos1).stripWhiteSpace();

	      QStringList tags = QStringList::split(',', tag);
	      QStringList::ConstIterator t;
	      for (t=tags.begin(); t != tags.end(); ++t)
		_cache->insert((*t).stripWhiteSpace().latin1(), strdup(desc.local8Bit()));
	    }

	  whatis.close();
	}
    }
}


QString MANProtocol::pageName(QString page)
{
  const char *pagename = (*_cache)[page.latin1()];
  if (pagename)
    return pagename;
  return page;
}


void MANProtocol::showIndex(QString section)
{
  QCString output;
  
  QTextStream os(output, IO_WriteOnly);

  // print header
  os << "<html>" << endl;
  os << i18n("<head><title>UNIX Manual Index</title></head>") << endl;
  os << i18n("<body bgcolor=#ffffff><h1>Index for Section %1: %2</h1>").arg(section).arg(sectionName(section)) << endl;

  // compose list of search paths -------------------------------------------------------------

  QStringList manPaths = getManPaths();

  // search for the man pages
  QStringList pages;
  QStringList::ConstIterator it;
  for (it = manPaths.begin(); it != manPaths.end(); ++it)
    {
      QDir dir(*it, QString("man%1*").arg(section), 0, QDir::Dirs);

      if (!dir.exists()) 
	continue;

      QStringList dirList = dir.entryList();
      QStringList::Iterator itDir;
      for (itDir = dirList.begin(); !(*itDir).isNull(); ++itDir)
	{
	  if ( (*itDir).at(0) == '.' )
	    continue;

	  QString dirName = QString("%1/%2").arg(*it).arg(*itDir);
	  QDir fileDir(dirName, QString("*.%1*").arg(section), 0, QDir::Files | QDir::Hidden | QDir::Readable);

	  if (!fileDir.exists()) 
	    return;

	  // does dir contain files
	  if (fileDir.count() > 0)
	    {
	      QStringList fileList = fileDir.entryList();
	      QStringList::Iterator itFile;
	      for (itFile = fileList.begin(); !(*itFile).isNull(); ++itFile)
		{
		  QString fileName = *itFile;
		  QString file = dirName;
		  file += '/';
		  file += *itFile;
	
		  // skip compress extension
		  if (fileName.right(3) == ".gz")
		    {
		      fileName.truncate(fileName.length()-3);
		    }
		  else if (fileName.right(2) == ".Z")
		    {
		      fileName.truncate(fileName.length()-2);
		    }

		  // strip section
		  int pos = fileName.findRev('.');
		  if ((pos > 0) && (fileName.mid(pos).find(section) > 0))
		    fileName = fileName.left(pos);

		  if (!fileName.isEmpty() && !pages.contains(fileName))
		    pages.append(fileName);
		}
	    }
	}
    }

  // print out the list
  os << "<table>" << endl;
  pages.sort();
  initCache(section);
  QStringList::ConstIterator page;
  for (page = pages.begin(); page != pages.end(); ++page)
    {
      os << "<tr><td><a href=\"man:" << *page << "(" << section << ")\">";
      os << *page << "</a></td><td>&nbsp;</td><td> " << pageName(*page) << "</td></tr>" << endl;
    }

  os << "</table>" << endl;

  // print footer
  os << "</body></html>" << endl;
  
  data(output);
  finished();
}
