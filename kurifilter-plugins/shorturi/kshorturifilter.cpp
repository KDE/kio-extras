/*  -*- c-basic-offset: 2 -*-

    kshorturifilter.h

    This file is part of the KDE project
    Copyright (C) 2000 Dawit Alemayehu <adawit@kde.org>
    Copyright (C) 2000 Malte Starostik <starosti@zedat.fu-berlin.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "kshorturifilter.h"

#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>

#include <QDir>
#include <QRegExp>
#include <QList>

#include <QtDBus/QtDBus>

#include <kdebug.h>
#include <kprotocolinfo.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kauthorized.h>
#include <kmimetype.h>

#define FQDN_PATTERN    "(?:[a-zA-Z0-9][a-zA-Z0-9+-]*\\.[a-zA-Z]+)"
#define IPv4_PATTERN    "[0-9]{1,3}\\.[0-9]{1,3}(?:\\.[0-9]{0,3})?(?:\\.[0-9]{0,3})?"
#define IPv6_PATTERN    "^\\[.*\\]"
#define ENV_VAR_PATTERN "\\$[a-zA-Z_][a-zA-Z0-9_]*"

#define QL1S(x) QLatin1String(x)

 /**
  * IMPORTANT:
  *  If you change anything here, please run the regression test
  *  ../tests/kurifiltertest.
  *
  *  If you add anything here, make sure to add a corresponding
  *  test code to ../tests/kurifiltertest.
  */

typedef QMap<QString,QString> EntryMap;

#if 0
static QString stringDetails( const QString& str )
{
  if ( str.isNull() )
    return QL1S( "<null>" );
  else if ( str.isEmpty() )
    return QL1S( "<empty>" );
  else
    return str;
}
#endif

static bool isValidShortURL( const QString& cmd )
{
  // Examples of valid short URLs:
  // "kde.org", "foo.bar:8080", "user@foo.bar:3128"
  // "192.168.1.0", "127.0.0.1:3128"
  // "[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]"
  QRegExp exp;

  // Match FQDN_PATTERN
  exp.setPattern( QL1S(FQDN_PATTERN) );
  if ( cmd.contains( exp ) )
  {
    kDebug(7023) << cmd << " matches FQDN_PATTERN" << endl;
#if 0
    // something like wallpaper.png also matches a the FQDN pattern
    // but is very unlikely to be meant as such
    if (KMimeType::findByPath(cmd) != KMimeType::defaultMimeTypePtr())
        return false;
#endif

    return true;
  }

  // Match IPv4 addresses
  exp.setPattern( QL1S(IPv4_PATTERN) );
  if ( cmd.contains( exp ) )
  {
    kDebug(7023) << cmd << " matches IPv4_PATTERN" << endl;
    return true;
  }

  // Match IPv6 addresses
  exp.setPattern( QL1S(IPv6_PATTERN) );
  if ( cmd.contains( exp ) )
  {
    kDebug(7023) << cmd << " matches IPv6_PATTERN" << endl;
    return true;
  }

  kDebug(7023) << cmd << "' is not a short URL." << endl;
  return false;
}

static QString removeArgs( const QString& _cmd )
{
  QString cmd( _cmd );

  if( cmd[0] != '\'' && cmd[0] != '"' )
  {
    // Remove command-line options (look for first non-escaped space)
    int spacePos = 0;

    do
    {
      spacePos = cmd.indexOf( ' ', spacePos+1 );
    } while ( spacePos > 1 && cmd[spacePos - 1] == '\\' );

    if( spacePos > 0 )
    {
      cmd = cmd.left( spacePos );
      //kDebug(7023) << "spacePos=" << spacePos << " returning " << cmd;
    }
  }

  return cmd;
}

KShortUriFilter::KShortUriFilter( QObject *parent, const QVariantList & /*args*/ )
                :KUriFilterPlugin( "kshorturifilter", parent )
{
    QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KUriFilterPlugin",
                                "configure", this, SLOT(configure()));
    configure();
}

