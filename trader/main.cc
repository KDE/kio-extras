#include "main.h"
#include <kio_base.h>
#include <kio_manager.h>

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <assert.h>

#include <iostream>

#include <k2url.h>
#include <kapp.h>
#include <klocale.h>
#include <kregexp.h>
#include <k2config.h>

#include <qstrlist.h>

K2Config* g_pConfig = 0L;
K2Config* g_pOfferPrefs = 0L;
K2Config* g_pServiceTypePrefs = 0L;

/**
 * @return 6 on error
 */
int findOfferPreference( const char *_service_type, const char *_name )
{
  QString name = _service_type;
  name += "/";
  name += _name;
  
  int i;
  if ( !g_pOfferPrefs->readLong( name, i ) )
    return 6;
  
  return i;
}

K2Config* findServiceTypePropertyPreferences( const char *_service_type, const char *_property )
{
  K2Config *c = g_pServiceTypePrefs->findGroup( _service_type );
  if ( !c )
    return 0L;
  c = c->findGroup( _property );
  return c;
}


CORBA::ORB_var orb = 0L;
CORBA::BOA_var boa = 0L;
CosTradingRepos::ServiceTypeRepository_var g_vRepo = 0L;
CosTrading::Lookup_var g_vLookup = 0L;

void testDir2( const char *_name )
{
    DIR *dp;
    QString c = kapp->localkdedir().data();
    c += _name;
    dp = opendir( c.data() );
    if ( dp == NULL )
	::mkdir( c.data(), S_IRWXU );
    else
	closedir( dp );
}

void sig_handler( int );
void sig_handler2( int );
void main( int argc, char **argv )
{
  cerr << "Starting" << endl;

  testDir2( "" );
  testDir2( "/share" );    
  testDir2( "/share/apps" );
  testDir2( "/share/apps/kio_trader" );
  
  orb = CORBA::ORB_init( argc, argv, "mico-local-orb" );

  cerr << "ORB Done" << endl;
  
  boa = orb->BOA_init (argc, argv, "mico-local-boa");

  cerr << "Started" << endl;

  CORBA::Object_var trobj = orb->resolve_initial_references( "TradingService" );
  assert( !CORBA::is_nil( trobj ) );
  
  cerr << "Got Initref" << endl;

  g_vLookup = CosTrading::Lookup::_narrow( trobj );
  assert( !CORBA::is_nil( g_vLookup ) );
  
  CORBA::Object_var repobj = g_vLookup->type_repos();
  assert( !CORBA::is_nil( repobj ) );

  g_vRepo = CosTradingRepos::ServiceTypeRepository::_narrow( repobj );
  assert( !CORBA::is_nil( g_vRepo ) );

  cerr << "Got trader ..." << endl;

  KApplication app( argc, argv );
  
  signal(SIGCHLD,sig_handler);
  signal(SIGSEGV,sig_handler2);

  /**
   * Read the config file
   */
  QString configfile = kapp->localkdedir();
  configfile += "/share/apps/kio_trader/preferences.kfg";
  
  struct stat buff;
  if ( stat( configfile, &buff ) == -1 )
    g_pConfig = new K2Config;
  else
    g_pConfig = new K2Config( configfile );

  g_pOfferPrefs = g_pConfig->findGroup( "OfferPreferences" );
  if ( !g_pOfferPrefs )
    g_pOfferPrefs = g_pConfig->insertGroup( "OfferPreferences", "OfferPreferences" );
  g_pServiceTypePrefs = g_pConfig->findGroup( "ServiceTypePreferences" );
  if ( !g_pServiceTypePrefs )
    g_pServiceTypePrefs = g_pConfig->insertGroup( "ServiceTypePreferences", "ServiceTypePreferences" );

  /**
   * Do your job and loop forever
   */
  ProtocolManager manager;

  Connection parent( 0, 1 );
  
  TraderProtocol file( &parent );
  file.dispatchLoop();

  cerr << "Done" << endl;
}

void sig_handler2( int )
{
  cerr << "###############SEG FILE#############" << endl;
  exit(1);
}

void sig_handler( int )
{
  int pid;
  int status;
    
  while( 1 )
  {
    pid = waitpid( -1, &status, WNOHANG );
    if ( pid <= 0 )
    {
      // Reinstall signal handler, since Linux resets to default after
      // the signal occured ( BSD handles it different, but it should do
      // no harm ).
      signal( SIGCHLD, sig_handler );
      return;
    }
  }
}