bool KShortUriFilter::filterUri( KUriFilterData& data ) const
{
 /*
  * Here is a description of how the shortURI deals with the supplied
  * data.  First it expands any environment variable settings and then
  * deals with special shortURI cases. These special cases are the "smb:"
  * URL scheme which is very specific to KDE, "#" and "##" which are
  * shortcuts for man:/ and info:/ protocols respectively. It then handles
  * local files.  Then it checks to see if the URL is valid and one that is
  * supported by KDE's IO system.  If all the above checks fails, it simply
  * lookups the URL in the user-defined list and returns without filtering
  * if it is not found. TODO: the user-defined table is currently only manually
  * hackable and is missing a config dialog.
  */

  KUrl url = data.uri();
  QString cmd = data.typedString();
  const bool isMalformed = !url.isValid();
  const QString protocol = url.protocol();
  //kDebug(7023) << "url=" << url << " cmd=" << cmd << " isMalformed=" << isMalformed;

  if (!isMalformed &&
      (protocol.length() == 4) &&
      (protocol != QLatin1String("http")) &&
      (protocol[0]=='h') &&
      (protocol[1]==protocol[2]) &&
      (protocol[3]=='p'))
  {
     // Handle "encrypted" URLs like: h++p://www.kde.org
     url.setProtocol( QLatin1String("http"));
     setFilteredUri( data, url);
     setUriType( data, KUriFilterData::NetProtocol );
     return true;
  }

  // TODO: Make this a bit more intelligent for Minicli! There
  // is no need to make comparisons if the supplied data is a local
  // executable and only the argument part, if any, changed! (Dawit)
  // You mean caching the last filtering, to try and reuse it, to save stat()s? (David)

  const QString starthere_proto = QL1S("start-here:");
  if (cmd.indexOf(starthere_proto) == 0 )
  {
    setFilteredUri( data, KUrl("system:/") );
    setUriType( data, KUriFilterData::LocalDir );
    return true;
  }

  // Handle MAN & INFO pages shortcuts...
  const QString man_proto = QL1S("man:");
  const QString info_proto = QL1S("info:");
  if( cmd[0] == '#' ||
      cmd.indexOf( man_proto ) == 0 ||
      cmd.indexOf( info_proto ) == 0 )
  {
    if( cmd.left(2) == QL1S("##") )
      cmd = QL1S("info:/") + cmd.mid(2);
    else if ( cmd[0] == '#' )
      cmd = QL1S("man:/") + cmd.mid(1);

    else if ((cmd==info_proto) || (cmd==man_proto))
      cmd+='/';

    setFilteredUri( data, KUrl( cmd ));
    setUriType( data, KUriFilterData::Help );
    return true;
  }

  // Detect UNC style (aka windows SMB) URLs
  if ( cmd.startsWith( QLatin1String( "\\\\") ) )
  {
    // make sure path is unix style
    cmd.replace('\\', '/');
    cmd.prepend( QLatin1String( "smb:" ) );
    setFilteredUri( data, KUrl( cmd ));
    setUriType( data, KUriFilterData::NetProtocol );
    return true;
  }

  bool expanded = false;

  // Expanding shortcut to HOME URL...
  QString path;
  QString ref;
  QString query;
  QString nameFilter;

  if (KUrl::isRelativeUrl(cmd) && QDir::isRelativePath(cmd)) {
     path = cmd;
     //kDebug(7023) << "path=cmd=" << path;
  } else {
    if (url.isLocalFile())
    {
      //kDebug(7023) << "hasRef=" << url.hasRef();
      // Split path from ref/query
      // but not for "/tmp/a#b", if "a#b" is an existing file,
      // or for "/tmp/a?b" (#58990)
      if( ( url.hasRef() || !url.query().isEmpty() )
           && !url.path().endsWith(QL1S("/")) ) // /tmp/?foo is a namefilter, not a query
      {
        path = url.path();
        ref = url.ref();
        //kDebug(7023) << "isLocalFile set path to " << stringDetails( path );
        //kDebug(7023) << "isLocalFile set ref to " << stringDetails( ref );
        query = url.query();
        if (path.isEmpty() && url.hasHost())
          path = '/';
      }
      else
      {
        path = cmd;
        //kDebug(7023) << "(2) path=cmd=" << path;
      }
    }
  }

  if( path[0] == '~' )
  {
    int slashPos = path.indexOf('/');
    if( slashPos == -1 )
      slashPos = path.length();
    if( slashPos == 1 )   // ~/
    {
      path.replace ( 0, 1, QDir::homePath() );
    }
    else // ~username/
    {
      QString user = path.mid( 1, slashPos-1 );
      struct passwd *dir = getpwnam(user.toLocal8Bit().data());
      if( dir && strlen(dir->pw_dir) )
      {
        path.replace (0, slashPos, QString::fromLocal8Bit(dir->pw_dir));
      }
      else
      {
        QString msg = dir ? i18n("<qt><b>%1</b> does not have a home folder.</qt>", user) :
                            i18n("<qt>There is no user called <b>%1</b>.</qt>", user);
        setErrorMsg( data, msg );
        setUriType( data, KUriFilterData::Error );
        // Always return true for error conditions so
        // that other filters will not be invoked !!
        return true;
      }
    }
    expanded = true;
  }
  else if ( path[0] == '$' ) {
    // Environment variable expansion.
    QRegExp r (QL1S(ENV_VAR_PATTERN));
    if ( r.indexIn( path ) == 0 )
    {
      const char* exp = getenv( path.mid( 1, r.matchedLength() - 1 ).toLocal8Bit().data() );
      if(exp)
      {
        path.replace( 0, r.matchedLength(), QString::fromLocal8Bit(exp) );
        expanded = true;
      }
    }
  }

  if ( expanded || cmd.startsWith( '/' ) )
  {
    // Look for #ref again, after $ and ~ expansion (testcase: $QTDIR/doc/html/functions.html#s)
    // Can't use KUrl here, setPath would escape it...
    const int pos = path.indexOf('#');
    if ( pos > -1 )
    {
      const QString newPath = path.left( pos );
      if ( QFile::exists( newPath ) )
      {
        ref = path.mid( pos + 1 );
        path = newPath;
        //kDebug(7023) << "Extracted ref: path=" << path << " ref=" << ref;
      }
    }
  }


  bool isLocalFullPath = (!path.isEmpty() && path[0] == '/');

  // Checking for local resource match...
  // Determine if "uri" is an absolute path to a local resource  OR
  // A local resource with a supplied absolute path in KUriFilterData
  const QString abs_path = data.absolutePath();

  const bool canBeAbsolute = (protocol.isEmpty() && !abs_path.isEmpty());
  const bool canBeLocalAbsolute = (canBeAbsolute && abs_path[0] =='/');
  bool exists = false;

  //kDebug(7023) << "abs_path=" << abs_path
  //         << " protocol=" << protocol
  //         << " canBeAbsolute=" << canBeAbsolute
  //         << " canBeLocalAbsolute=" << canBeLocalAbsolute
  //         << " isLocalFullPath=" << isLocalFullPath
  //         << endl;

  struct stat buff;
  if ( canBeLocalAbsolute )
  {
    QString abs = QDir::cleanPath( abs_path );
    // combine absolute path (abs_path) and relative path (cmd) into abs_path
    int len = path.length();
    if( (len==1 && path[0]=='.') || (len==2 && path[0]=='.' && path[1]=='.') )
        path += '/';
    //kDebug(7023) << "adding " << abs << " and " << path;
    abs = QDir::cleanPath(abs + '/' + path);
    //kDebug(7023) << "checking whether " << abs << " exists.";
    // Check if it exists
    if( stat( QFile::encodeName(abs).data(), &buff ) == 0 )
    {
      path = abs; // yes -> store as the new cmd
      exists = true;
      isLocalFullPath = true;
    }
  }

  if( isLocalFullPath && !exists )
  {
    exists = ( stat( QFile::encodeName(path).data() , &buff ) == 0 );

    if ( !exists ) {
      // Support for name filter (/foo/*.txt), see also KonqMainWindow::detectNameFilter
      // If the app using this filter doesn't support it, well, it'll simply error out itself
      int lastSlash = path.lastIndexOf( '/' );
      if ( lastSlash > -1 && path.indexOf( ' ', lastSlash ) == -1 ) // no space after last slash, otherwise it's more likely command-line arguments
      {
        QString fileName = path.mid( lastSlash + 1 );
        QString testPath = path.left( lastSlash + 1 );
        if ( ( fileName.indexOf( '*' ) != -1 || fileName.indexOf( '[' ) != -1 || fileName.indexOf( '?' ) != -1 )
           && stat( QFile::encodeName(testPath).data(), &buff ) == 0 )
        {
          nameFilter = fileName;
          //kDebug(7023) << "Setting nameFilter to " << nameFilter;
          path = testPath;
          exists = true;
        }
      }
    }
  }

  //kDebug(7023) << "path =" << path << " isLocalFullPath=" << isLocalFullPath << " exists=" << exists;
  if( exists )
  {
    KUrl u;
    u.setPath(path);
    //kDebug(7023) << "ref=" << stringDetails(ref) << " query=" << stringDetails(query);
    u.setRef(ref);
    u.setQuery(query);

    if (!KAuthorized::authorizeUrlAction( QLatin1String("open"), KUrl(), u))
    {
      // No authorization, we pretend it's a file will get
      // an access denied error later on.
      setFilteredUri( data, u );
      setUriType( data, KUriFilterData::LocalFile );
      return true;
    }

    // Can be abs path to file or directory, or to executable with args
    bool isDir = S_ISDIR( buff.st_mode );
    if( !isDir && access ( QFile::encodeName(path).data(), X_OK) == 0 )
    {
      //kDebug(7023) << "Abs path to EXECUTABLE";
      setFilteredUri( data, u );
      setUriType( data, KUriFilterData::Executable );
      return true;
    }

    // Open "uri" as file:/xxx if it is a non-executable local resource.
    if( isDir || S_ISREG( buff.st_mode ) )
    {
      //kDebug(7023) << "Abs path as local file or directory";
      if ( !nameFilter.isEmpty() )
        u.setFileName( nameFilter );
      setFilteredUri( data, u );
      setUriType( data, ( isDir ) ? KUriFilterData::LocalDir : KUriFilterData::LocalFile );
      return true;
    }

    // Should we return LOCAL_FILE for non-regular files too?
    kDebug(7023) << "File found, but not a regular file nor dir... socket?";
  }

  // Let us deal with possible relative URLs to see
  // if it is executable under the user's $PATH variable.
  // We try hard to avoid parsing any possible command
  // line arguments or options that might have been supplied.
  QString exe = removeArgs( cmd );
  //kDebug(7023) << "findExe with " << exe;
  if( data.checkForExecutables() && !KStandardDirs::findExe( exe ).isNull() )
  {
    //kDebug(7023) << "EXECUTABLE  exe=" << exe;
    setFilteredUri( data, KUrl::fromPath( exe ));
    // check if we have command line arguments
    if( exe != cmd )
        setArguments(data, cmd.right(cmd.length() - exe.length()));
    setUriType( data, KUriFilterData::Executable );
    return true;
  }

  // Process URLs of known and supported protocols so we don't have
  // to resort to the pattern matching scheme below which can possibly
  // slow things down...
  if ( !isMalformed && !isLocalFullPath && !protocol.isEmpty() )
  {
    //kDebug(7023) << "looking for protocol " << protocol;
    if ( KProtocolInfo::protocols().contains( protocol ) )
    {
      setFilteredUri( data, url );
      if ( protocol == QL1S("man") || protocol == QL1S("help") )
        setUriType( data, KUriFilterData::Help );
      else
        setUriType( data, KUriFilterData::NetProtocol );
      return true;
    }
  }

  // Okay this is the code that allows users to supply custom matches for
  // specific URLs using Qt's regexp class. This is hard-coded for now.
  // TODO: Make configurable at some point...
  if ( !cmd.contains( ' ' ) )
  {
    QList<URLHint>::ConstIterator it;
    for( it = m_urlHints.begin(); it != m_urlHints.end(); ++it )
    {
      QRegExp match( (*it).regexp );
      if ( match.indexIn( cmd ) == 0 )
      {
        //kDebug(7023) << "match - prepending " << (*it).prepend;
        cmd.prepend( (*it).prepend );
        setFilteredUri( data, KUrl( cmd ) );
        setUriType( data, (*it).type );
        return true;
      }
    }

    // If cmd is NOT a local resource, check if it is a valid "shortURL"
    // candidate and append the default protocol the user supplied. (DA)
    if ( protocol.isEmpty() && isValidShortURL(cmd) )
    {
      kDebug(7023) << "Valid short url, from malformed url -> using default proto="
               << m_strDefaultProtocol << endl;

      cmd.insert( 0, m_strDefaultProtocol );
      setFilteredUri( data, KUrl( cmd ));
      setUriType( data, KUriFilterData::NetProtocol );
      return true;
    }
  }

  // If we previously determined that the URL might be a file,
  // and if it doesn't exist, then error
  if( isLocalFullPath && !exists )
  {
    KUrl u;
    u.setPath(path);
    u.setRef(ref);

    if (!KAuthorized::authorizeUrlAction( QLatin1String("open"), KUrl(), u))
    {
      // No authorization, we pretend it exists and will get
      // an access denied error later on.
      setFilteredUri( data, u );
      setUriType( data, KUriFilterData::LocalFile );
      return true;
    }
    //kDebug(7023) << "fileNotFound -> ERROR";
    setErrorMsg( data, i18n( "<qt>The file or folder <b>%1</b> does not exist.</qt>", data.uri().prettyUrl() ) );
    setUriType( data, KUriFilterData::Error );
    return true;
  }

  // If we reach this point, we cannot filter this thing so simply return false
  // so that other filters, if present, can take a crack at it.
  return false;
}