TraderProtocol::TraderProtocol( Connection *_conn ) : IOProtocol( _conn )
{
  TraderInfo info;
  
  if ( !CORBA::is_nil( g_vRepo ) )
  {    
    info.m_vLookup = g_vLookup;
    
    CosTradingRepos::ServiceTypeRepository::ServiceTypeNameSeq_var stseq;
    CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes types;
    types._d( CosTradingRepos::ServiceTypeRepository::all );
    stseq = g_vRepo->list_types( types );
    for ( int i = 0; i < stseq->length(); i++ )
    {    
      cerr << "#" << stseq[i] << endl;
      string name = stseq[i].in();
      
      CosTradingRepos::ServiceTypeRepository::TypeStruct_var t;
      t = g_vRepo->describe_type( stseq[i] );
      
      info.m_mapTypes[ name ] = t;
    }
  }
  
  m_mapTraders[ i18n("Local") ] = info;
}

TraderInfo* TraderProtocol::findTrader( const char *_trader )
{
  map<string,TraderInfo>::iterator it = m_mapTraders.find( _trader );
  if ( it == m_mapTraders.end() )
    return 0L;
  
  return &(it->second);
}

ServiceType* TraderInfo::findServiceType( const char *_type )
{
  map<string,ServiceType>::iterator it = m_mapTypes.find( _type );
  if ( it == m_mapTypes.end() )
    return 0L;
  
  return &(it->second);
}

bool TraderInfo::findServiceTypeByPrefix( const char *_prefix, list<string>& _lst )
{
  string pattern = _prefix;
  if ( !pattern.empty() )
    pattern += "/";

  KRegExp r( "([^/]+)$" );
  
  map<string,ServiceType>::iterator it = m_mapTypes.begin();
  for( ; it != m_mapTypes.end(); ++it )
  {
    if ( strncmp( pattern.c_str(), it->first.c_str(), pattern.length() ) == 0 )
      if ( r.match( it->first.c_str() + pattern.length() ) )
      _lst.push_back( r.group(1) );
  }

  return !( _lst.empty() );
}

void TraderInfo::findServiceTypePostfixes( const char *_prefix, list<string>& _lst )
{
  string pattern = _prefix;
  if ( !pattern.empty() )
    pattern += "/";

  KRegExp r( "([^/]+)" );

  map<string,ServiceType>::iterator it = m_mapTypes.begin();
  for( ; it != m_mapTypes.end(); ++it )
  {
    if ( strncmp( pattern.c_str(), it->first.c_str(), pattern.length() ) == 0 )
      if ( r.match( it->first.c_str() + pattern.length() ) )
      _lst.push_back( r.group(1) );
  }
}

OfferInfo* TraderInfo::findOffer( const char *_type, const char *_name, OfferInfo& _offer )
{
  CosTrading::ServiceTypeName_var stype;
  CosTrading::Constraint_var constr;
  CosTrading::Lookup::Preference_var prefs;
  CosTrading::Lookup::SpecifiedProps desired;
  desired._d( CosTrading::Lookup::all );
  CosTrading::PolicySeq policyseq;
  policyseq.length( 0 );
  prefs = CORBA::string_dup( "max 1" );
  string tmp = "Name = ";
  tmp += _name;
  constr = CORBA::string_dup( tmp.c_str() );
  stype = CORBA::string_dup( _type );
  CosTrading::OfferSeq_var offers = 0L;
  CosTrading::OfferIterator_var offer_itr = 0L;
  CosTrading::PolicyNameSeq_var limits = 0L;
  m_vLookup->query( stype, constr, prefs, policyseq, desired, 1000, offers, offer_itr, limits );

  if ( offers->length() == 0 )
    return 0L;
  
  _offer = offers[0];
  return &_offer;
}

void TraderProtocol::slotGet( const char *_url )
{
  cerr << "GETTING stuff" << endl;

  string url = _url;
  
  K2URL usrc( _url );
  if ( usrc.isMalformed() )
  {
    error( ERR_MALFORMED_URL, url.c_str() );
    finished();
    return;
  }

  if ( strcmp( usrc.protocol(), "trader" ) != 0 )
  {
    error( ERR_INTERNAL, "kio_trader got non trader URL as source in get command" );
    finished();
    return;
  }

  string html;

  if ( strcmp( usrc.path(), "/cgi/setprefs" ) == 0 )
  {
    setPreferences( usrc.query(), html );
  }
  else
  {    
    string path = usrc.path( 0 );
    string type;
    string trader;
    string offer;
    
    if ( splitPath( path.c_str(), trader, type, offer ) )
    {
      TraderInfo *info = findTrader( trader.c_str() );
      if ( info )
      {
	ServiceType *st = info->findServiceType( type.c_str() );
	if ( st )
	{
	  if ( offer == i18n("Information" ) )
	    describeServiceType( trader.c_str(), type.c_str(), info, *st, html );
	  else if ( offer == i18n("Preferences" ) )
	    preferences( trader.c_str(), type.c_str(), info, *st, html );
	  else
	  {
	    OfferInfo offerinf;
	    if( info->findOffer( type.c_str(), offer.c_str(), offerinf ) )
	      describeOffer( trader.c_str(), type.c_str(), offer.c_str(), info, offerinf, html );
	  }
	}
      }
    }
  }
  
  if ( html.empty() )
  {
    error( ERR_DOES_NOT_EXIST, url.c_str() );
    finished();
    return;
  }
  
  ready();

  gettingFile( url.c_str() );
  
  int len = html.size();
  totalSize( len );  
  int processed_size = 0;
  
  time_t t_start = time( 0L );
  time_t t_last = t_start;
  
  const char *p = html.c_str();
  while( processed_size < len )
  {
    int n = 2048;
    if ( processed_size + n > len )
      n = len - processed_size;
    data( (void*)(p + processed_size), n );

    processed_size += n;
    time_t t = time( 0L );
    if ( t - t_last >= 1 )
    {
      processedSize( processed_size );
      speed( processed_size / ( t - t_start ) );
      t_last = t;
    }
  }

  dataEnd();
  
  processedSize( len );
  time_t t = time( 0L );
  if ( t - t_start >= 1 )
    speed( processed_size / ( t - t_start ) );

  finished();
}

void TraderProtocol::setPreferences( const char *_query, string& _html )
{
}

void TraderProtocol::describeOffer( const char *_trader, const char *_type, const char *_name, TraderInfo *_info,
				    OfferInfo& _offer, string& _html )
{
  _html = "<HTML><HEAD><TITLE>Service Type ";
  _html += _name;
  _html += "</TITLE></HEAD><BODY background=\"file:";
  _html += kapp->kde_wallpaperdir().data();
  _html += "/omg.jpg\"><h1>";
  _html += _name;
  _html += "</h1><ul><li>Service Type<br>&nbsp;&nbsp;&nbsp;";
  _html += "<a href=\"trader:/";
  _html += _trader;
  _html += "/";
  _html += _type;
  _html += "/";
  _html += i18n("Information");
  _html += "\">";
  _html += _type;
  _html += "</a></li>";

  if ( _offer.properties.length() > 0 )
  {
    _html += "<li>Properties<br><ul>";
    
    for( int k = 0; k < _offer.properties.length(); ++k )
    {
      CosTrading::StringList strlst;
      string val;
      char *s;
      if ( _offer.properties[k].value >>= s )
      {  
	val = s;
	CORBA::string_free( s );
      
	if ( !val.empty() )
        {
	  if ( strncmp( val.c_str(), "/* XPM */", 9 ) == 0 )
          {    
	  }
	  else if ( strncmp( val.c_str(), "/* HREF */", 10 ) == 0 )
	  {
	    _html += "<li>";
	    _html += _offer.properties[k].name;
	    _html += "<br>&nbsp;&nbsp;&nbsp;";
	    _html += "<a href=\"";
	    _html += val.c_str() + 10;
	    _html += "\">";
	    _html += val.c_str() + 10;
	    _html += "</a>";
	    _html += "</li>";
	  }
	  else if ( strncmp( val.c_str(), "/* IMG */", 9 ) == 0 )
	  {
	    _html += "<li>";
	    _html += _offer.properties[k].name;
	    _html += "<br>&nbsp;&nbsp;&nbsp;";
	    _html += "<img src=\"";
	    _html += val.c_str() + 9;
	    _html += "\">";
	    _html += "</li>";
	  }
	  else
	  {
	    _html += "<li>";
	    _html += _offer.properties[k].name;
	    _html += "<br>&nbsp;&nbsp;&nbsp;";
	    _html += val;
	    _html += "</li>";
	  }
	}
      }
      else if ( _offer.properties[k].value >>= strlst )
      {
	_html += "<li>";
	_html += _offer.properties[k].name;
	_html += "<br><ul>";
	
	for( int k = 0; k < strlst.length(); k++ )
	{
	  val = strlst[ k ].in();
	  if ( val.empty() )
	  {
	  }
	  else if ( strncmp( val.c_str(), "/* XPM */", 9 ) == 0 )
          {    
	  }
	  else if ( strncmp( val.c_str(), "/* HREF */", 10 ) == 0 )
	  {
	    _html += "<li>";
	    _html += "<a href=\"";
	    _html += val.c_str() + 10;
	    _html += "\">";
	    _html += val.c_str() + 10;
	    _html += "</a>";
	    _html += "</li>";
	  }
	  else if ( strncmp( val.c_str(), "/* IMG */", 9 ) == 0 )
	  {
	    _html += "<li>";
	    _html += "<img src=\"";
	    _html += val.c_str() + 9;
	    _html += "\">";
	    _html += "</li>";
	  }
	  else
	  {
	    _html += "<li>";
	    _html += val;
	    _html += "</li>";
	  }
	}

	_html += "</ul></li>";
      }
      else
      {
	_html += "<li>";
	_html += _offer.properties[k].name;
	_html += " ( Unknow type ";
	_html += "</li>";
      }
    }
    
    _html += "</ul></li>";
  }
  
  _html += "</ul></body></html>";
}