KCModule* KShortUriFilter::configModule( QWidget*, const char* ) const
{
    return 0; //new KShortUriOptions( parent, name );
}

QString KShortUriFilter::configName() const
{
//    return i18n("&ShortURLs"); we don't have a configModule so no need for a configName that confuses translators
    return KUriFilterPlugin::configName();
}

void KShortUriFilter::configure()
{
  KConfig config( objectName() + QL1S( "rc"), KConfig::NoGlobals );
  KConfigGroup cg( config.group("") );

  m_strDefaultProtocol = cg.readEntry( "DefaultProtocol", QString("http://") );
  const EntryMap patterns = config.entryMap( QL1S("Pattern") );
  const EntryMap protocols = config.entryMap( QL1S("Protocol") );
  KConfigGroup typeGroup(&config, "Type");

  for( EntryMap::ConstIterator it = patterns.begin(); it != patterns.end(); ++it )
  {
    QString protocol = protocols[it.key()];
    if (!protocol.isEmpty())
    {
      int type = typeGroup.readEntry(it.key(), -1);
      if (type > -1 && type <= KUriFilterData::Unknown)
        m_urlHints.append( URLHint(it.value(), protocol, static_cast<KUriFilterData::UriTypes>(type) ) );
      else
        m_urlHints.append( URLHint(it.value(), protocol) );
    }
  }
}

K_PLUGIN_FACTORY(KShortUriFilterFactory, registerPlugin<KShortUriFilter>();)
K_EXPORT_PLUGIN(KShortUriFilterFactory("kcmkurifilt"))

#include "kshorturifilter.moc"