void TraderProtocol::describeServiceType( const char *_trader, const char *_name, TraderInfo *_info,
					  ServiceType& _type, string& _html )
{
  QString lang = kapp->getLocale()->language();
  
  _html = "<HTML><HEAD><TITLE>Service Type ";
  _html += _name;
  _html += "</TITLE></HEAD><BODY background=\"file:";
  _html += kapp->kde_wallpaperdir().data();
  _html += "/omg.jpg\"><h1>";
  _html += _name;
  _html += "</h1><ul><li>Interface<br>&nbsp;&nbsp;&nbsp;";
  _html += _type.if_name;
  _html += "</li>";
  
  if ( _type.super_types.length() > 0 )
  {
    _html += "<li>Super Types<br><ul>";
    for( int k = 0; k < _type.super_types.length(); k++ )
    {
      _html += "<li><a href=\"trader:/";
      _html += _trader;
      _html += "/";
      _html += _type.super_types[k];
      _html += "/";
      _html += i18n("Information");
      _html += "\">";
      _html += _type.super_types[k];
      _html += "</a></li>";
    }
    _html += "</ul></li>";
  }

  list<string> subs;
  map<string,ServiceType>::iterator it = _info->m_mapTypes.begin();
  for( ; it != _info->m_mapTypes.end(); ++it )
  {
    for( int k = 0; k < it->second.super_types.length(); k++ )
    {
      if ( strcmp( it->second.super_types[k].in(), _name ) == 0 )
	subs.push_back( it->first );
    }
  }

  if ( !subs.empty() )
  {   
    _html += "<li>Sub Types<br><ul>";

    list<string>::iterator it2 = subs.begin();
    for( ; it2 != subs.end(); ++it2 )
    {
      _html += "<li><a href=\"trader:/";
      _html += _trader;
      _html += "/";
      _html += *it2;
      _html += "/";
      _html += i18n("Information");
      _html += "\">";
      _html += *it2;
      _html += "</a></li>";
    }
    _html += "</ul></li>";
  }
  
  char buffer[ 100 ];
  sprintf( buffer, "%i:%i", _type.incarnation.high, _type.incarnation.low );
  _html += "<li>Incarnation Number<br>&nbsp;&nbsp;&nbsp;";
  _html += buffer;
  _html += "</li>";
  
  if ( _type.props.length() > 0 )
  {
    _html += "<li>Scheme<br><ul>";
    for( int j = 0; j < _type.props.length(); j++ )    
    { 
      string prop = _type.props[j].name.in();
      _html += "<li>";
      _html += prop;
      _html += "</li>";
    }
    _html += "</ul></li>";    
  }

  if ( _type.masked )
  {    
    _html += "<li>Is masked</li>";
  }
  else
  {    
    _html += "<li>Not masked</li>";
  }

  if ( _type.values.length() > 0 )
  {    
    _html += "<li>Properties<br><ul>";

    for( int y = 0; y < _type.values.length(); y++ )    
    {           
      string val;
      char *s;
      if ( _type.values[y].value >>= s )
      {  
	val = s;
	CORBA::string_free( s );
      }
      
      if ( !val.empty() )
      {
	if ( strncmp( val.c_str(), "/* XPM */", 9 ) == 0 )
	{    
	}
	else if ( strncmp( val.c_str(), "/* HREF */", 10 ) == 0 )
	{
	  _html += "<li>";
	  _html += _type.values[y].name;
	  _html += "<br>&nbsp;&nbsp;&nbsp;";
	  _html += "<a href=\"";
	  _html += val.c_str() + 10;
	  _html += "\">";
	  _html += val.c_str() + 10;
	  _html += "</a>";
	  _html += "</li>";
	}
	else if ( strncmp( val.c_str(), "/* IMG */", 9 ) == 0 )
	{
	  _html += "<li>";
	  _html += _type.values[y].name;
	  _html += "<br>&nbsp;&nbsp;&nbsp;";
	  _html += "<img src=\"";
	  _html += val.c_str() + 9;
	  _html += "\">";
	  _html += "</li>";
	}
	else
	{
	  _html += "<li>";
	  _html += _type.values[y].name;
	  _html += "<br>&nbsp;&nbsp;&nbsp;";
	  _html += val;
	  _html += "</li>";
	}
      }
    }

    _html += "</ul></li>";
  }

  _html += "</ul></body></html>";
}

void TraderProtocol::preferences( const char *_trader, const char *_name, TraderInfo *_info, ServiceType& _type, string& _html )
{
  QString lang = kapp->getLocale()->language();
  
  _html = "<HTML><HEAD><TITLE>Service Type ";
  _html += _name;
  _html += " ";
  _html += i18n("Preferences");
  _html += "</TITLE></HEAD><BODY background=\"file:";
  _html += kapp->kde_wallpaperdir().data();
  _html += "/omg.jpg\"><h1>";
  _html += _name;
  _html += "</h1><form action=\"trader:/cgi/setprefs\" action=GET><ul><li>Interface<br>&nbsp;&nbsp;&nbsp;";
  _html += _type.if_name;
  _html += "</li>";

  // References to super types
  if ( _type.super_types.length() > 0 )
  {
    _html += "<li>Super Types<br><ul>";
    for( int k = 0; k < _type.super_types.length(); k++ )
    {
      _html += "<li><a href=\"trader:/";
      _html += _trader;
      _html += "/";
      _html += _type.super_types[k];
      _html += "/";
      _html += i18n("Preferences");
      _html += "\">";
      _html += _type.super_types[k];
      _html += "</a></li>";
    }
    _html += "</ul></li>";
  }

  // References to subtypes
  list<string> subs;
  map<string,ServiceType>::iterator it = _info->m_mapTypes.begin();
  for( ; it != _info->m_mapTypes.end(); ++it )
  {
    for( int k = 0; k < it->second.super_types.length(); k++ )
    {
      if ( strcmp( it->second.super_types[k].in(), _name ) == 0 )
	subs.push_back( it->first );
    }
  }

  if ( !subs.empty() )
  {   
    _html += "<li>Sub Types<br><ul>";

    list<string>::iterator it2 = subs.begin();
    for( ; it2 != subs.end(); ++it2 )
    {
      _html += "<li><a href=\"trader:/";
      _html += _trader;
      _html += "/";
      _html += *it2;
      _html += "/";
      _html += i18n("Preferences");
      _html += "\">";
      _html += *it2;
      _html += "</a></li>";
    }
    _html += "</ul></li>";
  }
  
  /**
   * Get all offers for this service type
   */
  CosTrading::ServiceTypeName_var stype;
  CosTrading::Constraint_var constr;
  CosTrading::Lookup::Preference_var prefs;
  CosTrading::Lookup::SpecifiedProps desired;
  desired._d( CosTrading::Lookup::all );
  CosTrading::PolicySeq policyseq;
  policyseq.length( 0 );
  prefs = CORBA::string_dup( "max 1" );
  constr = CORBA::string_dup( "" );
  stype = CORBA::string_dup( _name );
  CosTrading::OfferSeq_var offers = 0L;
  CosTrading::OfferIterator_var offer_itr = 0L;
  CosTrading::PolicyNameSeq_var limits = 0L;
  _info->m_vLookup->query( stype, constr, prefs, policyseq, desired, 1000, offers, offer_itr, limits );
	
  if ( offers->length() > 0 )
  {    
    _html += "<li>Offers<br><table><tr><td><i>Name</i></td><td><i>Grading</i></td></tr>";
    for( int k = 0; k < offers->length(); k++ )
    {
      QString name;
      for( int i = 0; i < offers[k].properties.length(); ++i )
	if ( strcmp( offers[k].properties[i].name.in(), "Name" ) == 0 )
        {
	  char *s;
	  if ( offers[k].properties[i].value >>= s )
	  {  
	    name = s;
	    CORBA::string_free( s );
	  }
	}

      long pref = 6;
      if ( name.isEmpty() )
	name = "(Unnamed)";
      else
	pref = findOfferPreference( _name, name );
      _html += "<tr><td>";
      _html += name;
      _html += "</td><td><select name=\"";
      _html += name;
      _html += "\">";

      char buffer[ 1024 ];
      for( int p = 1; p <= 6; p++ )
      {   
	if ( p == pref )
	  sprintf( buffer, "<option selected value=%i>%i", p, p );
	else
	  sprintf( buffer, "<option value=%i>%i", p, p );
	_html += buffer;
      }
      _html += "</select></td></tr>";
    }
    _html += "</table></li>";
  }

  // Let the user tell us his opinion about the properties
  if ( _type.props.length() > 0 )
  {
    bool show_importance = true;
    
    _html += "<li>Scheme<br><table border=1><tr><td><i>Property</i></td><td><i>Wishes</i></td><td><i>Grading</i></td></tr>";
    for( int j = 0; j < _type.props.length(); j++ )    
    { 
      QString prop = _type.props[j].name.in();

      K2Config *prefs = findServiceTypePropertyPreferences( _name, prop );
      
      _html += "<tr><td valign=top>";
      _html += prop;
      _html += "</td>";
      if ( _type.props[ j ].value_type->equal( CORBA::_tc_boolean ) )
      {
	string value ="";
	if ( prefs )
	  prefs->readString( "value", value );
	
	_html += "<td><select name=\"_property.";
	_html += prop;
	if ( value == "yes" )
	  _html += "\"><option selected value=\"yes\">Good";
	else
	  _html += "\"><option value=\"yes\">Good";
	if ( value != "no" && value != "yes" )
	  _html += "<option selected value=\"unknown\">Dont Know";
	else
	  _html += "<option value=\"unknown\">Dont Know";
	if ( value == "no" )
	  _html += "<option selected value=\"no\">Bad</select></td>";
	else
	  _html += "<option value=\"no\">Bad</select></td>";
      }
      else if ( _type.props[ j ].value_type->equal( CORBA::_tc_float ) )
      {
	string value ="";
	if ( prefs )
	  prefs->readString( "value", value );

	_html += "<td><select name=\"_property.";
	_html += prop;
	if ( value == "max" )
	  _html += "\"><option selected value=\"max\">Maximize";
	else
	  _html += "\"><option value=\"max\">Maximize";
	if ( value != "min" && value != "max" )
	  _html += "<option selected value=\"unknown\">Dont Know";
	else
	  _html += "<option value=\"unknown\">Dont Know";
	if ( value == "min" )
	  _html += "<option selected value=\"min\">Minimize</select></td>";
	else
	  _html += "<option value=\"min\">Minimize</select></td>";
      }
      else if ( _type.props[ j ].value_type->equal( CORBA::_tc_long ) )
      {
	string value ="";
	if ( prefs )
	  prefs->readString( "value", value );

	_html += "<td><select name=\"_property.";
	_html += prop;
	if ( value == "max" )
	  _html += "\"><option selected value=\"max\">Maximize";
	else
	  _html += "\"><option value=\"max\">Maximize";
	if ( value != "min" && value != "max" )
	  _html += "<option selected value=\"unknown\">Dont Know";
	else
	  _html += "<option value=\"unknown\">Dont Know";
	if ( value == "min" )
	  _html += "<option selected value=\"min\">Minimize</select></td>";
	else
	  _html += "<option value=\"min\">Minimize</select></td>";
      }
      else if ( _type.props[ j ].value_type->equal( CORBA::_tc_string ) )
      {
	list<string> values;
	if ( prefs )
	  prefs->readStringList( "value", values );
	QStrList lst;
	list<string>::iterator it2 = values.begin();
	for( ; it2 != values.end(); ++it2 )
	{
	  cerr << "Appending '" << it2->c_str() << "'" << endl;
	  lst.append( it2->c_str() );
	}
	
	_html += "<td>";
	bool init = true;
	for( int k = 0; k < offers->length(); k++ )
	{
	  string name;
	  for( int i = 0; i < offers[k].properties.length(); ++i )
	    if ( strcmp( offers[k].properties[i].name.in(), prop.data() ) == 0 )
	    {
	      char *s;
	      if ( offers[k].properties[i].value >>= s )
	      {  
		if ( strncmp( s, "/* XPM */", 9 ) != 0 && strncmp( s, "/* IMG */", 9 ) != 0 &&  strncmp( s, "/* HREF */", 10 ) != 0 )
		{
		  if ( init )
		  {
		    init = false;
		    _html += "<select multiple size=3 name=\"_property.";
		    _html += prop;
		    _html += "\">";
		  }
		  cerr << "Comparing with '" << s << "'" << endl;
		  if ( lst.find( s ) != -1 )
		  {
		    cerr << "Found" << endl;
		    _html += "<option selected value=\"";
		  }
		  else
		  {
		    _html += "<option value=\"";
		    cerr << "!Found" << endl;
		  }
		  
		  _html += s;
		  _html += "\">";
		  _html += s;
		}
		CORBA::string_free( s );
	      }
	    }
	}
	if ( !init )
	  _html += "</select>";
	else
	  _html += "&nbsp;";
	_html += "</td>";
      } 
      else
      {
	show_importance = false;
	_html += "<td>&nbsp;</td>";
      }
      
      if ( show_importance )
      {
	int imp = 6;
	char buffer[ 1024 ];
	if ( prefs )
	  prefs->readLong( "importance", imp );

	_html += "<td><select name=\"_importance.";
	_html += prop;
	_html += "\">";
	
	for( int p = 1; p <= 6; p++ )
        {   
	  if ( p == imp )
	    sprintf( buffer, "<option selected value=%i>%i", p, p );
	  else
	    sprintf( buffer, "<option value=%i>%i", p, p );
	  _html += buffer;
	}
	
	_html += "</select></td>";
      }
      else
	_html += "<td>&nbsp;</td>";

      _html += "</tr>";
    }
    _html += "</table>";    
  }

  _html += "</ul><br><center><input type=submit value=\"Change Preferences\"></center></form></body></html>";
}

void TraderProtocol::slotListDir( const char *_url )
{
  string url = _url;
  
  K2URL usrc( _url );
  if ( usrc.isMalformed() )
  {
    error( ERR_MALFORMED_URL, url.c_str() );
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  if ( strcmp( usrc.protocol(), "trader" ) != 0 )
  {
    error( ERR_INTERNAL, "kio_trader got non trader URL as source in get command" );
    finished();
    return;
  }
  
  string path = usrc.path( 0 );
  
  if ( path == "/" )
  {
    map<string,TraderInfo>::iterator it = m_mapTraders.begin();
    for( ; it != m_mapTraders.end(); ++it )
    {
      UDSEntry entry;
      UDSAtom atom;
      atom.m_uds = UDS_NAME;
      atom.m_str = it->first;
      entry.push_back( atom );
      atom.m_uds = UDS_FILE_TYPE;
      atom.m_long = S_IFDIR;
      entry.push_back( atom );

      listEntry( entry );
    }
  
    finished();      
    return;
  }
  
  KRegExp r( "^/([^/]+)$" );
  if ( r.match( path.c_str() ) )
  {
    TraderInfo *info = findTrader( r.group( 1 ) );
    if ( !info )
    {
      error( ERR_DOES_NOT_EXIST, url.c_str() );
      finished();
      return;
    }

    map<string,int> done;
    list<string> lst;
    info->findServiceTypePostfixes( "", lst );
    list<string>::iterator it = lst.begin();
    for( ; it != lst.end(); ++it )
    {
      if ( done.find( *it ) == done.end() )
      {  
	UDSEntry entry;
	UDSAtom atom;
	atom.m_uds = UDS_NAME;
	atom.m_str = *it;
	entry.push_back( atom );
	atom.m_uds = UDS_FILE_TYPE;
	atom.m_long = S_IFDIR;
	entry.push_back( atom );
	
	listEntry( entry );
	done[ *it ] = 1;
      }
    }

    finished();
    return;
  }
  
  string trader;
  string type;
  if ( splitPath( path.c_str(), trader, type ) )
  {
    TraderInfo *info = findTrader( trader.c_str() );
    if ( info )
    {
      ServiceType *st = info->findServiceType( type.c_str() );
      if ( st )
      {
	UDSEntry entry;
	UDSAtom atom;
	atom.m_uds = UDS_NAME;
	atom.m_str = i18n("Information");
	entry.push_back( atom );
	atom.m_uds = UDS_FILE_TYPE;
	atom.m_long = S_IFREG;
	entry.push_back( atom );
      
	listEntry( entry );

	atom.m_uds = UDS_NAME;
	atom.m_str = i18n("Preferences");
	entry.push_back( atom );
	atom.m_uds = UDS_FILE_TYPE;
	atom.m_long = S_IFREG;
	entry.push_back( atom );
      
	listEntry( entry );

	/**
	 * Get all offers for this service type
	 */
	CosTrading::ServiceTypeName_var stype;
	CosTrading::Constraint_var constr;
	CosTrading::Lookup::Preference_var prefs;
	CosTrading::Lookup::SpecifiedProps desired;
	desired._d( CosTrading::Lookup::all );
	CosTrading::PolicySeq policyseq;
	policyseq.length( 0 );
	prefs = CORBA::string_dup( "max 1" );
	constr = CORBA::string_dup( "" );
	stype = CORBA::string_dup( type.c_str() );
	CosTrading::OfferSeq_var offers = 0L;
	CosTrading::OfferIterator_var offer_itr = 0L;
	CosTrading::PolicyNameSeq_var limits = 0L;
	info->m_vLookup->query( stype, constr, prefs, policyseq, desired, 1000, offers, offer_itr, limits );
	
	for( int k = 0; k < offers->length(); k++ )
	{
	  string name;
	  for( int i = 0; i < offers[k].properties.length(); ++i )
	    if ( strcmp( offers[k].properties[i].name.in(), "Name" ) == 0 )
	    {
	      char *s;
	      if ( offers[k].properties[i].value >>= s )
	      {  
		name = s;
		CORBA::string_free( s );
	      }
	    }
	  
	  if ( !name.empty() )
	  {    
	    UDSEntry entry;
	    UDSAtom atom;
	    atom.m_uds = UDS_NAME;
	    atom.m_str = name;
	    entry.push_back( atom );
	    atom.m_uds = UDS_FILE_TYPE;
	    atom.m_long = S_IFREG;
	    entry.push_back( atom );
	    
	    listEntry( entry );
	  }
	}
      }

      map<string,int> done;
      list<string> lst;
      info->findServiceTypePostfixes( type.c_str(), lst );
      list<string>::iterator it = lst.begin();
      for( ; it != lst.end(); ++it )
      {
	if ( done.find( *it ) == done.end() )
	{  
	  UDSEntry entry;
	  UDSAtom atom;
	  atom.m_uds = UDS_NAME;
	  atom.m_str = *it;
	  entry.push_back( atom );
	  atom.m_uds = UDS_FILE_TYPE;
	  atom.m_long = S_IFDIR;
	  entry.push_back( atom );
	  
	  listEntry( entry );
	  done[ *it ] = 1;
	}
      }
    }

    finished();
    return;
  }

  error( ERR_DOES_NOT_EXIST, url.c_str() );
  finished();
  return;  
}

void TraderProtocol::slotTestDir( const char *_url )
{
  string url = _url;
  
  K2URL usrc( _url );
  if ( usrc.isMalformed() )
  {
    error( ERR_MALFORMED_URL, url.c_str() );
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  if ( strcmp( usrc.protocol(), "trader" ) != 0 )
  {
    error( ERR_INTERNAL, "kio_trader got non trader URL as source in get command" );
    finished();
    return;
  }

  string path = usrc.path( 0 );

  if ( path == "/" )
  {
    isDirectory();
    finished();
    return;
  }

  
  KRegExp r( "^/([^/]+)$" );
  if ( r.match( path.c_str() ) )
  {
    isDirectory();
    finished();
    return;
  }
  
  string type;
  string trader;

  if ( splitPath( path.c_str(), trader, type ) )
  {
    TraderInfo *info = findTrader( trader.c_str() );
    if ( info )
    {
      list<string> lst;
      if ( info->findServiceType( type.c_str() ) || info->findServiceTypeByPrefix( type.c_str(), lst ) )
      {
	isDirectory();
	finished();
	return;
      }
    } 
  }

  isFile();

  finished();
}

bool TraderProtocol::splitPath( const char *_path, string& _trader, string& _type )
{
  KRegExp r( "^/([^/]+)/(.*[^/])$" );
  
  if ( !r.match( _path ) )
    return false;
  
  _trader = r.group( 1 );
  _type = r.group( 2 );
 
  return true;
}

bool TraderProtocol::splitPath( const char *_path, string& _trader, string& _type, string &_offer )
{
  KRegExp r( "^/([^/]+)/(.*[^/])/([^/]+)$" );
  
  if ( !r.match( _path ) )
    return false;
  
  _trader = r.group( 1 );
  _type = r.group( 2 );
  _offer = r.group( 3 );
 
  return true;
}

void TraderProtocol::slotData( void *_p, int _len )
{
  /* switch( m_cmd )
    {
    case CMD_PUT:
      fwrite( _p, 1, _len, m_fPut );
      break;
    } */
}

void TraderProtocol::slotDataEnd()
{
  /* switch( m_cmd )
    {
    case CMD_PUT:  
      m_cmd = CMD_NONE;
    } */
}

/*************************************
 *
 * Utilities
 *
 *************************************/

void openFileManagerWindow( const char * )
{
  assert( 0 );
}
