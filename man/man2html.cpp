/*
** This program was written by Richard Verhoeven (NL:5482ZX35)
** at the Eindhoven University of Technology. Email: rcb5@win.tue.nl
**
** Permission is granted to distribute, modify and use this program as long
** as this comment is not removed or changed.
*/

// kate: space-indent on; indent-width 4; replace-tabs on;

/*
 * man2html-linux-1.0/1.1
 * This version modified for Redhat/Caldera linux - March 1996.
 * Michael Hamilton <michael@actrix.gen.nz>.
 *
 * man2html-linux-1.2
 * Added support for BSD mandoc pages - I didn't have any documentation
 * on the mandoc macros, so I may have missed some.
 * Michael Hamilton <michael@actrix.gen.nz>.
 *
 * vh-man2html-1.3
 * Renamed to avoid confusion (V for Verhoeven, H for Hamilton).
 *
 * vh-man2html-1.4
 * Now uses /etc/man.config
 * Added support for compressed pages.
 * Added "length-safe" string operations for client input parameters.
 * More secure, -M secured, and client input string lengths checked.
 *
 */

/*
** If you want to use this program for your WWW server, adjust the line
** which defines the CGIBASE or compile it with the -DCGIBASE='"..."' option.
**
** You have to adjust the built-in manpath to your local system. Note that
** every directory should start and end with the '/' and that the first
** directory should be "/" to allow a full path as an argument.
**
** The program first check if PATH_INFO contains some information.
** If it does (t.i. man2html/some/thing is used), the program will look
** for a manpage called PATH_INFO in the manpath.
**
** Otherwise the manpath is searched for the specified command line argument,
** where the following options can be used:
**
** name      name of manpage (csh, printf, xv, troff)
** section   the section (1 2 3 4 5 6 7 8 9 n l 1v ...)
** -M path   an extra directory to look for manpages (replaces "/")
**
** If man2html finds multiple manpages that satisfy the options, an index
** is displayed and the user can make a choice. If only one page is
** found, that page will be displayed.
**
** man2html will add links to the converted manpages. The function add_links
** is used for that. At the moment it will add links as follows, where
**     indicates what should match to start with:
** ^^^
** Recognition           Item            Link
** ----------------------------------------------------------
** name(*)               Manpage         ../man?/name.*
**     ^
** name@hostname         Email address   mailto:name@hostname
**     ^
** method://string       URL             method://string
**       ^^^
** www.host.name         WWW server      http://www.host.name
** ^^^^
** ftp.host.name         FTP server      ftp://ftp.host.name
** ^^^^
** <file.h>              Include file    file:/usr/include/file.h
**      ^^^
**
** Since man2html does not check if manpages, hosts or email addresses exist,
** some links might not work. For manpages, some extra checks are performed
** to make sure not every () pair creates a link. Also out of date pages
** might point to incorrect places.
**
** The program will not allow users to get system specific files, such as
** /etc/passwd. It will check that "man" is part of the specified file and
** that  "/../" isn't. Even if someone manages to get such file, man2html will
** handle it like a manpage and will usually not produce any output (or crash).
**
** If you find any bugs when normal manpages are converted, please report
** them to me (rcb5@win.tue.nl) after you have checked that man(1) can handle
** the manpage correct.
**
** Known bugs and missing features:
**
**  * Equations are not converted at all.
**  * Tables are converted but some features are not possible in html.
**  * The tabbing environment is converted by counting characters and adding
**    spaces. This might go wrong (outside <PRE>)
**  * Some manpages rely on the fact that troff/nroff is used to convert
**    them and use features which are not descripted in the man manpages.
**    (definitions, calculations, conditionals, requests). I can't guarantee
**    that all these features work on all manpages. (I didn't have the
**    time to look through all the available manpages.)
*/

#ifdef SIMPLE_MAN2HTML
  // We suppose that we run on a standard Linux
# define HAVE_STRING_H 1
# define HAVE_UNISTD_H 1
#else
# include <config.h>
#endif

#include <ctype.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include <qvaluestack.h>
#include <qstring.h>
#include <qptrlist.h>
#include <qmap.h>

#ifdef SIMPLE_MAN2HTML
# include <stdlib.h>
# include <iostream>
# include <stdio.h>
# include <dirent.h>
# include <sys/stat.h>
#else
# include <qtextcodec.h>
# include <kdebug.h>
#endif

#include "man2html.h"

using namespace std;

#define NULL_TERMINATED(n) ((n) + 1)

#define HUGE_STR_MAX  10000
#define LARGE_STR_MAX 2000
#define MED_STR_MAX   500
#define SMALL_STR_MAX 100
#define TINY_STR_MAX  10

#define MAX_WORDLIST  100

#if 1
// The output is current too horrible to be called HTML 4.01
#define DOCTYPE "<!DOCTYPE HTML>"
#else
#define DOCTYPE "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
#endif

/* mdoc(7) Bl/El lists to HTML list types */
#define BL_DESC_LIST   1
#define BL_BULLET_LIST 2
#define BL_ENUM_LIST   4

/* mdoc(7) Bd/Ed example(?) blocks */
#define BD_LITERAL  1
#define BD_INDENT   2

static int s_nroff = 1; // NROFF mode by default

static int mandoc_name_count = 0; /* Don't break on the first Nm */

static char *stralloc(int len)
{
  /* allocate enough for len + NULL */
  char *news = new char [len+1];
#ifdef SIMPLE_MAN2HTML
  if (!news)
  {
    fprintf(stderr, "man2html: out of memory\n");
    exit(EXIT_FAILURE);
  }
#else
// modern compilers do not return a NULL pointer for a new
#endif
  return news;
}

// ### TODO: replace by either qstrncpy or strlcpy
static char *strmaxcpy(char *to, const char *from, int n)
{				/* Assumes space for n plus a null */
  int len = qstrlen(from);
  qstrncpy(to, from, n);
  to[(len <= n) ? len : n] = '\0';
  return to;
}

#if 0 // unused
static char *strmaxcat(char *to, char *from, int n)
{				/* Assumes space for n plus a null */
  int to_len = qstrlen(to);
  if (to_len < n) {
    int from_len = qstrlen(from);
    int cp = (to_len + from_len <= n) ? from_len : n - to_len;
    strncpy(to + to_len, from, cp);
    to[to_len + cp] = '\0';
  }
  return to;
}
#endif

static char *strlimitcpy(char *to, char *from, int n, int limit)
{                               /* Assumes space for limit plus a null */
  int len = n > limit ? limit : n;
  strmaxcpy(to, from, len);
  to[len] = '\0';
  return to;
}

/* below this you should not change anything unless you know a lot
** about this program or about troff.
*/


/// Structure for character definitions
struct CSTRDEF {
    int nr, slen;
    const char *st;
};



const char NEWLINE[2]="\n";

/**
 * Class for defining strings and macros
 */
class StringDefinition
{
public:
    StringDefinition( void ) : m_length(0) {}
    StringDefinition( int len, const char* cstr ) : m_length( len ), m_output( cstr ) {}
public:
    int m_length; ///< Length of output text
    QCString m_output; ///< Defined string
};

/**
 * Class for defining number registers
 * \note Not for internal read-only registers
 */
class NumberDefinition
{
    public:
        NumberDefinition( void ) : m_value(0), m_increment(0) {}
        NumberDefinition( int value ) : m_value( value ), m_increment(0) {}
        NumberDefinition( int value, int incr) : m_value( value ), m_increment( incr ) {}
    public:
        int m_value; ///< value of number register
        int m_increment; ///< Increment of number register
        // ### TODO: display form (.af)
};

/**
 * Map of string and macro definitions
 * \note Strings and macros are the same thing!
 */
static QMap<QCString,StringDefinition> s_stringDefinitionMap;

/**
 * Map of number registers
 * \note Intern number registers (starting with a dot are not handled here)
 */
static QMap<QCString,NumberDefinition> s_numberDefinitionMap;

static void InitStringDefinitions( void )
{
    // mdoc-only, see mdoc.samples(7)
    s_stringDefinitionMap.insert( "<=", StringDefinition( 1, "&le;" ) );
    s_stringDefinitionMap.insert( ">=", StringDefinition( 1, "&ge;" ) );
    s_stringDefinitionMap.insert( "Rq", StringDefinition( 1, "&rdquo;" ) );
    s_stringDefinitionMap.insert( "Lq", StringDefinition( 1, "&ldquo;" ) );
    s_stringDefinitionMap.insert( "ua", StringDefinition( 1, "&circ" ) ); // Note this is different from \(ua
    s_stringDefinitionMap.insert( "aa", StringDefinition( 1, "&acute;" ) );
    s_stringDefinitionMap.insert( "ga", StringDefinition( 1, "&grave;" ) );
    s_stringDefinitionMap.insert( "q",  StringDefinition( 1, "&quot;" ) );
    s_stringDefinitionMap.insert( "Pi", StringDefinition( 1, "&pi;" ) );
    s_stringDefinitionMap.insert( "Ne", StringDefinition( 1, "&ne;" ) );
    s_stringDefinitionMap.insert( "Le", StringDefinition( 1, "&le;" ) );
    s_stringDefinitionMap.insert( "Ge", StringDefinition( 1, "&ge;" ) );
    s_stringDefinitionMap.insert( "Lt", StringDefinition( 1, "&lt;" ) );
    s_stringDefinitionMap.insert( "Gt", StringDefinition( 1, "&gt;" ) );
    s_stringDefinitionMap.insert( "Pm", StringDefinition( 1, "&plusmn;" ) );
    s_stringDefinitionMap.insert( "If", StringDefinition( 1, "&infin;" ) );
    s_stringDefinitionMap.insert( "Na", StringDefinition( 3, "NaN" ) ); // Not a Number ### TODO: does it exist in Unicode?
    s_stringDefinitionMap.insert( "Ba", StringDefinition( 1, "|" ) );
    // end mdoc-only
    // man(7)
    s_stringDefinitionMap.insert( "Tm", StringDefinition( 1, "&trade;" ) ); // \*(TM
    s_stringDefinitionMap.insert( "R", StringDefinition( 1, "&reg;" ) ); // \*R
    // end man(7)
    // Missing characters from man(7):
    // \*S "Change to default font size"
}


#define V(A,B) ((A)*256+(B))

#if 0 // unused, seems to be for index stuff
static INTDEF standardint[] = {
    { V('n',' '), s_nroff,0, NULL },
    { V('t',' '), 1-s_nroff,0, NULL },
    { V('o',' '), 1,0, NULL },
    { V('e',' '), 0,0, NULL },
    { V('.','l'), 70,0,NULL },
    { V('.','$'), 0,0, NULL },
    { V('.','A'), s_nroff,0, NULL },
    { V('.','T'), 1-s_nroff,0, NULL },
    { V('.','V'), 1,0, NULL }, /* the me package tests for this */
    { 0, 0, 0, NULL } };
#endif


//used in expand_char, e.g. for "\(bu"
// see groff_char(7) for list
static CSTRDEF standardchar[] = {
    { V('*','*'), 1, "*" },
    { V('*','A'), 1, "&Alpha;" },
    { V('*','B'), 1, "&Beta;" },
    { V('*','C'), 1, "&Xi;" },
    { V('*','D'), 1, "&Delta;" },
    { V('*','E'), 1, "&Epsilon;" },
    { V('*','F'), 1, "&Phi;" },
    { V('*','G'), 1, "&Gamma;" },
    { V('*','H'), 1, "&Theta;" },
    { V('*','I'), 1, "&Iota;" },
    { V('*','K'), 1, "&Kappa;" },
    { V('*','L'), 1, "&Lambda;" },
    { V('*','M'), 1, "&Mu:" },
    { V('*','N'), 1, "&Nu;" },
    { V('*','O'), 1, "&Omicron;" },
    { V('*','P'), 1, "&Pi;" },
    { V('*','Q'), 1, "&Psi;" },
    { V('*','R'), 1, "&Rho;" },
    { V('*','S'), 1, "&Sigma;" },
    { V('*','T'), 1, "&Tau;" },
    { V('*','U'), 1, "&Upsilon;" },
    { V('*','W'), 1, "&Omega;" },
    { V('*','X'), 1, "&Chi;" },
    { V('*','Y'), 1, "&Eta;" },
    { V('*','Z'), 1, "&Zeta;" },
    { V('*','a'), 1, "&alpha;"},
    { V('*','b'), 1, "&beta;"},
    { V('*','c'), 1, "&xi;"},
    { V('*','d'), 1, "&delta;"},
    { V('*','e'), 1, "&epsilon;"},
    { V('*','f'), 1, "&phi;"},
    { V('*','g'), 1, "&gamma;"},
    { V('*','h'), 1, "&theta;"},
    { V('*','i'), 1, "&iota;"},
    { V('*','k'), 1, "&kappa;"},
    { V('*','l'), 1, "&lambda;"},
    { V('*','m'), 1, "&mu;" },
    { V('*','n'), 1, "&nu;"},
    { V('*','o'), 1, "&omicron;"},
    { V('*','p'), 1, "&pi;"},
    { V('*','q'), 1, "&psi;"},
    { V('*','r'), 1, "&rho;"},
    { V('*','s'), 1, "&sigma;"},
    { V('*','t'), 1, "&tau;"},
    { V('*','u'), 1, "&upsilon;"},
    { V('*','w'), 1, "&omega;"},
    { V('*','x'), 1, "&chi;"},
    { V('*','y'), 1, "&eta;"},
    { V('*','z'), 1, "&zeta;"},
    { V('+','-'), 1, "&plusmn;" }, // not in groff_char(7)
    { V('+','f'), 1, "&phi;"}, // phi1, we use the standard phi
    { V('+','h'), 1, "&theta;"}, // theta1, we use the standard theta
    { V('+','p'), 1, "&omega;"}, // omega1, we use the standard omega
    { V('1','2'), 1, "&frac12;" },
    { V('1','4'), 1, "&frac14;" },
    { V('3','4'), 1, "&frac34;" },
    { V('F','i'), 1, "&#xFB03;" }, // ffi ligature
    { V('F','l'), 1, "&#xFB04;" }, // ffl ligature
    { V('a','p'), 1, "~" },
    { V('b','r'), 1, "|" },
    { V('b','u'), 1, "&bull;" },
    { V('b','v'), 1, "|" },
    { V('c','i'), 1, "&#x25CB;" }, // circle ### TODO verify
    { V('c','o'), 1, "&copy;" },
    { V('c','t'), 1, "&cent;" },
    { V('d','e'), 1, "&deg;" },
    { V('d','g'), 1, "&dagger;" },
    { V('d','i'), 1, "&divide;" },
    { V('e','m'), 1, "&emdash;" },
    { V('e','n'), 1, "&endash;"},
    { V('e','q'), 1, "=" },
    { V('e','s'), 1, "&empty;" },
    { V('f','f'), 1, "&#0xFB00;" }, // ff ligature
    { V('f','i'), 1, "&#0xFB01;" }, // fi ligature
    { V('f','l'), 1, "&#0xFB02;" }, // fl ligature
    { V('f','m'), 1, "&prime;" },
    { V('g','a'), 1, "`" },
    { V('h','y'), 1, "-" },
    { V('l','c'), 2, "|&#175;" }, // ### TODO: not in groff_char(7)
    { V('l','f'), 2, "|_" }, // ### TODO: not in groff_char(7)
    { V('l','k'), 1, "<FONT SIZE=+2>{</FONT>" }, // ### TODO: not in groff_char(7)
    { V('m','i'), 1, "-" }, // ### TODO: not in groff_char(7)
    { V('m','u'), 1, "&times;" },
    { V('n','o'), 1, "&not;" },
    { V('o','r'), 1, "|" },
    { V('p','l'), 1, "+" },
    { V('r','c'), 2, "&#175;|" }, // ### TODO: not in groff_char(7)
    { V('r','f'), 2, "_|" },  // ### TODO: not in groff_char(7)
    { V('r','g'), 1, "&reg;" },
    { V('r','k'), 1, "<FONT SIZE=+2>}</FONT>" }, // ### TODO: not in groff_char(7)
    { V('r','n'), 1, "&oline;" },
    { V('r','u'), 1, "_" },
    { V('s','c'), 1, "&sect;" },
    { V('s','l'), 1, "/" },
    { V('s','q'), 2, "[]" }, // ### TODO verify
    { V('t','s'), 1, "&#x03C2;" }, // terminal sigma
    { V('u','l'), 1, "_" },
    { V('-','D'), 1, "&ETH;" },
    { V('S','d'), 1, "&eth;" },
    { V('T','P'), 1, "&THORN;" },
    { V('T','p'), 1, "&thorn;" },
    { V('A','E'), 1, "&AElig;" },
    { V('a','e'), 1, "&aelig;" },
    { V('O','E'), 1, "&OElig;" },
    { V('o','e'), 1, "&oelig;" },
    { V('s','s'), 1, "&szlig;" },
    { V('\'','A'), 1, "&Aacute;" },
    { V('\'','E'), 1, "&Eacute;" },
    { V('\'','I'), 1, "&Iacute;" },
    { V('\'','O'), 1, "&Oacute;" },
    { V('\'','U'), 1, "&Uacute;" },
    { V('\'','Y'), 1, "&Yacute;" },
    { V('\'','a'), 1, "&aacute;" },
    { V('\'','e'), 1, "&eacute;" },
    { V('\'','i'), 1, "&iacute;" },
    { V('\'','o'), 1, "&oacute;" },
    { V('\'','u'), 1, "&uacute;" },
    { V('\'','y'), 1, "&yacute;" },
    { V(':','A'), 1, "&Auml;" },
    { V(':','E'), 1, "&Euml;" },
    { V(':','I'), 1, "&Iuml;" },
    { V(':','O'), 1, "&Ouml;" },
    { V(':','U'), 1, "&Uuml;" },
    { V(':','a'), 1, "&auml;" },
    { V(':','e'), 1, "&euml;" },
    { V(':','i'), 1, "&iuml;" },
    { V(':','o'), 1, "&ouml;" },
    { V(':','u'), 1, "&uuml;" },
    { V(':','y'), 1, "&yuml;" },
    { V('^','A'), 1, "&Acirc;" },
    { V('^','E'), 1, "&Ecirc;" },
    { V('^','I'), 1, "&Icirc;" },
    { V('^','O'), 1, "&Ocirc;" },
    { V('^','U'), 1, "&Ucirc;" },
    { V('^','a'), 1, "&acirc;" },
    { V('^','e'), 1, "&ecirc;" },
    { V('^','i'), 1, "&icirc;" },
    { V('^','o'), 1, "&ocirc;" },
    { V('^','u'), 1, "&ucirc;" },
    { V('`','A'), 1, "&Agrave;" },
    { V('`','E'), 1, "&Egrave;" },
    { V('`','I'), 1, "&Igrave;" },
    { V('`','O'), 1, "&Ograve;" },
    { V('`','U'), 1, "&Ugrave;" },
    { V('`','a'), 1, "&agrave;" },
    { V('`','e'), 1, "&egrave;" },
    { V('`','i'), 1, "&igrave;" },
    { V('`','o'), 1, "&ograve;" },
    { V('`','u'), 1, "&ugrave;" },
    { V('~','A'), 1, "&Atilde;" },
    { V('~','N'), 1, "&Ntilde;" },
    { V('~','O'), 1, "&Otilde;" },
    { V('~','a'), 1, "&atilde" },
    { V('~','n'), 1, "&ntidle;" },
    { V('~','o'), 1, "&otidle;" },
    { V(',','C'), 1, "&Ccedil;" },
    { V(',','c'), 1, "&ccedil;" },
    { V('/','L'), 1, "&#x0141;" }, // ### TODO: verify: U+0141 is with stroke, not with slash
    { V('/','l'), 1, "&#x0142;" }, // ### TODO: verify: U+0142 is with stroke, not with slash
    { V('/','O'), 1, "&Oslash;" },
    { V('/','o'), 1, "&oslash;" },
    { V('o','A'), 1, "&Aring;" },
    { V('o','a'), 1, "&aring;" },
    { V('a','"'), 1, "\"" },
    { V('a','-'), 1, "&macr;" },
    { V('a','.'), 1, "." },
    { V('a','^'), 1, "&circ;" },
    { V('a','a'), 1, "&acute;" },
    { V('a','b'), 1, "`" },
    { V('a','c'), 1, "&cedil;" },
    { V('a','d'), 1, "&uml;" },
    { V('a','h'), 1, "&#x02C2;" }, // caron
    { V('a','o'), 1, "&#x02DA;" }, // ring
    { V('a','~'), 1, "&tilde;" },
    { V('h','o'), 1, "&#x02DB;" }, // ogonek
    { V('.','i'), 1, "&#x0131;" }, // dot less i
    { V('C','s'), 1, "&curren;" },
    { V('D','o'), 1, "$" },
    { V('P','o'), 1, "&pound;" },
    { V('Y','e'), 1, "&yen;" },
    { V('F','n'), 1, "&fnof;" },
    { V('F','o'), 1, "&laquo;" },
    { V('F','c'), 1, "&raquo;" },
    { V('f','o'), 1, "&#x2039;" }, // single left guillemet
    { V('f','c'), 1, "&#x203A;" }, // single right guillemet
    { V('r','!'), 1, "&iecl;" },
    { V('r','?'), 1, "&iquest;" },
    { V('O','f'), 1, "&ordf" },
    { V('O','m'), 1, "&ordm;" },
    { V('p','c'), 1, "&middot;" },
    { V('S','1'), 1, "&sup1;" },
    { V('S','2'), 1, "&sup2;" },
    { V('S','3'), 1, "&sup3;" },
    { V('<','-'), 1, "&larr;" },
    { V('-','>'), 1, "&rarr;" },
    { V('<','>'), 1, "&harr;" },
    { V('d','a'), 1, "&darr;" },
    { V('u','a'), 1, "&uarr;" },
    { V('l','A'), 1, "&lArr;" },
    { V('r','A'), 1, "&rArr;" },
    { V('h','A'), 1, "&hArr;" },
    { V('d','A'), 1, "&dArr;" },
    { V('u','A'), 1, "&uArr;" },
    { V('b','a'), 1, "|" },
    { V('b','b'), 1, "&brvbar;" },
    { V('t','m'), 1, "&trade;" },
    { V('d','d'), 1, "&Dagger;" },
    { V('p','s'), 1, "&para;" },
    { V('%','0'), 1, "&permil;" },
    { V('f','/'), 1, "&frasl;" }, // Fraction slash
    { V('s','d'), 1, "&Prime;" },
    { V('h','a'), 1, "^" },
    { V('t','i'), 1, "&tidle;" },
    { V('l','B'), 1, "[" },
    { V('r','B'), 1, "]" },
    { V('l','C'), 1, "{" },
    { V('r','C'), 1, "}" },
    { V('l','a'), 1, "&lt;" },
    { V('r','a'), 1, "&gt;" },
    { V('l','h'), 1, "&le;" },
    { V('r','h'), 1, "&ge;" },
    { V('B','q'), 1, "&bdquo;" },
    { V('b','q'), 1, "&sbquo;" },
    { V('l','q'), 1, "&ldquo;" },
    { V('r','q'), 1, "&rdquo;" },
    { V('o','q'), 1, "&lsquo;" },
    { V('c','q'), 1, "&rsquo;" },
    { V('a','q'), 1, "'" },
    { V('d','q'), 1, "\"" },
    { V('a','t'), 1, "@" },
    { V('s','h'), 1, "#" },
    { V('r','s'), 1, "\\" },
    { V('t','f'), 1, "&there4;" },
    { V('~','~'), 1, "&cong;" },
    { V('~','='), 1, "&asymp;" },
    { V('!','='), 1, "&ne;" },
    { V('<','='), 1, "&le;" },
    { V('=','='), 1, "&equiv;" },
    { V('=','~'), 1, "&cong;" }, // ### TODO: verify
    { V('>','='), 1, "&ge;" },
    { V('A','N'), 1, "&and;" },
    { V('O','R'), 1, "&or;" },
    { V('t','e'), 1, "&exist;" },
    { V('f','a'), 1, "&forall;" },
    { V('A','h'), 1, "&alefsym;" },
    { V('I','m'), 1, "&image;" },
    { V('R','e'), 1, "&real;" },
    { V('i','f'), 1, "&infin;" },
    { V('m','d'), 1, "&sdot;" },
    { V('m','o'), 1, "&#x2206;" }, // element ### TODO verify
    { V('n','m'), 1, "&notin;" },
    { V('p','t'), 1, "&prop;" },
    { V('p','p'), 1, "&perp;" },
    { V('s','b'), 1, "&sub;" },
    { V('s','p'), 1, "&sup;" },
    { V('i','b'), 1, "&sube;" },
    { V('i','p'), 1, "&supe;" },
    { V('i','s'), 1, "&int;" },
    { V('s','r'), 1, "&radic;" },
    { V('p','d'), 1, "&part;" },
    { V('c','*'), 1, "&otimes;" },
    { V('c','+'), 1, "&oplus;" },
    { V('c','a'), 1, "&cap;" },
    { V('c','u'), 1, "&cup;" },
    { V('g','r'), 1, "V" }, // gradient ### TODO Where in Unicode?
    { V('C','R'), 1, "&crarr;" },
    { V('s','t'), 2, "-)" }, // "such that" ### TODO Where in Unicode?
    { V('/','_'), 1, "&ang;" },
    { V('w','p'), 1, "&weierp;" },
    { V('l','z'), 1, "&loz;" },
    { V('a','n'), 1, "-" }, // "horizontal arrow extension"  ### TODO Where in Unicode?
};

/* default: print code */


/* static char eqndelimopen=0, eqndelimclose=0; */
static char escapesym='\\', nobreaksym='\'', controlsym='.', fieldsym=0, padsym=0;

static char *buffer=NULL;
static int buffpos=0, buffmax=0;
static bool scaninbuff=false;
static int itemdepth=0;
static int section=0;
static int dl_set[20]= { 0 };
static bool still_dd=0;
static int tabstops[20] = { 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96 };
static int maxtstop=12;
static int curpos=0;

static char *scan_troff(char *c, bool san, char **result);
static char *scan_troff_mandoc(char *c, bool san, char **result);

static char **argument=NULL;

static char charb[TINY_STR_MAX];

static QCString htmlPath, cssPath;

static QCString s_dollarZero; // Value of $0

void setResourcePath(const QCString& _htmlPath, const QCString& _cssPath)
{
    htmlPath=_htmlPath;
    cssPath=_cssPath;
}

// ### TODO: a char can also have long names
static const char *expand_char(int nr)
{
  if (!nr) return NULL;
  for (size_t i = 0; i < sizeof(standardchar)/sizeof(CSTRDEF); i++) {
      if (standardchar[i].nr==nr) {
          curpos+=standardchar[i].slen;
          return standardchar[i].st;
      }
  }
  charb[0]=nr/256;
  charb[1]=nr%256;
  charb[2]='\0';
  if (charb[0] == '<') {	/* Fix up <= */
    charb[4] = charb[1];
    qstrncpy(charb, "&lt;", 4);
    charb[5] = '\0';
  }
  curpos+=2;
  return charb;
}

static char outbuffer[NULL_TERMINATED(HUGE_STR_MAX)];
static int no_newline_output=0;
static int newline_for_fun=0;
static bool output_possible=false;

static const char *includedirs[] = {
    "/usr/include",
    "/usr/include/sys",
    "/usr/local/include",
    "/opt/local/include",
    "/usr/ccs",
    "/usr/X11R6/include",
    "/usr/openwin/include",
    "/usr/include/g++",
    0
};

static bool ignore_links=false;

static void add_links(char *c)
{
    /*
    ** Add the links to the output.
    ** At the moment the following are recognized:
    **
    ** name(*)                 -> ../man?/name.*
    ** method://string         -> method://string
    ** www.host.name           -> http://www.host.name
    ** ftp.host.name           -> ftp://ftp.host.name
    ** name@host               -> mailto:name@host
    ** <name.h>                -> file:/usr/include/name.h   (guess)
    **
    ** Other possible links to add in the future:
    **
    ** /dir/dir/file  -> file:/dir/dir/file
    */
    if (ignore_links)
    {
        output_real(c);
        return;
    }

    int i,j,nr;
    char *f, *g,*h;
    const int numtests=6; // Nmber of tests
    char *idtest[numtests]; // url, mailto, www, ftp, manpage, C header file
    bool ok;
    /* search for (section) */
    nr=0;
    idtest[0]=strstr(c+1,"://");
    idtest[1]=strchr(c+1,'@');
    idtest[2]=strstr(c,"www.");
    idtest[3]=strstr(c,"ftp.");
    idtest[4]=strchr(c+1,'(');
    idtest[5]=strstr(c+1,".h&gt;");
    for (i=0; i<numtests; ++i) nr += (idtest[i]!=NULL);
    while (nr) {
	j=-1;
	for (i=0; i<numtests; i++)
	    if (idtest[i] && (j<0 || idtest[i]<idtest[j])) j=i;
	switch (j) {
	case 5: { /* <name.h> */
	    f=idtest[5];
	    h=f+2;
	    g=f;
	    while (g>c && g[-1]!=';') g--;
            bool wrote_include = false;

            if (g!=c) {

                QCString dir;
                QCString file(g, h - g + 1);
                file = file.stripWhiteSpace();
                for (int index = 0; includedirs[index]; index++) {
                    QCString str = QCString(includedirs[index]) + "/" + file;
                    if (!access(str, R_OK)) {
                        dir = includedirs[index];
                        break;
                    }
                }
                if (!dir.isEmpty()) {

                    char t;
                    t=*g;
                    *g=0;
                    output_real(c);
                    *g=t;*h=0;

                    QCString str;
                    str.sprintf("<A HREF=\"file:%s/%s\">%s</A>&gt;", dir.data(), file.data(), file.data());
                    output_real(str.data());
                    c=f+6;
                    wrote_include = true;
                }

            }

            if (!wrote_include) {
                f[5]=0;
                output_real(c);
                f[5]=';';
                c=f+5;
            }
        }
        break;
	case 4: /* manpage */
	    f=idtest[j];
	    /* check section */
	    g=strchr(f,')');
	    if (g!=NULL && (g-f)<12 && (isalnum(f[-1]) || f[-1]=='>') &&
		isdigit(f[1]) && f[1]!='0' && ((g-f)<=2 || isalpha(f[2])))
	    {
		ok = TRUE;
		h = f+2;
		while (h<g)
		{
		    if (!isalnum(*h++))
		    {
			ok = FALSE;
			break;
		    }
		}
	    }
	    else ok = FALSE;

	    if (ok)
	    {
		/* this might be a link */
		h=f-1;
		/* skip html makeup */
		while (h>c && *h=='>') {
		    while (h!=c && *h!='<') h--;
		    if (h!=c) h--;
		}
		if (isalnum(*h)) {
		    char t,sec, *e;
                    QString subsec; // ### TODO avoid using QString, as we do not know the encoding
                    QString fstr(f); // ### TODO avoid using QString, as we do not know the encoding
		    e=h+1;
		    sec=f[1];
		    subsec=f[2];
		    int index = fstr.find(')', 2);
		    if (index != -1)
		      subsec = fstr.mid(2, index - 2);
		    else // No closing ')' found, take first character as subsection.
		      subsec = fstr.mid(2, 1);
		    while (h>c && (isalnum(h[-1]) || h[-1]=='_'
				    || h[-1]==':' || h[-1]=='-' || h[-1]=='.'))
			h--;
		    t=*h;
		    *h='\0';
                    output_real(c);
		    *h=t;
		    t=*e;
		    *e='\0';
                    QCString str;
		    if (subsec.isEmpty())
                        str.sprintf("<A HREF=\"man:%s(%c)\">%s</A>", h, sec, h);
		    else
                        str.sprintf("<A HREF=\"man:%s(%c%s)\">%s</A>", h, sec, subsec.lower().latin1(), h);
                    output_real(str.data());
		    *e=t;
		    c=e;
		}
	    }
	    *f='\0';
            output_real(c);
	    *f='(';
	    idtest[4]=f-1;
	    c=f;
	    break; /* manpage */
	case 3: /* ftp */
	case 2: /* www */
	    g=f=idtest[j];
	    while (*g && (isalnum(*g) || *g=='_' || *g=='-' || *g=='+' ||
                *g=='.' || *g=='/')) g++;
	    if (g[-1]=='.') g--;
	    if (g-f>4) {
		char t;
		t=*f; *f='\0';
                output_real(c);
		*f=t; t=*g;*g='\0';
                QCString str;
                str.sprintf("<A HREF=\"%s://%s\">%s</A>", ((j==3)?"ftp":"http"), f, f);
                output_real(str.data());
		*g=t;
		c=g;
	    } else {
		f[3]='\0';
                output_real(c);
		c=f+3;
		f[3]='.';
	    }
	    break;
	case 1: /* mailto */
	    g=f=idtest[1];
	    while (g>c && (isalnum(g[-1]) || g[-1]=='_' || g[-1]=='-' ||
			   g[-1]=='+' || g[-1]=='.' || g[-1]=='%')) g--;
            if (g-7>=c && g[-1]==':')
            {
                // We have perhaps an email address starting with mailto:
                if (!qstrncmp("mailto:",g-7,7))
                    g-=7;
            }
	    h=f+1;
	    while (*h && (isalnum(*h) || *h=='_' || *h=='-' || *h=='+' ||
			  *h=='.')) h++;
	    if (*h=='.') h--;
	    if (h-f>4 && f-g>1) {
		char t;
		t=*g;
		*g='\0';
                output_real(c);
		*g=t;t=*h;*h='\0';
                QCString str;
                str.sprintf("<A HREF=\"mailto:%s\">%s</A>", g, g);
                output_real(str.data());
		*h=t;
                c=h;
	    } else {
		*f='\0';
                output_real(c);
		*f='@';
		idtest[1]=c;
		c=f;
	    }
	    break;
	case 0: /* url */
	    g=f=idtest[0];
	    while (g>c && isalpha(g[-1]) && islower(g[-1])) g--;
	    h=f+3;
	    while (*h && !isspace(*h) && *h!='<' && *h!='>' && *h!='"' &&
		   *h!='&') h++;
	    if (f-g>2 && f-g<7 && h-f>3) {
		char t;
		t=*g;
		*g='\0';
                output_real(c);
		*g=t; t=*h; *h='\0';
                QCString str;
                str.sprintf("<A HREF=\"%s\">%s</A>", g, g);
                output_real(str.data());
		*h=t;
		c=h;
	    } else {
		f[1]='\0';
                output_real(c);
		f[1]='/';
		c=f+1;
	    }
	    break;
     default:
	    break;
	}
	nr=0;
	if (idtest[0] && idtest[0]<c) idtest[0]=strstr(c+1,"://");
	if (idtest[1] && idtest[1]<c) idtest[1]=strchr(c+1,'@');
	if (idtest[2] && idtest[2]<c) idtest[2]=strstr(c,"www.");
	if (idtest[3] && idtest[3]<c) idtest[3]=strstr(c,"ftp.");
	if (idtest[4] && idtest[4]<c) idtest[4]=strchr(c+1,'(');
	if (idtest[5] && idtest[5]<c) idtest[5]=strstr(c+1,".h&gt;");
        for (i=0; i<numtests; i++) nr += (idtest[i]!=NULL);
    }
    output_real(c);
}

static int current_font=0;
static int current_size=0;
static int fillout=1;

static void out_html(const char *c)
{
  if (!c) return;

  // Added, probably due to the const?
  char *c2 = qstrdup(c);
  char *c3 = c2;

  static int obp=0;

  if (no_newline_output) {
      int i=0;
      no_newline_output=1;
      while (c2[i]) {
	  if (!no_newline_output) c2[i-1]=c2[i];
	  if (c2[i]=='\n') no_newline_output=0;
	  i++;
      }
      if (!no_newline_output) c2[i-1]=0;
  }
  if (scaninbuff) {
      while (*c2) {
	  if (buffpos>=buffmax) {
	      char *h = new char[buffmax*2];
       
#ifdef SIMPLE_MAN2HTML
	      if (!h)
              {
                  fprintf(stderr,"Memory full, cannot output!");
                  exit(1);
              }
#else
// modern compiler do not return a NULL for a new
#endif       
              memcpy(h, buffer, buffmax);
              delete [] buffer;
	      buffer=h;
	      buffmax=buffmax*2;
	  }
	  buffer[buffpos++]=*c2++;
      }
  } else
      if (output_possible) {
	  while (*c2) {
	      outbuffer[obp++]=*c2;
	      if (*c=='\n' || obp >= HUGE_STR_MAX) {
		  outbuffer[obp]='\0';
		  add_links(outbuffer);
		  obp=0;
	      }
	      c2++;
	  }
      }
  delete [] c3;
}

#define FO0 ""
#define FC0 ""
#define FO1 "<span class=\"parameter\">"
#define FC1 "</span>"
#define FO2 "<span class=\"option\">"
#define FC2 "</span>"
#define FO3 "<TT>"
#define FC3 "</TT>"

static const char *switchfont[16] = { ""     , FC0 FO1, FC0 FO2, FC0 FO3,
			 FC1 FO0, ""     , FC1 FO2, FC1 FO3,
			 FC2 FO0, FC2 FO1, ""     , FC2 FO3,
			 FC3 FO0, FC3 FO1, FC3 FO2, ""      };

static const char *change_to_font(int nr)
{
  int i;
  switch (nr) {
  case '0': nr++;
  case '1': case '2': case '3': case '4': nr=nr-'1'; break;
  case V('C','W'): nr=3; break;
  case 'L': nr=3; break;
  case 'B': nr=2; break;
  case 'I': nr=1; break;
  case 'P': case 'R': nr=0; break;
  case 0: case 1: case 2: case 3: break;
  default: nr=0; break;
  }
  i= current_font*4+nr%4;
  current_font=nr%4;
  return switchfont[i];
}

static char sizebuf[200];

static const char *change_to_size(int nr)
{
  int i;
  switch (nr) {
  case '0': case '1': case '2': case '3': case '4': case '5': case '6':
  case '7': case '8': case '9': nr=nr-'0'; break;
  case '\0': break;
  default: nr=current_size+nr; if (nr>9) nr=9; if (nr< -9) nr=-9; break;
  }
  if (nr==current_size) return "";
  i=current_font;
  sizebuf[0]='\0';
  strcat(sizebuf, change_to_font(0));
  if (current_size) strcat(sizebuf, "</FONT>");
  current_size=nr;
  if (nr) {
    int l;
    strcat(sizebuf, "<FONT SIZE=\"");
    l=qstrlen(sizebuf);
    if (nr>0) sizebuf[l++]='+'; else sizebuf[l++]='-',nr=-nr;
    sizebuf[l++]=nr+'0';
    sizebuf[l++]='"';
    sizebuf[l++]='>';
    sizebuf[l]='\0';
  }
  strcat(sizebuf, change_to_font(i));
  return sizebuf;
}

/* static int asint=0; */
static int intresult=0;

#define SKIPEOL while (*c && *c++!='\n')

static bool skip_escape=false;
static bool single_escape=false;

static char* scan_named_character(char* c)
{
    QCString name;
    if ( *c == '(' )
    {
        // \*(ab  Name of two characters
        name=c[1];
        name+=c[2];
        c+=2;
    }
    else if ( *c == '[' )
    {
        // \*[long_name]  Long name
        // Named character groff(7)
        // We must find the ] to get a name
        c++;
        while ( *c && *c != ']' && *c != '\n' )
        {
            name+=*c;
            c++;
        }
        if ( !*c || *c == '\n' )
        {
#ifndef SIMPLE_MAN2HTML
            kdDebug(7107) << "Found linefeed! Could not parse character name: " << name << endl;
#endif
            return c-1;
        }
    }
    else
    {
        // \*a Name of one character
        name=c[1];
        ++c;
    }
    if ( skip_escape )
        return c;
    // Now we have the name, let us find it between the string names
    QMap<QCString,StringDefinition>::iterator it=s_stringDefinitionMap.find(name);
    if (it==s_stringDefinitionMap.end())
    {
#ifndef SIMPLE_MAN2HTML
        kdDebug(7107) << "EXCEPTION: cannot find string with long name: " << name << endl;
#endif
        // No output, as an undefined string is empty by default
    }
    else
    {
#ifndef SIMPLE_MAN2HTML
        kdDebug(7107) << "String with long name: " << name << endl;
#endif
        out_html((*it).m_output);
    }
    return c;
}

static char* scan_dollar_parameter(char*& c)
{
    int argno = 0; // No dollar argument number yet!
    char* h = ""; // Not NULL !
    if ( *c == '0' )
    {
        //kdDebug(7107) << "$0" << endl;
        h = s_dollarZero.data();
        return h;
    }
    else if ( *c >= '1' && *c <= '9' )
    {
        //kdDebug(7107) << "$ direct" << endl;
        argno = ( *c - '0' );
    }
    else if ( *c == '(' )
    {
        //kdDebug(7107) << "$(" << endl;
        if ( c[1] && c[2] && c[1] >= '0' && c[1] <= '9' && c[2] >= '0' && c[2] <= '9' )
        {
            argno = ( c[1] - '0' ) * 10 + ( c[2] - '0' );
            c += 3;
        }
        else
        {
            if ( !c[1] )
                ;
            else if ( !c[2] )
                c ++;
            else
                c += 2;
            return "";
        }
    }
    else if ( *c == '[' )
    {
        //kdDebug(7107) << "$[" << endl;
        argno = 0;
        c++;
        while ( *c && *c>='0' && *c<='9' && *c!=']' )
        {
            argno *= 10;
            argno += ( *c - '0' );
            c++;
        }
        if ( *c != ']' )
        {
            return "";
        }
    }
    else
    {
        //kdDebug(7107) << "$ char: " << *c << endl;
    }
    // ### TODO $* $@
    //kdDebug(7107) << "ARG $" << argno << endl;
    if (argument && argno > 0 )
    {
        //kdDebug(7107) << "ARG $" << argno << " OK!" << endl;
        argno--;
        // ### TODO support more than 20 arguments
        if (argno < 0 || argno > (int)qstrlen(*argument))
        {
#ifndef SIMPLE_MAN2HTML
            kdDebug(7107) << "EXCEPTION: cannot find parameter number: " << (argno+1) << endl;
#endif
            return "";
        }

        h=argument[argno];
        if (!h)
            h="";
    }
    return h;
}

// ### TODO known missing escapes from groff(7):
// ### TODO \& \! \) \:

static char *scan_escape(char *c)
{
    const char *h=NULL; // help pointer
    char b[32]; // help array
    bool exoutputp;
    bool exskipescape;
    int i,j;

    intresult=0;
    switch (*c) {
    case 'e': h="\\"; curpos++;break;
    case '0': // ### TODO Where in Unicode? (space of digit width)
    case '~': // non-breakable-space (resizeable!)
    case ' ':
	h="&nbsp;";curpos++; break;
    case '|': // half-non-breakable-space
    	h = "&#x2002;"; curpos++; break; // EN SPACE (### TODO: what is U+202F (NARROW NO-BREAK SPACE)
    case '^': // quarter-non-breakable-space
    	h = "&#x2005;"; break; // FOUR-PER-EM SPACE
    case '"': SKIPEOL; c--; h=""; break;
    // ### TODO \# like \" but does not ignore the end of line (groff(7))
    case '$':
    {
        c++;
        h = scan_dollar_parameter( c );
        break;
    }
    case 'z':
	c++;
	if (*c=='\\') { c=scan_escape(c+1); c--;h=""; }
	else {
	    b[0]=*c;
	    b[1]='\0';
	    h="";
	}
	break;
    case 'k': c++; if (*c=='(') c+=2;
    case '!':
    case '%':
    case 'a':
    case 'd':
    case 'r':
    case 'u':
    case '\n':
    case '&':
        h=""; break;
    case '(':
       c++;
       i= c[0]*256+c[1];
       c++;
       h = expand_char(i);
       break;
    case '*':
    {
        c++;
        c = scan_named_character( c );
        h = "";
        break;
    }
    case 'f':
	c++;
	if (*c=='\\') {
	    c++;
	    c=scan_escape(c);
	    c--;
	    i=intresult;
	} else 	if (*c != '(')
	    i=*c;
     // ### TODO f[]
	else {
	    c++;
	    i=c[0]*256+c[1];
	    c++;
	}
	if (!skip_escape) h=change_to_font(i); else h="";
	break;
    case 's':
	c++;
	j=0;i=0;
	if (*c=='-') {j= -1; c++;} else if (*c=='+') {j=1; c++;}
	if (*c=='0') c++; else if (*c=='\\') {
	    c++;
	    c=scan_escape(c);
	    i=intresult; if (!j) j=1;
	} else
	    while (isdigit(*c) && (!i || (!j && i<4))) i=i*10+(*c++)-'0';
	if (!j) { j=1; if (i) i=i-10; }
	if (!skip_escape) h=change_to_size(i*j); else h="";
	c--;
	break;
    case 'n':
    {
	c++;
	j=0;
	switch (*c) {
	case '+': j=1; c++; break;
	case '-': j=-1; c++; break;
	default: break;
	}
        QCString name;
	if (*c=='(') {
	    c++;
            name=c[0];
            name+=c[1];
	    c=c+1;
	} else {
            name=c[0];
	}
        if ( name[0] == '.' )
        {
            // Internal read-only variable
            if ( name == ".s" )
                intresult = current_size;
            else if ( name == ".f" )
                intresult = current_font;
            else
                intresult = 0; // Undefined variable
        }
        else
        {
            QMap< QCString, NumberDefinition >::iterator it = s_numberDefinitionMap.find( name );
            if ( it == s_numberDefinitionMap.end() )
            {
                intresult = 0; // Undefined variable
            }
            else
            {
                (*it).m_value += j * (*it).m_increment;
                intresult = (*it).m_value;
            }
        }
	h="";
	break;
    }
    case 'w':
	c++;
	i=*c;
	c++;
	exoutputp=output_possible;
	exskipescape=skip_escape;
	output_possible=false;
	skip_escape=true;
	j=0;
	while (*c!=i) {
	    j++;
	    if (*c==escapesym) c=scan_escape(c+1); else c++;
	}
	output_possible=exoutputp;
	skip_escape=exskipescape;
	intresult=j;
	break;
    case 'l': h="<HR>"; curpos=0;
    case 'b':
    case 'v':
    case 'x':
    case 'o':
    case 'L':
    case 'h':
	c++;
	i=*c;
	c++;
	exoutputp=output_possible;
	exskipescape=skip_escape;
	output_possible=0;
	skip_escape=true;
	while (*c != i)
	    if (*c==escapesym) c=scan_escape(c+1);
	    else c++;
	output_possible=exoutputp;
	skip_escape=exskipescape;
	break;
    case 'c': no_newline_output=1; break;
    case '{': newline_for_fun++; h="";break; // Start conditional block
    case '}': if (newline_for_fun) newline_for_fun--; h="";break; // End conditional block
    case 'p': h="<BR>\n";curpos=0; break;
    case 't': h="\t";curpos=(curpos+8)&0xfff8; break;
    case '<': h="&lt;";curpos++; break;
    case '>': h="&gt;";curpos++; break;
    case '\\': if (single_escape) { c--; break;}
    case 'N':
	if (*++c) c++; // c += 2
        if (sscanf(c, "%d", &i) != 1)
		break;
	c+=sprintf(b, "%d", i); // Skip over number
	switch(i) {
		case 8: h="\t";curpos=(curpos+8)&0xfff8; break;
		case 34: h="&quot;"; curpos++; break;
		default: b[0]=i; b[1]=0; h=b; curpos++; break;
	}
	break;
     case '\'': h="&acute;";curpos++; break; // groff(7) ### TODO verify
     case '`': h="&grave;";curpos++; break; // groff(7)
     case '-': h="-";curpos++; break; // groff(7)
     case '.': h=".";curpos++; break; // groff(7)
     default: b[0]=*c; b[1]=0; h=b; curpos++; break;
    }
    c++;
    if (!skip_escape) out_html(h);
    return c;
}

class TABLEROW;

class TABLEITEM {
public:
    TABLEITEM(TABLEROW *row);
    ~TABLEITEM() {
        delete [] contents;
    }
    void setContents(const char *_contents) {
        delete [] contents;
        contents = qstrdup(_contents);
    }
    const char *getContents() const { return contents; }

    void init() {
        delete [] contents;
        contents = 0;
        size = 0;
        align = 0;
        valign = 0;
        colspan = 1;
        rowspan = 1;
        font = 0;
        vleft = 0;
        vright = 0;
        space = 0;
        width = 0;
    }

    void copyLayout(const TABLEITEM *orig) {
        size = orig->size;
        align = orig->align;
        valign = orig->valign;
        colspan = orig->colspan;
        rowspan = orig->rowspan;
        font = orig->font;
        vleft = orig->vleft;
        vright = orig->vright;
        space = orig->space;
        width = orig->width;
    }

public:
    int size,align,valign,colspan,rowspan,font,vleft,vright,space,width;

private:
    char *contents;
    TABLEROW *_parent;
};

class TABLEROW {
    char *test;
public:
    TABLEROW() {
        test = new char;
        items.setAutoDelete(true);
        prev = 0; next = 0;
    }
    ~TABLEROW() {
        delete test;

    }
    int length() const { return items.count(); }
    bool has(int index) {
        return (index >= 0) && (index < (int)items.count());
    }
    TABLEITEM &at(int index) {
        return *items.at(index);
    }

    TABLEROW *copyLayout() const;

    void addItem(TABLEITEM *item) {
        items.append(item);
    }
    TABLEROW *prev, *next;

private:
    QPtrList<TABLEITEM> items;
};

TABLEITEM::TABLEITEM(TABLEROW *row) : contents(0), _parent(row) {
     init();
     _parent->addItem(this);
}

TABLEROW *TABLEROW::copyLayout() const {
    TABLEROW *newrow = new TABLEROW();

    QPtrListIterator<TABLEITEM> it(items);
    for ( ; it.current(); ++it) {
        TABLEITEM *newitem = new TABLEITEM(newrow);
        newitem->copyLayout(it.current());
    }
    return newrow;
}

static const char *tableopt[]= { "center", "expand", "box", "allbox",
                                 "doublebox", "tab", "linesize",
                                 "delim", NULL };
static int tableoptl[] = { 6,6,3,6,9,3,8,5,0};


static void clear_table(TABLEROW *table)
{
    TABLEROW *tr1,*tr2;

    tr1=table;
    while (tr1->prev) tr1=tr1->prev;
    while (tr1) {
	tr2=tr1;
	tr1=tr1->next;
	delete tr2;
    }
}

static char *scan_expression(char *c, int *result);

static char *scan_format(char *c, TABLEROW **result, int *maxcol)
{
    TABLEROW *layout, *currow;
    TABLEITEM *curfield;
    int i,j;
    if (*result) {
	clear_table(*result);
    }
    layout= currow=new TABLEROW();
    curfield=new TABLEITEM(currow);
    while (*c && *c!='.') {
	switch (*c) {
	case 'C': case 'c': case 'N': case 'n':
	case 'R': case 'r': case 'A': case 'a':
	case 'L': case 'l': case 'S': case 's':
	case '^': case '_':
	    if (curfield->align)
		curfield=new TABLEITEM(currow);
	    curfield->align=toupper(*c);
	    c++;
	    break;
	case 'i': case 'I': case 'B': case 'b':
	    curfield->font = toupper(*c);
	    c++;
	    break;
	case 'f': case 'F':
	    c++;
	    curfield->font = toupper(*c);
	    c++;
	    if (!isspace(*c) && *c!='.') c++;
	    break;
	case 't': case 'T': curfield->valign='t'; c++; break;
	case 'p': case 'P':
	    c++;
	    i=j=0;
	    if (*c=='+') { j=1; c++; }
	    if (*c=='-') { j=-1; c++; }
	    while (isdigit(*c)) i=i*10+(*c++)-'0';
	    if (j) curfield->size= i*j; else curfield->size=j-10;
	    break;
	case 'v': case 'V':
	case 'w': case 'W':
	    c=scan_expression(c+2,&curfield->width);
	    break;
	case '|':
	    if (curfield->align) curfield->vleft++;
	    else curfield->vright++;
	    c++;
	    break;
	case 'e': case 'E':
	    c++;
	    break;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	    i=0;
	    while (isdigit(*c)) i=i*10+(*c++)-'0';
	    curfield->space=i;
	    break;
	case ',': case '\n':
	    currow->next=new TABLEROW();
	    currow->next->prev=currow;
	    currow=currow->next;
	    currow->next=NULL;
	    curfield=new TABLEITEM(currow);
	    c++;
	    break;
	default:
	    c++;
	    break;
	}
    }
    if (*c=='.') while (*c++!='\n');
    *maxcol=0;
    currow=layout;
    while (currow) {
	i=currow->length();
	if (i>*maxcol) *maxcol=i;
	currow=currow->next;
    }
    *result=layout;
    return c;
}

static TABLEROW *next_row(TABLEROW *tr)
{
    if (tr->next) {
	tr=tr->next;
	if (!tr->next)
            return next_row(tr);
        return tr;
    } else {
	tr->next = tr->copyLayout();
        tr->next->prev = tr;
	return tr->next;
    }
}

static char itemreset[20]="\\fR\\s0";

#define FORWARDCUR  do { curfield++; } while (currow->has(curfield) &&  currow->at(curfield).align=='S');

static char *scan_table(char *c)
{
    char *h;
    char *g;
    int center=0, expand=0, box=0, border=0, linesize=1;
    int i,j,maxcol=0, finished=0;
    int oldfont, oldsize,oldfillout;
    char itemsep='\t';
    TABLEROW *layout=NULL, *currow;
    int curfield = -1;
    while (*c++!='\n');
    h=c;
    if (*h=='.') return c-1;
    oldfont=current_font;
    oldsize=current_size;
    oldfillout=fillout;
    out_html(change_to_font(0));
    out_html(change_to_size(0));
    if (!fillout) {
	fillout=1;
	out_html("</PRE>");
    }
    while (*h && *h!='\n') h++;
    if (h[-1]==';') {
	/* scan table options */
	while (c<h) {
	    while (isspace(*c)) c++;
	    for (i=0; tableopt[i] && qstrncmp(tableopt[i],c,tableoptl[i]);i++);
	    c=c+tableoptl[i];
	    switch (i) {
	    case 0: center=1; break;
	    case 1: expand=1; break;
	    case 2: box=1; break;
	    case 3: border=1; break;
	    case 4: box=2; break;
	    case 5: while (*c++!='('); itemsep=*c++; break;
	    case 6: while (*c++!='('); linesize=0;
		while (isdigit(*c)) linesize=linesize*10+(*c++)-'0';
		break;
	    case 7: while (*c!=')') c++;
	    default: break;
	    }
	    c++;
	}
	c=h+1;
    }
    /* scan layout */
    c=scan_format(c,&layout, &maxcol);
//    currow=layout;
    currow=next_row(layout);
    curfield=0;
    i=0;
    while (!finished && *c) {
	/* search item */
	h=c;
	if ((*c=='_' || *c=='=') && (c[1]==itemsep || c[1]=='\n')) {
	    if (c[-1]=='\n' && c[1]=='\n') {
		if (currow->prev) {
		    currow->prev->next=new TABLEROW();
		    currow->prev->next->next=currow;
		    currow->prev->next->prev=currow->prev;
		    currow->prev=currow->prev->next;
		} else {
		    currow->prev=layout=new TABLEROW();
		    currow->prev->prev=NULL;
		    currow->prev->next=currow;
		}
		TABLEITEM *newitem = new TABLEITEM(currow->prev);
		newitem->align=*c;
		newitem->colspan=maxcol;
		curfield=0;
		c=c+2;
	    } else {
		if (currow->has(curfield)) {
		    currow->at(curfield).align=*c;
                    FORWARDCUR;
		}
		if (c[1]=='\n') {
		    currow=next_row(currow);
		    curfield=0;
		}
		c=c+2;
	    }
	} else if (*c=='T' && c[1]=='{') {
	    h=c+2;
	    c=strstr(h,"\nT}");
	    c++;
	    *c='\0';
	    g=NULL;
	    scan_troff(h,0,&g);
	    scan_troff(itemreset, 0, &g);
	    *c='T';
	    c+=3;
	    if (currow->has(curfield)) {
		currow->at(curfield).setContents(g);
                FORWARDCUR;
	    }
            delete [] g;

	    if (c[-1]=='\n') {
		currow=next_row(currow);
		curfield=0;
	    }
	} else if (*c=='.' && c[1]=='T' && c[2]=='&' && c[-1]=='\n') {
	    TABLEROW *hr;
	    while (*c++!='\n');
	    hr=currow;
	    currow=currow->prev;
	    hr->prev=NULL;
	    c=scan_format(c,&hr, &i);
	    hr->prev=currow;
	    currow->next=hr;
	    currow=hr;
	    next_row(currow);
	    curfield=0;
	} else if (*c=='.' && c[1]=='T' && c[2]=='E' && c[-1]=='\n') {
	    finished=1;
	    while (*c++!='\n');
	    if (currow->prev)
		currow->prev->next=NULL;
	    currow->prev=NULL;
            clear_table(currow);
            currow = 0;
        } else if (*c=='.' && c[-1]=='\n' && !isdigit(c[1])) {
	    /* skip troff request inside table (usually only .sp ) */
	    while (*c++!='\n');
	} else {
	    h=c;
	    while (*c && (*c!=itemsep || c[-1]=='\\') &&
		   (*c!='\n' || c[-1]=='\\')) c++;
	    i=0;
	    if (*c==itemsep) {i=1; *c='\n'; }
	    if (h[0]=='\\' && h[2]=='\n' &&
		(h[1]=='_' || h[1]=='^')) {
		if (currow->has(curfield)) {
		    currow->at(curfield).align=h[1];
                    FORWARDCUR;
		}
		h=h+3;
	    } else {
		g=NULL;
		h=scan_troff(h,1,&g);
		scan_troff(itemreset,0, &g);
		if (currow->has(curfield)) {
		    currow->at(curfield).setContents(g);
                    FORWARDCUR;
		}
                delete [] g;
	    }
	    if (i) *c=itemsep;
	    c=h;
	    if (c[-1]=='\n') {
		currow=next_row(currow);
		curfield=0;
	    }
	}
    }
    /* calculate colspan and rowspan */
    currow=layout;
    while (currow->next) currow=currow->next;
    while (currow) {
        int ti = 0, ti1 = 0, ti2 = -1;
        TABLEROW *prev = currow->prev;
        if (!prev)
            break;

	while (prev->has(ti1)) {
	    if (currow->has(ti))
                switch (currow->at(ti).align) {
                    case 'S':
                        if (currow->has(ti2)) {
                            currow->at(ti2).colspan++;
                            if (currow->at(ti2).rowspan<prev->at(ti1).rowspan)
                                currow->at(ti2).rowspan=prev->at(ti1).rowspan;
                        }
                        break;
                    case '^':
                        if (prev->has(ti1)) prev->at(ti1).rowspan++;
                    default:
                        if (ti2 < 0) ti2=ti;
                        else {
                            do {
                                ti2++;
                            } while (currow->has(ti2) && currow->at(ti2).align=='S');
                        }
                        break;
                }
            ti++;
            if (ti1 >= 0) ti1++;
        }
        currow=currow->prev;
    }
    /* produce html output */
    if (center) out_html("<CENTER>");
    if (box==2) out_html("<TABLE BORDER><TR><TD>");
    out_html("<TABLE");
    if (box || border) {
        out_html(" BORDER");
        if (!border) out_html("><TR><TD><TABLE");
        if (expand) out_html(" WIDTH=\"100%\"");
    }
    out_html(">\n");
    currow=layout;
    while (currow) {
        j=0;
        out_html("<TR VALIGN=top>");
	curfield=0;
	while (currow->has(curfield)) {
	    if (currow->at(curfield).align!='S' && currow->at(curfield).align!='^') {
		out_html("<TD");
		switch (currow->at(curfield).align) {
                    case 'N':
                        currow->at(curfield).space+=4;
                    case 'R':
                        out_html(" ALIGN=right");
                        break;
                    case 'C':
                        out_html(" ALIGN=center");
                    default:
                        break;
		}
		if (!currow->at(curfield).valign && currow->at(curfield).rowspan>1)
		    out_html(" VALIGN=center");
		if (currow->at(curfield).colspan>1) {
		    char buf[5];
		    out_html(" COLSPAN=");
		    sprintf(buf, "%i", currow->at(curfield).colspan);
		    out_html(buf);
		}
		if (currow->at(curfield).rowspan>1) {
		    char buf[5];
		    out_html(" ROWSPAN=");
		    sprintf(buf, "%i", currow->at(curfield).rowspan);
		    out_html(buf);
		}
		j=j+currow->at(curfield).colspan;
		out_html(">");
		if (currow->at(curfield).size) out_html(change_to_size(currow->at(curfield).size));
		if (currow->at(curfield).font) out_html(change_to_font(currow->at(curfield).font));
		switch (currow->at(curfield).align) {
		case '=': out_html("<HR><HR>"); break;
		case '_': out_html("<HR>"); break;
		default:
		    out_html(currow->at(curfield).getContents());
		    break;
		}
		if (currow->at(curfield).space)
		    for (i=0; i<currow->at(curfield).space;i++) out_html("&nbsp;");
		if (currow->at(curfield).font) out_html(change_to_font(0));
		if (currow->at(curfield).size) out_html(change_to_size(0));
		if (j>=maxcol && currow->at(curfield).align>'@' && currow->at(curfield).align!='_')
		    out_html("<BR>");
		out_html("</TD>");
	    }
	    curfield++;
	}
	out_html("</TR>\n");
	currow=currow->next;
    }

    clear_table(layout);

    if (box && !border) out_html("</TABLE>");
    out_html("</TABLE>");
    if (box==2) out_html("</TABLE>");
    if (center) out_html("</CENTER>\n");
    else out_html("\n");
    if (!oldfillout) out_html("<PRE>");
    fillout=oldfillout;
    out_html(change_to_size(oldsize));
    out_html(change_to_font(oldfont));
    return c;
}

static char *scan_expression(char *c, int *result)
{
    int value=0,value2,sign=1,opex=0;
    char oper='c';

    if (*c=='!') {
	c=scan_expression(c+1, &value);
	value= (!value);
    } else if (*c=='n') {
	c++;
	value=s_nroff;
    } else if (*c=='t') {
	c++;
	value=1-s_nroff;
    } else if (*c=='\'' || *c=='"' || *c<' ' || (*c=='\\' && c[1]=='(')) {
	/* ?string1?string2?
	** test if string1 equals string2.
	*/
	char *st1=NULL, *st2=NULL, *h;
	char *tcmp=NULL;
	char sep;
	sep=*c;
	if (sep=='\\') {
	    tcmp=c;
	    c=c+3;
	}
	c++;
	h=c;
	while (*c!= sep && (!tcmp || qstrncmp(c,tcmp,4))) c++;
	*c='\n';
	scan_troff(h, 1, &st1);
	*c=sep;
	if (tcmp) c=c+3;
	c++;
	h=c;
	while (*c!=sep && (!tcmp || qstrncmp(c,tcmp,4))) c++;
	*c='\n';
	scan_troff(h,1,&st2);
	*c=sep;
	if (!st1 && !st2) value=1;
	else if (!st1 || !st2) value=0;
	else value=(!qstrcmp(st1, st2));
	delete [] st1;
        delete [] st2;
	if (tcmp) c=c+3;
	c++;
    } else {
	while (*c && !isspace(*c) && *c!=')' && opex >= 0) {
	    opex=0;
	    switch (*c) {
	    case '(':
		c=scan_expression(c+1, &value2);
		value2=sign*value2;
		opex=1;
		break;
	    case '.':
	    case '0': case '1':
	    case '2': case '3':
	    case '4': case '5':
	    case '6': case '7':
	    case '8': case '9': {
		int num=0,denum=1;
		value2=0;
		while (isdigit(*c)) value2=value2*10+((*c++)-'0');
		if (*c=='.' && isdigit(c[1])) {
		    c++;
		    while (isdigit(*c)) {
			num=num*10+((*c++)-'0');
			denum=denum*10;
		    }
		}
		if (isalpha(*c)) {
		    /* scale indicator */
		    switch (*c) {
		    case 'i': /* inch -> 10pt */
			value2=value2*10+(num*10+denum/2)/denum;
			num=0;
			break;
		    default:
			break;
		    }
		    c++;
		}
		value2=value2+(num+denum/2)/denum;
		value2=sign*value2;
		opex=1;
                if (*c=='.')
                    opex = -1;

	    }
            break;
	    case '\\':
		c=scan_escape(c+1);
		value2=intresult*sign;
		if (isalpha(*c)) c++; /* scale indicator */
		opex=1;
		break;
	    case '-':
		if (oper) { sign=-1; c++; break; }
	    case '>':
	    case '<':
	    case '+':
	    case '/':
	    case '*':
	    case '%':
	    case '&':
	    case '=':
	    case ':':
		if (c[1]=='=') oper=(*c++) +16; else oper=*c;
		c++;
		break;
	    default: c++; break;
	    }
	    if (opex > 0) {
		sign=1;
		switch (oper) {
		case 'c': value=value2; break;
		case '-': value=value-value2; break;
		case '+': value=value+value2; break;
		case '*': value=value*value2; break;
		case '/': if (value2) value=value/value2; break;
		case '%': if (value2) value=value%value2; break;
		case '<': value=(value<value2); break;
		case '>': value=(value>value2); break;
		case '>'+16: value=(value>=value2); break;
		case '<'+16: value=(value<=value2); break;
		case '=': case '='+16: value=(value==value2); break;
		case '&': value = (value && value2); break;
		case ':': value = (value || value2); break;
		default:
                    {
#ifdef SIMPLE_MAN2HTML
                        fprintf(stderr, "man2html: unknown operator %c.\n", oper);
#else
                        kdDebug(7107) << "Unknown operator " << char(oper) << endl;
#endif
                    }
		}
		oper=0;
	    }
	}
	if (*c==')') c++;
    }
    *result=value;
    return c;
}

static void trans_char(char *c, char s, char t)
{
    char *sl=c;
    int slash=0;
    while (*sl!='\n' || slash) {
	if (!slash) {
	    if (*sl==escapesym)
		slash=1;
	    else if (*sl==s)
		*sl=t;
	} else slash=0;
	sl++;
    }
}

// 2004-10-19, patched by Waldo Bastian <bastian@kde.org>:
// Fix handling of lines like:
// .TH FIND 1L \" -*- nroff -*-
// Where \" indicates the start of comment.
// 
// The problem is the \" handling in fill_words(), the return value
// indicates the end of the word as well as the end of the line, which makes it
// basically impossible to express that the end of the last word is not the end of
// the line.
// 
// I have corrected that by adding an extra parameter 'next_line' that returns a
// pointer to the next line, while the function itself returns a pointer to the end
// of the last word.
static char *fill_words(char *c, char *words[], int *n, bool newline, char **next_line)
{
    char *sl=c;
    int slash=0;
    int skipspace=0;
    *n=0;
    words[*n]=sl;
    while (*sl && (*sl!='\n' || slash)) {
	if (!slash) {
	    if (*sl=='"') {
                if (skipspace && (*(sl+1)=='"'))
                    *sl++ = '\a';
                else {
        	    *sl='\a';
	            skipspace=!skipspace;
                }
	    } else if (*sl==escapesym) {
		slash=1;
                if (sl[1]=='\n')
                    *sl='\a';
	    } else if ((*sl==' ' || *sl=='\t') && !skipspace) {
		if (newline) *sl='\n';
		if (words[*n]!=sl) (*n)++;
		words[*n]=sl+1;
	    }
	} else {
	    if (*sl=='"') {
		sl--;
		if (newline) *sl='\n';
		if (words[*n]!=sl) (*n)++;
		if (next_line)
		{
		    char *eow = sl;
		    sl++;
		    while (*sl && *sl !='\n') sl++;
		    *next_line = sl;
		    return eow;
		}
		return sl;
	    }
	    slash=0;
	}
	sl++;
    }
    if (sl!=words[*n]) (*n)++;
    if (next_line) *next_line = sl+1;
    return sl;
}

static const char *abbrev_list[] = {
    "GSBG", "Getting Started ",
    "SUBG", "Customizing SunOS",
    "SHBG", "Basic Troubleshooting",
    "SVBG", "SunView User's Guide",
    "MMBG", "Mail and Messages",
    "DMBG", "Doing More with SunOS",
    "UNBG", "Using the Network",
    "GDBG", "Games, Demos &amp; Other Pursuits",
    "CHANGE", "SunOS 4.1 Release Manual",
    "INSTALL", "Installing SunOS 4.1",
    "ADMIN", "System and Network Administration",
    "SECUR", "Security Features Guide",
    "PROM", "PROM User's Manual",
    "DIAG", "Sun System Diagnostics",
    "SUNDIAG", "Sundiag User's Guide",
    "MANPAGES", "SunOS Reference Manual",
    "REFMAN", "SunOS Reference Manual",
    "SSI", "Sun System Introduction",
    "SSO", "System Services Overview",
    "TEXT", "Editing Text Files",
    "DOCS", "Formatting Documents",
    "TROFF", "Using <B>nroff</B> and <B>troff</B>",
    "INDEX", "Global Index",
    "CPG", "C Programmer's Guide",
    "CREF", "C Reference Manual",
    "ASSY", "Assembly Language Reference",
    "PUL", "Programming Utilities and Libraries",
    "DEBUG", "Debugging Tools",
    "NETP", "Network Programming",
    "DRIVER", "Writing Device Drivers",
    "STREAMS", "STREAMS Programming",
    "SBDK", "SBus Developer's Kit",
    "WDDS", "Writing Device Drivers for the SBus",
    "FPOINT", "Floating-Point Programmer's Guide",
    "SVPG", "SunView 1 Programmer's Guide",
    "SVSPG", "SunView 1 System Programmer's Guide",
    "PIXRCT", "Pixrect Reference Manual",
    "CGI", "SunCGI Reference Manual",
    "CORE", "SunCore Reference Manual",
    "4ASSY", "Sun-4 Assembly Language Reference",
    "SARCH", "<FONT SIZE=\"-1\">SPARC</FONT> Architecture Manual",
    "KR", "The C Programming Language",
    NULL, NULL };

static const char *lookup_abbrev(char *c)
{
    int i=0;

    if (!c) return "";
    while (abbrev_list[i] && qstrcmp(c,abbrev_list[i])) i=i+2;
    if (abbrev_list[i]) return abbrev_list[i+1];
    else return c;
}

static const char *section_list[] = {
#ifdef Q_OS_SOLARIS
    // for Solaris
    "1", "User Commands",
    "1B", "SunOS/BSD Compatibility Package Commands",
    "1b", "SunOS/BSD Compatibility Package Commands",
    "1C", "Communication Commands ",
    "1c", "Communication Commands",
    "1F", "FMLI Commands ",
    "1f", "FMLI Commands",
    "1G", "Graphics and CAD Commands ",
    "1g", "Graphics and CAD Commands ",
    "1M", "Maintenance Commands",
    "1m", "Maintenance Commands",
    "1S", "SunOS Specific Commands",
    "1s", "SunOS Specific Commands",
    "2", "System Calls",
    "3", "C Library Functions",
    "3B", "SunOS/BSD Compatibility Library Functions",
    "3b", "SunOS/BSD Compatibility Library Functions",
    "3C", "C Library Functions",
    "3c", "C Library Functions",
    "3E", "C Library Functions",
    "3e", "C Library Functions",
    "3F", "Fortran Library Routines",
    "3f", "Fortran Library Routines",
    "3G", "C Library Functions",
    "3g", "C Library Functions",
    "3I", "Wide Character Functions",
    "3i", "Wide Character Functions",
    "3K", "Kernel VM Library Functions",
    "3k", "Kernel VM Library Functions",
    "3L", "Lightweight Processes Library",
    "3l", "Lightweight Processes Library",
    "3M", "Mathematical Library",
    "3m", "Mathematical Library",
    "3N", "Network Functions",
    "3n", "Network Functions",
    "3R", "Realtime Library",
    "3r", "Realtime Library",
    "3S", "Standard I/O Functions",
    "3s", "Standard I/O Functions",
    "3T", "Threads Library",
    "3t", "Threads Library",
    "3W", "C Library Functions",
    "3w", "C Library Functions",
    "3X", "Miscellaneous Library Functions",
    "3x", "Miscellaneous Library Functions",
    "4", "File Formats",
    "4B", "SunOS/BSD Compatibility Package File Formats",
    "4b", "SunOS/BSD Compatibility Package File Formats",
    "5", "Headers, Tables, and Macros",
    "6", "Games and Demos",
    "7", "Special Files",
    "7B", "SunOS/BSD Compatibility Special Files",
    "7b", "SunOS/BSD Compatibility Special Files",
    "8", "Maintenance Procedures",
    "8C", "Maintenance Procedures",
    "8c", "Maintenance Procedures",
    "8S", "Maintenance Procedures",
    "8s", "Maintenance Procedures",
    "9", "DDI and DKI",
    "9E", "DDI and DKI Driver Entry Points",
    "9e", "DDI and DKI Driver Entry Points",
    "9F", "DDI and DKI Kernel Functions",
    "9f", "DDI and DKI Kernel Functions",
    "9S", "DDI and DKI Data Structures",
    "9s", "DDI and DKI Data Structures",
    "L", "Local Commands",
#else
    // Other OS
    "1", "User Commands ",
    "1C", "User Commands",
    "1G", "User Commands",
    "1S", "User Commands",
    "1V", "User Commands ",
    "2", "System Calls",
    "2V", "System Calls",
    "3", "C Library Functions",
    "3C", "Compatibility Functions",
    "3F", "Fortran Library Routines",
    "3K", "Kernel VM Library Functions",
    "3L", "Lightweight Processes Library",
    "3M", "Mathematical Library",
    "3N", "Network Functions",
    "3R", "RPC Services Library",
    "3S", "Standard I/O Functions",
    "3V", "C Library Functions",
    "3X", "Miscellaneous Library Functions",
    "4", "Devices and Network Interfaces",
    "4F", "Protocol Families",
    "4I", "Devices and Network Interfaces",
    "4M", "Devices and Network Interfaces",
    "4N", "Devices and Network Interfaces",
    "4P", "Protocols",
    "4S", "Devices and Network Interfaces",
    "4V", "Devices and Network Interfaces",
    "5", "File Formats",
    "5V", "File Formats",
    "6", "Games and Demos",
    "7", "Environments, Tables, and Troff Macros",
    "7V", "Environments, Tables, and Troff Macros",
    "8", "Maintenance Commands",
    "8C", "Maintenance Commands",
    "8S", "Maintenance Commands",
    "8V", "Maintenance Commands",
    "L", "Local Commands",
#endif
    // The defaults
    NULL, "Misc. Reference Manual Pages",
    NULL, NULL
};

static const char *section_name(char *c)
{
    int i=0;

    if (!c) return "";
    while (section_list[i] && qstrcmp(c,section_list[i])) i=i+2;
    if (section_list[i+1]) return section_list[i+1];
    else return c;
}

static char *skip_till_newline(char *c)
{
    int lvl=0;

    while (*c && (*c!='\n' || lvl>0)) {
	if (*c=='\\') {
	    c++;
	    if (*c=='}') lvl--; else if (*c=='{') lvl++;
	}
	c++;
    }
    if (*c) c++;
    if (lvl<0 && newline_for_fun) {
	newline_for_fun = newline_for_fun+lvl;
	if (newline_for_fun<0) newline_for_fun=0;
    }
    return c;
}

// Some known missing requests from man(7):
// - see "safe subset"

// Some known missing requests from mdoc(7):
// - start or end of quotings

// Some of the requests are from mdoc.
// On Linux see the man pages mdoc(7), mdoc.samples(7) and groff_mdoc(7)
// See also the online man pages of FreeBSD: mdoc(7)

#define REQ_UNKNOWN   -1
#define REQ_ab         0
#define REQ_di         1
#define REQ_ds         2
#define REQ_as         3
#define REQ_br         4
#define REQ_c2         5
#define REQ_cc         6
#define REQ_ce         7
#define REQ_ec         8
#define REQ_eo         9
#define REQ_ex        10
#define REQ_fc        11
#define REQ_fi        12
#define REQ_ft        13
#define REQ_el        14
#define REQ_ie        15
#define REQ_if        16
#define REQ_ig        17
#define REQ_nf        18
#define REQ_ps        19
#define REQ_sp        20
#define REQ_so        21
#define REQ_ta        22
#define REQ_ti        23
#define REQ_tm        24
#define REQ_B         25
#define REQ_I         26
#define REQ_Fd        27
#define REQ_Fn        28
#define REQ_Fo        29
#define REQ_Fc        30
#define REQ_OP        31
#define REQ_Ft        32
#define REQ_Fa        33
#define REQ_BR        34
#define REQ_BI        35
#define REQ_IB        36
#define REQ_IR        37
#define REQ_RB        38
#define REQ_RI        39
#define REQ_DT        40
#define REQ_IP        41
#define REQ_TP        42
#define REQ_IX        43
#define REQ_P         44
#define REQ_LP        45
#define REQ_PP        46
#define REQ_HP        47
#define REQ_PD        48
#define REQ_Rs        49
#define REQ_RS        50
#define REQ_Re        51
#define REQ_RE        52
#define REQ_SB        53
#define REQ_SM        54
#define REQ_Ss        55
#define REQ_SS        56
#define REQ_Sh        57
#define REQ_SH        58 // man(7) "Sub Header"
#define REQ_Sx        59
#define REQ_TS        60
#define REQ_Dt        61
#define REQ_TH        62
#define REQ_TX        63
#define REQ_rm        64
#define REQ_rn        65
#define REQ_nx        66
#define REQ_in        67
#define REQ_nr        68
#define REQ_am        69
#define REQ_de        70
#define REQ_Bl        71 // mdoc(7) "Begin List"
#define REQ_El        72 // mdoc(7) "End List"
#define REQ_It        73 // mdoc(7) "ITem"
#define REQ_Bk        74
#define REQ_Ek        75
#define REQ_Dd        76
#define REQ_Os        77 // mdoc(7)
#define REQ_Bt        78
#define REQ_At        79 // mdoc(7) "AT&t" (not parsable, not callable)
#define REQ_Fx        80 // mdoc(7) "Freebsd" (not parsable, not callable)
#define REQ_Nx        81
#define REQ_Ox        82
#define REQ_Bx        83 // mdoc(7) "Bsd"
#define REQ_Ux        84 // mdoc(7) "UniX"
#define REQ_Dl        85
#define REQ_Bd        86
#define REQ_Ed        87
#define REQ_Be        88
#define REQ_Xr        89 // mdoc(7) "eXternal Reference"
#define REQ_Fl        90 // mdoc(7) "FLag"
#define REQ_Pa        91
#define REQ_Pf        92
#define REQ_Pp        93
#define REQ_Dq        94 // mdoc(7) "Double Quote"
#define REQ_Op        95
#define REQ_Oo        96
#define REQ_Oc        97
#define REQ_Pq        98 // mdoc(7) "Parenthese Quote"
#define REQ_Ql        99
#define REQ_Sq       100 // mdoc(7) "Single Quote"
#define REQ_Ar       101
#define REQ_Ad       102
#define REQ_Em       103 // mdoc(7) "EMphasis"
#define REQ_Va       104
#define REQ_Xc       105
#define REQ_Nd       106
#define REQ_Nm       107
#define REQ_Cd       108
#define REQ_Cm       109
#define REQ_Ic       110
#define REQ_Ms       111
#define REQ_Or       112
#define REQ_Sy       113
#define REQ_Dv       114
#define REQ_Ev       115
#define REQ_Fr       116
#define REQ_Li       117
#define REQ_No       118
#define REQ_Ns       119
#define REQ_Tn       120
#define REQ_nN       121
#define REQ_perc_A   122
#define REQ_perc_D   123
#define REQ_perc_N   124
#define REQ_perc_O   125
#define REQ_perc_P   126
#define REQ_perc_Q   127
#define REQ_perc_V   128
#define REQ_perc_B   129
#define REQ_perc_J   130
#define REQ_perc_R   131
#define REQ_perc_T   132
#define REQ_An       133 // mdoc(7) "Author Name"
#define REQ_Aq       134 // mdoc(7) "Angle bracket Quote"
#define REQ_Bq       135 // mdoc(7) "Bracket Quote"
#define REQ_Qq       136 // mdoc(7)  "straight double Quote"
#define REQ_UR       137 // man(7) "URl"
#define REQ_UE       138 // man(7) "Url End"
#define REQ_UN       139 // man(7) "Url Name" (a.k.a. anchors)
#define REQ_troff    140 // groff(7) "TROFF mode"
#define REQ_nroff    141 // groff(7) "NROFF mode"
#define REQ_als      142 // groff(7) "ALias String"

static int get_request(char *req, int len)
{
    static const char *requests[] = {
        "ab", "di", "ds", "as", "br", "c2", "cc", "ce", "ec", "eo", "ex", "fc",
        "fi", "ft", "el", "ie", "if", "ig", "nf", "ps", "sp", "so", "ta", "ti",
        "tm", "B",  "I",  "Fd", "Fn", "Fo", "Fc", "OP", "Ft", "Fa", "BR", "BI",
        "IB", "IR", "RB", "RI", "DT", "IP", "TP", "IX", "P",  "LP", "PP", "HP",
        "PD", "Rs", "RS", "Re", "RE", "SB", "SM", "Ss", "SS", "Sh", "SH", "Sx",
        "TS", "Dt", "TH", "TX", "rm", "rn", "nx", "in", "nr", "am", "de", "Bl",
        "El", "It", "Bk", "Ek", "Dd", "Os", "Bt", "At", "Fx", "Nx", "Ox", "Bx",
        "Ux", "Dl", "Bd", "Ed", "Be", "Xr", "Fl", "Pa", "Pf", "Pp", "Dq", "Op",
        "Oo", "Oc", "Pq", "Ql", "Sq", "Ar", "Ad", "Em", "Va", "Xc", "Nd", "Nm",
        "Cd", "Cm", "Ic", "Ms", "Or", "Sy", "Dv", "Ev", "Fr", "Li", "No", "Ns",
        "Tn", "nN", "%A", "%D", "%N", "%O", "%P", "%Q", "%V", "%B", "%J", "%R",
        "%T", "An", "Aq", "Bq", "Qq", "UR", "UE", "UN", "troff", "nroff", "als",
        0 };
    int r = 0;
    while (requests[r] && qstrncmp(req, requests[r], len)) r++;
    return requests[r] ? r : REQ_UNKNOWN;
}

// &%(#@ c programs !!!
//static int ifelseval=0;
// If/else can be nested!
static QValueStack<int> s_ifelseval;

// Process a (mdoc) request involving quotes
static char* process_quote(char* c, int j, const char* open, const char* close)
{
    trans_char(c,'"','\a');
    c+=j;
    if (*c=='\n') c++; // ### TODO: why? Quote requests cannot be empty!
    out_html(open);
    c=scan_troff_mandoc(c,1,0);
    out_html(close);
    out_html(NEWLINE);
    if (fillout)
      curpos++;
    else
      curpos=0;
    return c;
}

static char *scan_request(char *c)
{
    // mdoc(7) stuff
    static bool mandoc_synopsis=false; /* True if we are in the synopsis section */
    static bool mandoc_command=false;  /* True if this is mdoc(7) page */
    static int mandoc_bd_options; /* Only copes with non-nested Bd's */
    static int function_argument=0; // Number of function argument (.Fo, .Fa, .Fc)
    // man(7) stuff
    static bool ur_ignore=false; // Has .UR a parameter : (for .UE to know if or not to write </a>)

    int i=0;
    bool mode=false;
    char *h=0;
    char *wordlist[MAX_WORDLIST];
    int words;
    char *sl;
    while (*c==' ' || *c=='\t') c++; // Spaces or tabs allowed between control character and request
    if (c[0]=='\n') return c+1;
    if (c[0]==escapesym)
    {
        /* some pages use .\" .\$1 .\} */
	/* .\$1 is too difficult/stuppid */
        if (c[1]=='$')
            c=skip_till_newline(c);
	else
	    c = scan_escape(c+1);
    }
    else
    {
        int j;
        if (c[1]=='\n') j=1; else j=2; // ### TODO: remove, as i is over-written later
        int nlen = 0;
        QCString macroName;
        while (c[nlen] && (c[nlen] != ' ') && (c[nlen] != '\t') && (c[nlen] != '\n') && (c[nlen] != escapesym))
        {
            macroName+=c[nlen];
            nlen++;
        }
        j = nlen;
        while (c[j] && c[j]==' ' || c[j]=='\t') j++;
        /* search macro database of self-defined macros */
        QMap<QCString,StringDefinition>::iterator it=s_stringDefinitionMap.find(macroName);
        if (it!=s_stringDefinitionMap.end())
        {
#ifndef SIMPLE_MAN2HTML
            kdDebug(7107) << "CALLING MACRO: " << macroName << endl;
#endif
            const QCString oldDollarZero = s_dollarZero; // Previous value of $0
            s_dollarZero = macroName;
            sl=fill_words(c+j, wordlist, &words, true, &c);
            *sl='\0';
            for (i=1;i<words; i++) wordlist[i][-1]='\0';
            for (i=0; i<words; i++)
            {
                char *h=NULL;
                if (mandoc_command)
                    scan_troff_mandoc(wordlist[i],1,&h);
                else
                    scan_troff(wordlist[i],1,&h);
                wordlist[i] = qstrdup(h);
                delete [] h;
            }
            for (i=words;i<20; i++) wordlist[i]=NULL;
            if ((*it).m_output.isEmpty())
            {
#ifndef SIMPLE_MAN2HTML
                kdDebug(7107) << "Macro is empty: " << macroName << endl;
#endif
            }
            else
            {
                //kdDebug(7107) << "Macro content is: " << endl << (*it).m_output << endl;
                const uint length = (*it).m_output.length();
                char* work = new char [length+2];
                work[0] = '\n'; // The macro must start after an end of line to allow a request on first line
                qstrncpy(work+1,(*it).m_output.data(),length+1);
                char** oldargument=argument;
                argument=wordlist;
                const int onff=newline_for_fun;
                if (mandoc_command)
                    scan_troff_mandoc(work, 0, NULL);
                else
                    scan_troff(work, 0, NULL);
                delete[] work;
                newline_for_fun=onff;
                argument=oldargument;
            }
            for (i=0; i<words; i++) delete [] wordlist[i];
            *sl='\n';
            s_dollarZero = oldDollarZero;
#ifndef SIMPLE_MAN2HTML
            kdDebug(7107) << "ENDING MACRO: " << macroName << endl;
#endif
        }
        else
        {
#ifndef SIMPLE_MAN2HTML
            kdDebug(7107) << "REQUEST: " << macroName << endl;
#endif
            switch (int request = get_request(c, nlen))
            {
                case REQ_ab: // groff(7) "ABort"
                {
                    h=c+j;
                    while (*h && *h !='\n') h++;
                    *h='\0';
                    if (scaninbuff && buffpos)
                    {
                        buffer[buffpos]='\0';
                        puts(buffer); // ### FIXME we should not write to stdout for a KIO
                    }
#ifdef SIMPLE_MAN2HTML
                    fprintf(stderr, "%s\n", c+j);
#else
                    // ### TODO find a way to disply it to the user
                    kdDebug(7107) << "Aborting: .ab " << (c+j) << endl;
#endif
                    return 0;
                    break;
                }
        	case REQ_An: // mdoc(7) "Author Name"
                {
                    c+=j;
                    c=scan_troff_mandoc(c,1,0);
                    break;
                }
                case REQ_di: // groff(7) "end current DIversion"
                {
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "Start .di" << endl;
#endif
                    c+=j;
                    if (*c=='\n')
                    {
                        ++c;
                        break;
                    }
                    h=c;
                    while (*h && *h!='\n' && (isalnum(*h) || *h=='@')) ++h;
                    if (*h)
                        *h=0;
                    const QCString name=c;
                    c=h+1;
                    while (*c && *c!='\n') c++;
                    c++;
                    h=c;
                    while (*c && qstrncmp(c,".di",3)) while (*c && *c++!='\n');
                    *c='\0';
                    char* result=0;
                    scan_troff(h,0,&result);
                    QMap<QCString,StringDefinition>::iterator it=s_stringDefinitionMap.find(name);
                    if (it==s_stringDefinitionMap.end())
                    {
                        StringDefinition def;
                        def.m_length=0;
                        def.m_output=result;
                        s_stringDefinitionMap.insert(name,def);
                    }
                    else
                    {
                        (*it).m_length=0;
                        (*it).m_output=result;
                    }
                    delete[] result;
                    if (*c) *c='.';
                    while (*c && *c++!='\n');
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "end .di" << endl;
#endif
                    break;
                }
                case REQ_ds: // groff(7) "Define String variable"
                    mode=true;
                case REQ_as: // groff (7) "Append String variable"
                {
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "start .ds/.as" << endl;
#endif
                    int oldcurpos=curpos;
                    c+=j;
                    h=c;
                    while (*h && *h!='\n' && (isalnum(*h) || (*h=='@'))) ++h;
                    if (*h)
                        *h=0;
                    const QCString name(c);
                    c=h+1;
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "Define/append string " << name << endl;
#endif
                    while (*c && isspace(*c)) c++;
                    if (*c && *c=='"') c++;
                    single_escape=true;
                    curpos=0;
                    char* result=0;
                    c=scan_troff(c,1,&result);
                    QMap<QCString,StringDefinition>::iterator it=s_stringDefinitionMap.find(name);
                    if (it==s_stringDefinitionMap.end())
                    {
                        StringDefinition def;
                        def.m_length=curpos;
                        def.m_output=result;
                        s_stringDefinitionMap.insert(name,def);
                    }
                    else
                    {
                        if (mode)
                        {   // .ds Defining String
                            (*it).m_length=curpos;
                            (*it).m_output=result;
                        }
                        else
                        {   // .as Appending String
                            (*it).m_length+=curpos;
                            (*it).m_output+=result;
                        }
                    }
                    delete[] result;
                    single_escape=false;
                    curpos=oldcurpos;
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "end .ds/.as" << endl;
#endif
                    break;
                }
                case REQ_br: // groff(7) "line BReak"
                {
                    if (still_dd) 
                        out_html("<DD>"); // ### VERIFY (does not look like generating good HTML)
                    else
                        out_html("<BR>\n");
                    curpos=0;
                    c=c+j;
                    if (c[0]==escapesym) c=scan_escape(c+1);
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_c2: // groff(7) "reset non-break Control character" (2 means non-break)
                {
                    c=c+j;
                    if (*c!='\n')
                        nobreaksym=*c;
                    else
                        nobreaksym='\'';
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_cc: // groff(7) "reset Control Character"
                {
                    c=c+j;
                    if (*c!='\n')
                        controlsym=*c;
                    else
                        controlsym='.';
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_ce: // groff (7) "CEnter"
                {
                    c=c+j;
                    if (*c=='\n')
                        i=1;
                    else
                    {
                        i=0;
                        while ('0'<=*c && *c<='9')
                        {
                            i=i*10+*c-'0';
                            c++;
                        }
                    }
                    c=skip_till_newline(c);
                    /* center next i lines */
                    if (i>0)
                    {
                        out_html("<CENTER>\n");
                        while (i && *c)
                        {
                            char *line=NULL;
                            c=scan_troff(c,1, &line);
                            if (line && qstrncmp(line, "<BR>", 4))
                            {
                                out_html(line);
                                out_html("<BR>\n");
                                delete [] line;
                                i--;
                            }
                        }
                        out_html("</CENTER>\n");
                        curpos=0;
                    }
                    break;
                }
                case REQ_ec: // groff(7) "reset Escape Character"
                {
                    c=c+j;
                    if (*c!='\n')
                        escapesym=*c;
                    else
                        escapesym='\\';
                    break;
                    c=skip_till_newline(c);
                }
                case REQ_eo: // groff(7) "turn Escape character Off"
                {
                    escapesym='\0';
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_ex: // groff(7) "EXit"
                {
                    return 0;
                    break;
                }
                case REQ_fc: // groff(7) "set Field and pad Character"
                {
                    c=c+j;
                    if  (*c=='\n')
                        fieldsym=padsym='\0';
                    else
                    {
                        fieldsym=c[0];
                        padsym=c[1];
                    }
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_fi: // groff(7) "FIll"
                {
                    if (!fillout)
                    {
                        out_html(change_to_font(0));
                        out_html(change_to_size('0'));
                        out_html("</PRE>\n");
                    }
                    curpos=0;
                    fillout=1;
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_ft: // groff(7) "previous FonT"
                {
                    c=c+j;
                    if (*c=='\n')
                        out_html(change_to_font(0));
                    else
                    {
                        if (*c==escapesym)
                        {
                            int fn;
                            c=scan_expression(c, &fn);
                            c--;
                            out_html(change_to_font(fn));
                        }
                        else
                        {
                            out_html(change_to_font(*c));
                            c++;
                        }
                    }
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_el: // groff(7) "ELse"
                {
                    int ifelseval = s_ifelseval.pop();
                    /* .el anything : else part of if else */
                    if (ifelseval)
                    {
                        c=c+j;
                        c[-1]='\n';
                        c=scan_troff(c,1,NULL);
                    }
                    else
                        c=skip_till_newline(c+j);
                    break;
                }
                case REQ_ie: // groff(7) "If with Else"
                /* .ie c anything : then part of if else */
                case REQ_if: // groff(7) "IF"
                {
                    /* .if c anything
                     * .if !c anything
                     * .if N anything
                     * .if !N anything
                     * .if 'string1'string2' anything
                     * .if !'string1'string2' anything
                     */
                    c=c+j;
                    c=scan_expression(c, &i);
                    if (request == REQ_ie)
                    {
                        int ifelseval=!i;
                        s_ifelseval.push( ifelseval );
                    }
                    if (i)
                    {
                        *c='\n';
                        c++;
                        c=scan_troff(c,1,NULL);
                    }
                    else
                        c=skip_till_newline(c);
                    break;
                }
                case REQ_ig: // groff(7) "IGnore"
                {
                    const char *endwith="..\n";
                    i=3;
                    c=c+j;
                    if (*c!='\n' && *c != '\\')
                    {
                        /* Not newline or comment */
                        endwith=c-1;i=1;
                        c[-1]='.';
                        while (*c && *c!='\n') c++,i++;
                    }
                    c++;
                    while (*c && qstrncmp(c,endwith,i)) while (*c++!='\n');
                    while (*c && *c++!='\n');
                    break;
                }
                case REQ_nf: // groff(7) "No Filling"
                {
                    if (fillout)
                    {
                        out_html(change_to_font(0));
                        out_html(change_to_size('0'));
                        out_html("<PRE>\n");
                    }
                    curpos=0;
                    fillout=0;
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_ps: // groff(7) "previous Point Size"
                {
                    c=c+j;
                    if (*c=='\n')
                        out_html(change_to_size('0'));
                    else
                    {
                        j=0; i=0;
                        if (*c=='-')
                        {
                            j= -1;
                            c++;
                        }
                        else if (*c=='+')
                            j=1;c++;
                        c=scan_expression(c, &i);
                        if (!j)
                        {
                            j=1;
                            if (i>5) i=i-10;
                        }
                        out_html(change_to_size(i*j));
                    }
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_sp: // groff(7) "SKip one line"
                {
                    c=c+j;
                    if (fillout)
                        out_html("<br><br>");
                    else
                    {
                        out_html(NEWLINE);
                    }
                    curpos=0;
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_so: // groff(7) "Include SOurce file"
                {
                    char *buf;
                    char *name=NULL;
                    curpos=0;
                    c=c+j;
                    if (*c=='/')
                        h=c;
                    else
                    {
                        h=c-3;
                        h[0]='.';
                        h[1]='.';
                        h[2]='/';
                    }
                    while (*c!='\n') c++;
                    *c='\0';
                    scan_troff(h,1, &name);
                    if (name[3]=='/')
                        h=name+3;
                    else
                        h=name;
                    /* this works alright, except for section 3 */
                    buf=read_man_page(h);
                    if (!buf)
                    {
#ifdef SIMPLE_MAN2HTML
                        fprintf(stderr, "man2html: unable to open or read file %s.\n", h);
#else
                        kdDebug(7107) << "Unable to open or read file: .so " << (h) << endl;
#endif
                        out_html("<BLOCKQUOTE>"
                                "man2html: unable to open or read file.\n");
                        out_html(h);
                        out_html("</BLOCKQUOTE>\n");
                    }
                    else
                        scan_troff(buf+1,0,NULL);
                    delete [] buf;
                    delete [] name;
    
                    *c++='\n';
                    break;
                }
                case REQ_ta: // gorff(7) "set TAbulators"
                {
                    c=c+j;
                    j=0;
                    while (*c!='\n')
                    {
                        sl=scan_expression(c, &tabstops[j]);
                        if (j>0 && (*c=='-' || *c=='+')) tabstops[j]+=tabstops[j-1];
                        c=sl;
                        while (*c==' ' || *c=='\t') c++;
                        j++;
                    }
                    maxtstop=j;
                    curpos=0;
                    break;
                }
                case REQ_ti: // groff(7) "Temporary Indent"
                {
                    /*while (itemdepth || dl_set[itemdepth]) {
                        out_html("</DL>\n");
                        if (dl_set[itemdepth]) dl_set[itemdepth]=0;
                        else itemdepth--;
                    }*/
                    out_html("<BR>\n");
                    c=c+j;
                    c=scan_expression(c, &j);
                    for (i=0; i<j; i++) out_html("&nbsp;");
                    curpos=j;
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_tm: // groff(7) "TerMinal" ### TODO: what are useful uses for it
                {
                    c=c+j;
                    h=c;
                    while (*c!='\n') c++;
                    *c='\0';
#ifdef SIMPLE_MAN2HTML
                    fprintf(stderr,"%s\n", h);
#else
                    kdDebug(7107) << ".tm " << (h) << endl;
#endif
                    *c='\n';
                    break;
                }
                case REQ_B: // man(7) "Bold"
                case REQ_I: // man(7) "Italic"
                {
                    /* parse one line in a certain font */
                    out_html(change_to_font(*c));
                    fill_words(c, wordlist, &words, false, 0);
                    c=c+j;
                    if (*c=='\n') c++;
                    c=scan_troff(c, 1, NULL);
                    out_html(change_to_font('R'));
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Fd: // mdoc(7) "Function Definition" ### FIXME
                {
                    // brackets and commas have to be inserted automatically
                    char font[2];
                    font[0] = 'B';
                    font[1] = 'B';
                    c+=j;
                    if (*c=='\n') c++;
                    char *eol=strchr(c,'\n');
                    char *semicolon=strchr(c,';');
                    if ((semicolon!=0) && (semicolon<eol)) *semicolon=' ';
                    
                    sl=fill_words(c, wordlist, &words, true, &c);
                    for (i=0; i<words; i++)
                    {
                        wordlist[i][-1]=' ';
                        out_html(change_to_font(font[i&1]));
                        scan_troff(wordlist[i],1,NULL);
                    }
                    if (mandoc_synopsis)
                    {
                        out_html(");");
                        out_html("<br>");
                    };
                    out_html(change_to_font('R'));
                    out_html(NEWLINE);
                    if (!fillout)
                        curpos=0;
                    else
                        curpos++;
                    break;
                }
                case REQ_Fn: // mdoc(7)  for "Function calls" ### FIXME
                {
                    // brackets and commas have to be inserted automatically
                    char font[2];
                    font[0] = 'B';
                    font[1] = 'B';
                    c+=j;
                    if (*c=='\n') c++;
                    char *eol=strchr(c,'\n');
                    char *semicolon=strchr(c,';');
                    if ((semicolon!=0) && (semicolon<eol)) *semicolon=' ';
        
                    sl=fill_words(c, wordlist, &words, true, &c);
                    if (!words)
                    {
                        out_html(" ()");
                    }
                    else
                    {
                        for (i=0; i<words; i++)
                        {
                            wordlist[i][-1]=' ';
                            out_html(change_to_font(font[i&1]));
                            scan_troff(wordlist[i],1,NULL);
                            if (i==0)
                            {
                                out_html(" (");
                            }
                            else if (i<words-1)
                                out_html(", ");
                        }
                        out_html(")");
                    }
                    out_html(change_to_font('R'));
                    if (mandoc_synopsis)
                        out_html("<br>");
                    out_html(NEWLINE);
                    if (!fillout)
                        curpos=0;
                    else
                        curpos++;
                    break;
                }
                case REQ_Fo: // mdoc(7) "Function definition Opening"
                {
                    char font[2];
                    font[0] = 'B';
                    font[1] = 'B';
                    c+=j;
                    if (*c=='\n') c++;
                    char *eol=strchr(c,'\n');
                    char *semicolon=strchr(c,';');
                    if ((semicolon!=0) && (semicolon<eol)) *semicolon=' ';
        
                    sl=fill_words(c, wordlist, &words, true, &c);
                    // Normally a .Fo has only one parameter
                    for (i=0; i<words; i++)
                    {
                        wordlist[i][-1]=' ';
                        out_html(change_to_font(font[i&1]));
                        scan_troff(wordlist[i],1,NULL);
                        if (i==0)
                        {
                            out_html(" (");
                        }
                        // ### TODO What should happen if there is more than one argument
                        // else if (i<words-1) out_html(", ");
                    }
                    function_argument=1; // Must be > 0
                    out_html(change_to_font('R'));
                    out_html(NEWLINE);
                    if (!fillout)
                        curpos=0;
                    else
                        curpos++;
                    break;
                }
                case REQ_Fc:// mdoc(7) "Function definition Close"
                {
                    // .Fc has no parameter
                    c+=j;
                    c=skip_till_newline(c);
                    char font[2];
                    font[0] = 'B';
                    font[1] = 'B';
                    out_html(change_to_font(font[i&1]));
                    out_html(")");
                    out_html(change_to_font('R'));
                    if (mandoc_synopsis)
                        out_html("<br>");
                    out_html(NEWLINE);
                    if (!fillout)
                        curpos=0;
                    else
                        curpos++;
                    function_argument=0; // Reset the count variable
                    break;
                }
                case REQ_Fa: // mdoc(7) "Function definition argument"
                {
                    char font[2] ;
                    font[0] = 'B';
                    font[1] = 'B';
                    c+=j;
                    if (*c=='\n') c++;
                    sl=fill_words(c, wordlist, &words, true, &c);
                    out_html(change_to_font(font[i&1]));
                    // function_argument==0 means that we had no .Fo  before, e.g. in mdoc.samples(7)
                    if (function_argument > 1)
                    {
                        out_html(", ");
                        curpos+=2;
                        function_argument++;
                    }
                    else if (function_argument==1)
                    {
                        // We are only at the first parameter
                        function_argument++;
                    }
                    for (i=0; i<words; i++)
                    {
                        wordlist[i][-1]=' ';
                        scan_troff(wordlist[i],1,NULL);
                    }
                    out_html(change_to_font('R'));
                    if (!fillout)
                        curpos=0;
                    else
                        curpos++;
                    break;
                }
                
                case REQ_OP:  /* groff manpages use this construction */
                    /* .OP a b : [ <B>a</B> <I>b</I> ] */
                    mode=true;
                    c[0]='B'; c[1]='I';
                    out_html(change_to_font('R'));
                    out_html("[");
                    curpos++;
                    // Do not break!
                case REQ_Ft:       //perhaps "Function return type"
                case REQ_BR:
                case REQ_BI:
                case REQ_IB:
                case REQ_IR:
                case REQ_RB:
                case REQ_RI:
                {
                    // ### VERIFY
                    bool inFMode=(c[0]=='F');
                    if (inFMode)
                    {
                        c[0]='B';
                        c[1]='I';
                    };
                    char font[2] ;
                    font[0] = c[0];
                    font[1] = c[1];
                    c=c+j;
                    if (*c=='\n') c++;
                    sl=fill_words(c, wordlist, &words, true, &c);
                    /* .BR name (section)
                     * indicates a link. It will be added in the output routine.
                     */
                    for (i=0; i<words; i++)
                    {
                        if ((mode) || (inFMode))
                        {
                            out_html(" ");
                            curpos++;
                        }
                        wordlist[i][-1]=' ';
                        out_html(change_to_font(font[i&1]));
                        scan_troff(wordlist[i],1,NULL);
                    }
                    out_html(change_to_font('R'));
                    if (mode)
                    {
                        out_html(" ]");
                        curpos++;
                    }
                    out_html(NEWLINE);
                    if (!fillout)
                        curpos=0;
                    else
                        curpos++;
                    break;
                }
                case REQ_DT: // man(7) "Default Tabulators"
                {
                    for (j=0;j<20; j++) tabstops[j]=(j+1)*8;
                    maxtstop=20;
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_IP: // man(7) "Ident Paragraph"
                {
                    sl=fill_words(c+j, wordlist, &words, true, &c);
                    if (!dl_set[itemdepth])
                    {
                        out_html("<DL>\n");
                        dl_set[itemdepth]=1;
                    }
                    out_html("<DT>");
                    if (words)
                        scan_troff(wordlist[0], 1,NULL);
                    out_html("<DD>");
                    curpos=0;
                    break;
                }
                case REQ_TP: // man(7) "hanging Tag Paragraph"
                {
                    if (!dl_set[itemdepth])
                    {
                        out_html("<br><br><DL>\n");
                        dl_set[itemdepth]=1;
                    }
                    out_html("<DT>");
                    c=skip_till_newline(c);
                    /* somewhere a definition ends with '.TP' */
                    if (!*c)
                        still_dd=true;
                    else
                    {
                        c=scan_troff(c,1,NULL);
                        out_html("<DD>");
                    }
                    curpos=0;
                    break;
                }
                case REQ_IX: // "INdex" ### TODO: where is it defined?
                {
                    /* general index */
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_P: // man(7) "Paragraph"
                case REQ_LP:// man(7) "Paragraph"
                case REQ_PP:// man(7) "Paragraph; reset Prevailing indent"
                {
                    if (dl_set[itemdepth])
                    {
                        out_html("</DL>\n");
                        dl_set[itemdepth]=0;
                    }
                    if (fillout)
                        out_html("<br><br>\n");
                    else
                    {
                        out_html(NEWLINE);
                    }
                    curpos=0;
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_HP: // man(7) "Hanging indent Paragraph"
                {
                    if (!dl_set[itemdepth])
                    {
                        out_html("<DL>");
                        dl_set[itemdepth]=1;
                    }
                    out_html("<DT>\n");
                    still_dd=true;
                    c=skip_till_newline(c);
                    curpos=0;
                    break;
                }
                case REQ_PD: // man(7) "Paragraph Distance"
                {
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_Rs: // mdoc(7) "Relative margin Start"
                case REQ_RS: // man(7) "Relative margin Start"
                {
                    sl=fill_words(c+j, wordlist, &words, true, 0);
                    j=1;
                    if (words>0) scan_expression(wordlist[0], &j);
                    if (j>=0)
                    {
                        itemdepth++;
                        dl_set[itemdepth]=0;
                        out_html("<DL><DT><DD>");
                        c=skip_till_newline(c);
                        curpos=0;
                        break;
                    }
                }
                case REQ_Re: // mdoc(7) "Relative margin End"
                case REQ_RE: // man(7) "Relative margin End"
                {
                    if (itemdepth > 0)
                    {
                        if (dl_set[itemdepth]) out_html("</DL>");
                        out_html("</DL>\n");
                        itemdepth--;
                    }
                    c=skip_till_newline(c);
                    curpos=0;
                    break;
                }
                case REQ_SB: // man(7) "Small; Bold"
                {
                    out_html(change_to_size(-1));
                    out_html(change_to_font('B'));
                    c=scan_troff(c+j, 1, NULL);
                    out_html(change_to_font('R'));
                    out_html(change_to_size('0'));
                    break;
                }
                case REQ_SM: // man(7) "SMall"
                {
                    c=c+j;
                    if (*c=='\n') c++;
                    out_html(change_to_size(-1));
                    trans_char(c,'"','\a');
                    c=scan_troff(c,1,NULL);
                    out_html(change_to_size('0'));
                    break;
                }
                case REQ_Ss: // mdoc(7) "Sub Section"
                    mandoc_command = 1;
                case REQ_SS: // mdoc(7) "Sub Section"
                    mode=true;
                case REQ_Sh: // mdoc(7) "Sub Header"
                                        /* hack for fallthru from above */
                    mandoc_command = !mode || mandoc_command;
                case REQ_SH: // man(7) "Sub Header"
                {
                    c=c+j;
                    if (*c=='\n') c++;
                    while (itemdepth || dl_set[itemdepth])
                    {
                        out_html("</DL>\n");
                        if (dl_set[itemdepth])
                            dl_set[itemdepth]=0;
                        else if (itemdepth > 0)
                            itemdepth--;
                    }
                    out_html(change_to_font(0));
                    out_html(change_to_size(0));
                    if (!fillout)
                    {
                        fillout=1;
                        out_html("</PRE>");
                    }
                    trans_char(c,'"', '\a');
                    if (section)
                    {
                        out_html("</div>\n");
                        section=0;
                    }
                    if (mode)
                        out_html("\n<H3>");
                    else
                        out_html("\n<H2>");
                    mandoc_synopsis = qstrncmp(c, "SYNOPSIS", 8) == 0;
                    c = mandoc_command ? scan_troff_mandoc(c,1,NULL) : scan_troff(c,1,NULL);
                    if (mode)
                        out_html("</H3>\n");
                    else
                        out_html("</H2>\n");
                    out_html("<div>\n");
        
                    section=1;
                    curpos=0;
                    break;
                }
                case REQ_Sx: // mdoc(7)
                {
                    // reference to a section header
                    out_html(change_to_font('B'));
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n') c++;
                    c=scan_troff(c, 1, NULL);
                    out_html(change_to_font('R'));
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_TS: // ### TODO where is it defined? (tbl?)
                {
                    c=scan_table(c);
                    break;
                }
                case REQ_Dt:	/* mdoc(7) */
                    mandoc_command = true;
                case REQ_TH: // man(7) "Title Header"
                {
                    if (!output_possible)
                    {
                        sl = fill_words(c+j, wordlist, &words, true, &c);
                        // ### TODO: the page should be displayed even if it is "anonymous" (words==0)
                        if (words>=1)
                        {
                            for (i=1; i<words; i++) wordlist[i][-1]='\0';
                            *sl='\0';
                            for (i=0; i<words; i++)
                            {
                                if (wordlist[i][0] == '\007')
                                    wordlist[i]++;
                                if (wordlist[i][qstrlen(wordlist[i])-1] == '\007')
                                    wordlist[i][qstrlen(wordlist[i])-1] = 0;
                            }
                            output_possible=true;
                            out_html( DOCTYPE"<HTML>\n<HEAD>\n");
#ifdef SIMPLE_MAN2HTML
                            // Most English man pages are in ISO-8859-1
                            out_html("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\">\n");
#else
                            // kio_man transforms from local to UTF-8
                            out_html("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=");
                            out_html(QTextCodec::codecForLocale()->mimeName());
                            out_html("\">\n");
#endif
                            out_html("<TITLE>");
                                out_html(scan_troff(wordlist[0], 0, NULL));
                            out_html( " Manpage</TITLE>\n");
                            out_html( "<link rel=\"stylesheet\" href=\"");
                            out_html(htmlPath);
                            out_html("/kde-default.css\" type=\"text/css\">\n" );
                            out_html( "<meta name=\"Mandoc Type\" content=\"");
                            if (mandoc_command)
                                out_html("mdoc");
                            else
                                out_html("man");
                            out_html("\">\n");
                            out_html( "</HEAD>\n\n" );
                            out_html("<BODY BGCOLOR=\"#FFFFFF\">\n\n" );
                            out_html("<div style=\"background-image: url(");
                            out_html(cssPath);
                            out_html("/top-middle.png); width: 100%; height: 131pt;\">\n" );
                            out_html("<div style=\"position: absolute; right: 0pt;\">\n");
                            out_html("<img src=\"");
                            out_html(htmlPath);
                            out_html("/top-right-konqueror.png\" style=\"margin: 0pt\" alt=\"Top right\">\n");
                            out_html("</div>\n");
        
                            out_html("<div style=\"position: absolute; left: 0pt;\">\n");
                            out_html("<img src=\"");
                            out_html(htmlPath);
                            out_html("/top-left.png\" style=\"margin: 0pt\" alt=\"Top left\">\n");
                            out_html("</div>\n");
                            out_html("<div style=\"position: absolute; top: 25pt; right: 100pt; text-align: right; font-size: xx-large; font-weight: bold; text-shadow: #fff 0pt 0pt 5pt; color: #444\">\n");
                            out_html( scan_troff(wordlist[0], 0, NULL ) );
                            out_html("</div>\n");
                            out_html("</div>\n");
                            out_html("<div style=\"margin-left: 5em; margin-right: 5em;\">\n");
                            out_html("<h1>" );
                                out_html( scan_troff(wordlist[0], 0, NULL ) );
                            out_html( "</h1>\n" );
                            if (words>1)
                            {
                                out_html("Section: " );
                                if (!mandoc_command && words>4)
                                    out_html(scan_troff(wordlist[4], 0, NULL) );
                                else
                                    out_html(section_name(wordlist[1]));
                                out_html(" (");
                                out_html(scan_troff(wordlist[1], 0, NULL));
                                out_html(")\n");
                            }
                            else
                            {
                                out_html("Section not specified");
                            }
                            *sl='\n';
                        }
                    }
                    else
                    {
#ifdef SIMPLE_MAN2HTML
                        fprintf(stderr, "%s", ".TH found but output not possible");
#else
                        kdWarning(7107) << ".TH found but output not possible" << endl;
#endif
                        c=skip_till_newline(c);
                    }
                    curpos=0;
                    break;
                }
                case REQ_TX: // mdoc(7)
                {
                    sl=fill_words(c+j, wordlist, &words, true, &c);
                    *sl='\0';
                    out_html(change_to_font('I'));
                    if (words>1) wordlist[1][-1]='\0';
                    const char *c2=lookup_abbrev(wordlist[0]);
                    curpos+=qstrlen(c2);
                    out_html(c2);
                    out_html(change_to_font('R'));
                    if (words>1)
                        out_html(wordlist[1]);
                    *sl='\n';
                    break;
                }
                case REQ_rm: // groff(7) "ReMove"
                /* .rm xx : Remove request, macro or string */
                    mode=true;
                case REQ_rn: // groff(7) "ReName"
                /* .rn xx yy : Rename request, macro or string xx to yy */
                {
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "start .rm/.rn" << endl;
#endif
                    c+=j;
                    h=c;
                    while (*h && *h!='\n' && (isalnum(*h) || *h=='@')) ++h;
                    if (*h)
                        *h=0;
                    const QCString name=c;
                    if ( name.isEmpty() )
                    {
#ifndef SIMPLE_MAN2HTML
                            kdDebug(7107) << "EXCEPTION: empty origin string to remove/rename: " << endl;
#endif
                            c=h;
                            break;
                    }
                    QCString name2;
                    if (!mode)
                    {
                        c=h+1;
                        while (*c && isspace(*c) && *c!='\n') ++c;
                        h=c;
                        while (*h && *h!='\n' && (isalnum(*h) || *h=='@')) ++h;
                        if (*h)
                            *h=0;
                        name2=c;
                        if ( name2.isEmpty() )
                        {
#ifndef SIMPLE_MAN2HTML
                            kdDebug(7107) << "EXCEPTION: empty destination string to rename: " << endl;
#endif
                            c=h;
                            break;
                        }
                    }
                    c=h+1;
#ifndef SIMPLE_MAN2HTML
                    if (mode)
                        kdDebug(7107) << "Remove " << name << endl;
                    else
                        kdDebug(7107) << "Rename " << name << " to " << name2 << endl;
#endif
                    while (*c && *c!='\n') c++;
                    c++;
                    QMap<QCString,StringDefinition>::iterator it=s_stringDefinitionMap.find(name);
                    if (it==s_stringDefinitionMap.end())
                    {
#ifndef SIMPLE_MAN2HTML
                        kdDebug(7107) << "EXCEPTION: cannot find string to rename or remove: " << name << endl;
#endif
                    }
                    else
                    {
                        if (mode)
                        {
                            // .rm ReMove
                            s_stringDefinitionMap.remove(name); // ### QT4: removeAll
                        }
                        else
                        {
                            // .rn ReName
                            StringDefinition def=(*it);
                            s_stringDefinitionMap.remove(name); // ### QT4: removeAll
                            s_stringDefinitionMap.insert(name2,def);
                        }
                    }
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "end .rm/.rn" << endl;
#endif
                    break;
                }
                case REQ_nx: // ### TODO in man(7) it is "No filling", not "next file"
                /* .nx filename : next file. */
                case REQ_in: // groff(7) "INdent"
                {
                    /* .in +-N : Indent */
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_nr: // groff(7) "Number Register"
                {
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "start .nr" << endl;
#endif
                    c += j;
                    h = c;
                    while ( *h && *h!='\n' && (isalnum(*h) || *h=='@')) ++h;
                    if (*h)
                        *h=0;
                    const QCString name=c;
                    if ( name.isEmpty() )
                    {
#ifndef SIMPLE_MAN2HTML
                            kdDebug(7107) << "EXCEPTION: empty origin string to remove/rename: " << endl;
#endif
                            c=h;
                            break;
                    }
                    c = h + 1;
                    while ( *c && ( *c==' ' || *c=='\t' ) ) c++;
                    int value = 0;
                    int increment = 0;
                    c=scan_expression( c, &value );
                    if ( *c && *c!='\n')
                    {
                        while ( *c && ( *c==' ' || *c=='\t' ) ) c++;
                        c=scan_expression( c, &increment );
                    }
                    c = skip_till_newline( c );
                    QMap <QCString, NumberDefinition>::iterator it = s_numberDefinitionMap.find( name );
                    if ( it == s_numberDefinitionMap.end() )
                    {
                        NumberDefinition def( value, increment );
                        s_numberDefinitionMap.insert( name, def );
                    }
                    else
                    {
                        (*it).m_value = value;
                        (*it).m_increment = increment;
                    }
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "end .nr" << endl;
#endif
                    break;
                }
                case REQ_am: // groff(7) "Append Macro"
                /* .am xx yy : append to a macro. */
                /* define or handle as .ig yy */
                mode=true;
                case REQ_de: // groff(7) "DEfine macro"
                /* .de xx yy : define or redefine macro xx; end at .yy (..) */
                /* define or handle as .ig yy */
                {
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "Start .am/.de" << endl;
#endif
                    c+=j;
                    char *next_line;
                    sl = fill_words(c, wordlist, &words, true, &next_line);
                    char *nameStart = wordlist[0];
                    c = nameStart;
                    while (*c && (*c != ' ') && (*c != '\n')) c++;
                    *c = '\0';
                    const QCString name(nameStart);
    
                    QCString endmacro;
                    if (words == 1)
                    {
                        endmacro="..";
                    }
                    else
                    {
                        endmacro='.';
                        c = wordlist[1];
                        while (*c && (*c != ' ') && (*c != '\n'))
                            endmacro+=*c++;
                    }
                    c = next_line;
                    sl=c;
                    const int length=qstrlen(endmacro);
                    while (*c && qstrncmp(c,endmacro,length))
                        c=skip_till_newline(c);

                    QCString macro;
                    while (sl!=c)
                    {
                        if (sl[0]=='\\' && sl[1]=='\\')
                        {
                            macro+='\\';
                            sl++;
                        }
                        else
                            macro+=*sl;
                        sl++;
                    }

                    QMap<QCString,StringDefinition>::iterator it=s_stringDefinitionMap.find(name);
                    if (it==s_stringDefinitionMap.end())
                    {
                        StringDefinition def;
                        def.m_length=0;
                        def.m_output=macro;
                        s_stringDefinitionMap.insert(name,def);
                    }
                    else if (mode)
                    {
                        // .am Append Macro
                        (*it).m_length=0; // It could be formerly a string
                        if ((*it).m_output.right(1)!='\n')
                            (*it).m_output+='\n';
                        (*it).m_output+=macro;
                    }
                    else
                    {
                        // .de DEfine macro
                        (*it).m_length=0; // It could be formerly a string
                        (*it).m_output=macro;
                    }
                    c=skip_till_newline(c);
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "End .am/.de" << endl;
#endif
                    break;
                }
                case REQ_Bl: // mdoc(7) "Begin List"
                {
                    char list_options[NULL_TERMINATED(MED_STR_MAX)];
                    char *nl = strchr(c,'\n');
                    c=c+j;
                    if (dl_set[itemdepth])
                    /* These things can nest. */
                    itemdepth++;
                    if (nl)
                    {
                        /* Parse list options */
                        strlimitcpy(list_options, c, nl - c, MED_STR_MAX);
                    }
                    if (strstr(list_options, "-bullet"))
                    {
                        /* HTML Unnumbered List */
                        dl_set[itemdepth] = BL_BULLET_LIST;
                        out_html("<UL>\n");
                    }
                    else if (strstr(list_options, "-enum"))
                    {
                        /* HTML Ordered List */
                        dl_set[itemdepth] = BL_ENUM_LIST;
                        out_html("<OL>\n");
                    }
                    else
                    {
                        /* HTML Descriptive List */
                        dl_set[itemdepth] = BL_DESC_LIST;
                        out_html("<DL>\n");
                    }
                    if (fillout)
                        out_html("<br><br>\n");
                    else
                    {
                        out_html(NEWLINE);
                    }
                    curpos=0;
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_El: // mdoc(7) "End List"
                {
                    c=c+j;
                    if (dl_set[itemdepth] & BL_DESC_LIST)
                        out_html("</DL>\n");
                    else if (dl_set[itemdepth] & BL_BULLET_LIST)
                        out_html("</UL>\n");
                    else if (dl_set[itemdepth] & BL_ENUM_LIST)
                        out_html("</OL>\n");
                    dl_set[itemdepth]=0;
                    if (itemdepth > 0) itemdepth--;
                    if (fillout)
                        out_html("<br><br>\n");
                    else
                    {
                        out_html(NEWLINE);
                    }
                    curpos=0;
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_It: // mdoc(7) "list ITem"
                {
                    c=c+j;
                    if (qstrncmp(c, "Xo", 2) == 0 && isspace(*(c+2)))
                    c = skip_till_newline(c);
                    if (dl_set[itemdepth] & BL_DESC_LIST)
                    {
                        out_html("<DT>");
                        out_html(change_to_font('B'));
                        if (*c=='\n')
                        {
                            /* Don't allow embedded comms after a newline */
                            c++;
                            c=scan_troff(c,1,NULL);
                        }
                        else
                        {
                            /* Do allow embedded comms on the same line. */
                            c=scan_troff_mandoc(c,1,NULL);
                        }
                        out_html(change_to_font('R'));
                        out_html(NEWLINE);
                        out_html("<DD>");
                    }
                    else if (dl_set[itemdepth] & (BL_BULLET_LIST | BL_ENUM_LIST))
                    {
                        out_html("<LI>");
                        c=scan_troff_mandoc(c,1,NULL);
                        out_html(NEWLINE);
                    }
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Bk:	/* mdoc(7) */
                case REQ_Ek:	/* mdoc(7) */
                case REQ_Dd:	/* mdoc(7) */
                case REQ_Os: // mdoc(7) "Operating System"
                {
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n') c++;
                    c=scan_troff_mandoc(c, 1, NULL);
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Bt: // mdoc(7) "Beta Test"
                {
                    trans_char(c,'"','\a');
                    c=c+j;
                    out_html(" is currently in beta test.");
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_At:	/* mdoc(7) */
                case REQ_Fx:	/* mdoc(7) */
                case REQ_Nx:	/* mdoc(7) */
                case REQ_Ox:	/* mdoc(7) */
                case REQ_Bx:	/* mdoc(7) */
                case REQ_Ux:	/* mdoc(7) */
                {
                    bool parsable=true;
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n') c++;
                    if (request==REQ_At)
                    {
                        out_html("AT&amp;T UNIX ");
                        parsable=false;
                    }
                    else if (request==REQ_Fx)
                    {
                        out_html("FreeBSD ");
                        parsable=false;
                    }
                    else if (request==REQ_Nx)
                        out_html("NetBSD ");
                    else if (request==REQ_Ox)
                        out_html("OpenBSD ");
                    else if (request==REQ_Bx)
                        out_html("BSD ");
                    else if (request==REQ_Ux)
                        out_html("UNIX ");
                    if (parsable)
                        c=scan_troff_mandoc(c,1,0);
                    else
                        c=scan_troff(c,1,0);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Dl:	/* mdoc(7) */
                {
                    c=c+j;
                    out_html(NEWLINE);
                    out_html("<BLOCKQUOTE>");
                    out_html(change_to_font('L'));
                    if (*c=='\n') c++;
                    c=scan_troff_mandoc(c, 1, NULL);
                    out_html(change_to_font('R'));
                    out_html("</BLOCKQUOTE>");
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
	       case REQ_Bd:	/* mdoc(7) */
	       {			/* Seems like a kind of example/literal mode */
                    char bd_options[NULL_TERMINATED(MED_STR_MAX)];
                    char *nl = strchr(c,'\n');
                    c=c+j;
                    if (nl)
                        strlimitcpy(bd_options, c, nl - c, MED_STR_MAX);
                    out_html(NEWLINE);
                    mandoc_bd_options = 0; /* Remember options for terminating Bl */
                    if (strstr(bd_options, "-offset indent"))
                    {
                        mandoc_bd_options |= BD_INDENT;
                        out_html("<BLOCKQUOTE>\n");
                    }
                    if ( strstr(bd_options, "-literal") || strstr(bd_options, "-unfilled"))
                    {
                        if (fillout)
                        {
                            mandoc_bd_options |= BD_LITERAL;
                            out_html(change_to_font(0));
                            out_html(change_to_size('0'));
                            out_html("<PRE>\n");
                        }
                        curpos=0;
                        fillout=0;
                    }
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_Ed:	/* mdoc(7) */
                {
                    if (mandoc_bd_options & BD_LITERAL)
                    {
                        if (!fillout)
                        {
                            out_html(change_to_font(0));
                            out_html(change_to_size('0'));
                            out_html("</PRE>\n");
                        }
                    }
                    if (mandoc_bd_options & BD_INDENT)
                        out_html("</BLOCKQUOTE>\n");
                    curpos=0;
                    fillout=1;
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_Be:	/* mdoc(7) */
                {
                    c=c+j;
                    if (fillout)
                        out_html("<br><br>");
                    else
                    {
                        out_html(NEWLINE);
                    }
                    curpos=0;
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_Xr:	/* mdoc(7) */ // ### FIXME: it should issue a <a href="man:somewhere(x)"> directly
                {
                    /* Translate xyz 1 to xyz(1)
                     * Allow for multiple spaces.  Allow the section to be missing.
                     */
                    char buff[NULL_TERMINATED(MED_STR_MAX)];
                    char *bufptr;
                    trans_char(c,'"','\a');
                    bufptr = buff;
                    c = c+j;
                    if (*c == '\n') c++; /* Skip spaces */
                    while (isspace(*c) && *c != '\n') c++;
                    while (isalnum(*c) || *c == '.' || *c == ':' || *c == '_' || *c == '-')
                    {
                        /* Copy the xyz part */
                        *bufptr = *c;
                        bufptr++;
                        if (bufptr >= buff + MED_STR_MAX) break;
                        c++;
                    }
                    while (isspace(*c) && *c != '\n') c++;	/* Skip spaces */
                    if (isdigit(*c))
                    {
                        /* Convert the number if there is one */
                        *bufptr = '(';
                        bufptr++;
                        if (bufptr < buff + MED_STR_MAX)
                        {
                            while (isalnum(*c))
                            {
                                *bufptr = *c;
                                bufptr++;
                                if (bufptr >= buff + MED_STR_MAX) break;
                                c++;
                            }
                            if (bufptr < buff + MED_STR_MAX)
                            {
                                *bufptr = ')';
                                bufptr++;
                            }
                        }
                    }
                    while (*c != '\n')
                    {
                        /* Copy the remainder */
                        if (!isspace(*c))
                        {
                            *bufptr = *c;
                            bufptr++;
                            if (bufptr >= buff + MED_STR_MAX) break;
                        }
                        c++;
                    }
                    *bufptr = '\n';
                    bufptr[1] = 0;
                    scan_troff_mandoc(buff, 1, NULL);
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Fl:	// mdoc(7) "FLags"
                {
                    trans_char(c,'"','\a');
                    c+=j;
                    sl=fill_words(c, wordlist, &words, true, &c);
                    out_html(change_to_font('B'));
                    if (!words)
                    {
                        out_html("-"); // stdin or stdout
                    }
                    else
                    {
                        for (i=0;i<words;++i)
                        {
                            if (ispunct(wordlist[i][0]) && wordlist[i][0]!='-')
                            {
                                scan_troff_mandoc(wordlist[i], 1, NULL);
                            }
                            else
                            {
                                if (i>0)
                                    out_html(" "); // Put a space between flags
                                out_html("-");
                                scan_troff_mandoc(wordlist[i], 1, NULL);
                            }
                        }
                    }
                    out_html(change_to_font('R'));
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Pa:	/* mdoc(7) */
                case REQ_Pf:	/* mdoc(7) */
                {
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n') c++;
                    c=scan_troff_mandoc(c, 1, NULL);
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Pp:	/* mdoc(7) */
                {
                    if (fillout)
                        out_html("<br><br>\n");
                    else
                    {
                        out_html(NEWLINE);
                    }
                    curpos=0;
                    c=skip_till_newline(c);
                    break;
                }
                case REQ_Aq: // mdoc(7) "Angle bracket Quote"
                    c=process_quote(c,j,"&lt;","&gt;");
                    break;
                case REQ_Bq: // mdoc(7) "Bracket Quote"
                    c=process_quote(c,j,"[","]");
                    break;
                case REQ_Dq:	// mdoc(7) "Double Quote"
                    c=process_quote(c,j,"&ldquo;","&rdquo;");
                    break;
                case REQ_Pq:	// mdoc(7) "Parenthese Quote"
                    c=process_quote(c,j,"(",")");
                    break;
                case REQ_Qq:	// mdoc(7) "straight double Quote"
                    c=process_quote(c,j,"&quot;","&quot;");
                    break;
                case REQ_Sq:	// mdoc(7) "Single Quote"
                    c=process_quote(c,j,"&lsquo;","&rsquo;");
                    break;
                case REQ_Op:	/* mdoc(7) */
                {
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n') c++;
                    out_html(change_to_font('R'));
                    out_html("[");
                    c=scan_troff_mandoc(c, 1, NULL);
                    out_html(change_to_font('R'));
                    out_html("]");
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Oo:	/* mdoc(7) */
                {
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n') c++;
                    out_html(change_to_font('R'));
                    out_html("[");
                    c=scan_troff_mandoc(c, 1, NULL);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
        	    break;
                }
                case REQ_Oc:	/* mdoc(7) */
                {
                    trans_char(c,'"','\a');
                    c=c+j;
                    c=scan_troff_mandoc(c, 1, NULL);
                    out_html(change_to_font('R'));
                    out_html("]");
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Ql:	/* mdoc(7) */
                {
                    /* Single quote first word in the line */
                    char *sp;
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n') c++;
                    sp = c;
                    do
                    {
                        /* Find first whitespace after the
                         * first word that isn't a mandoc macro
                         */
                        while (*sp && isspace(*sp)) sp++;
                        while (*sp && !isspace(*sp)) sp++;
                    } while (*sp && isupper(*(sp-2)) && islower(*(sp-1)));

                    /* Use a newline to mark the end of text to
                     * be quoted
                     */
                    if (*sp) *sp = '\n';
                    out_html("`");	/* Quote the text */
                    c=scan_troff_mandoc(c, 1, NULL);
                    out_html("'");
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Ar:	/* mdoc(7) */
                {
                    /* parse one line in italics */
                    out_html(change_to_font('I'));
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n')
                    {
                        /* An empty Ar means "file ..." */
                        out_html("file ...");
                    }
                    else
                        c=scan_troff_mandoc(c, 1, NULL);
                    out_html(change_to_font('R'));
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Em:	/* mdoc(7) */
                {
                    out_html("<em>");
                    trans_char(c,'"','\a');
                    c+=j;
                    if (*c=='\n') c++;
                    c=scan_troff_mandoc(c, 1, NULL);
                    out_html("</em>");
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Ad:	/* mdoc(7) */
                case REQ_Va:	/* mdoc(7) */
                case REQ_Xc:	/* mdoc(7) */
                {
                    /* parse one line in italics */
                    out_html(change_to_font('I'));
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n') c++;
                    c=scan_troff_mandoc(c, 1, NULL);
                    out_html(change_to_font('R'));
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Nd:	/* mdoc(7) */
                {
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n') c++;
                    out_html(" - ");
                    c=scan_troff_mandoc(c, 1, NULL);
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Nm:	// mdoc(7) "Name Macro" ### FIXME
                {
                    static char mandoc_name[NULL_TERMINATED(SMALL_STR_MAX)] = ""; // ### TODO Use QCString
                    trans_char(c,'"','\a');
                    c=c+j;

                    if (mandoc_synopsis && mandoc_name_count)
                    {
                        /* Break lines only in the Synopsis.
                         * The Synopsis section seems to be treated
                         * as a special case - Bummer!
                         */
                        out_html("<BR>");
                    }
                    else if (!mandoc_name_count)
                    {
                        const char *nextbreak = strchr(c, '\n');
                        const char *nextspace = strchr(c, ' ');
                        if (nextspace < nextbreak)
                            nextbreak = nextspace;

                        if (nextbreak)
                        {
                            /* Remember the name for later. */
                            strlimitcpy(mandoc_name, c, nextbreak - c, SMALL_STR_MAX);
                        }
                    }
                    mandoc_name_count++;
        
                    out_html(change_to_font('B'));
                    // ### FIXME: fill_words must be used
                    while (*c == ' '|| *c == '\t') c++;
                    if ((tolower(*c) >= 'a' && tolower(*c) <= 'z' ) || (*c >= '0' && *c <= '9'))
                    {
                        // alphanumeric argument
                        c=scan_troff_mandoc(c, 1, NULL);
                        out_html(change_to_font('R'));
                        out_html(NEWLINE);
                    }
                    else
                    {
                        /* If Nm has no argument, use one from an earlier
                        * Nm command that did have one.  Hope there aren't
                        * too many commands that do this.
                        */
                        out_html(mandoc_name);
                        out_html(change_to_font('R'));
                    }

                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Cd:	/* mdoc(7) */
                case REQ_Cm:	/* mdoc(7) */
                case REQ_Ic:	/* mdoc(7) */
                case REQ_Ms:	/* mdoc(7) */
                case REQ_Or:	/* mdoc(7) */
                case REQ_Sy:    /* mdoc(7) */
                {
                    /* parse one line in bold */
                    out_html(change_to_font('B'));
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n') c++;
                    c=scan_troff_mandoc(c, 1, NULL);
                    out_html(change_to_font('R'));
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_Dv:	/* mdoc(7) */
                case REQ_Ev:	/* mdoc(7) */
                case REQ_Fr:	/* mdoc(7) */
                case REQ_Li:	/* mdoc(7) */
                case REQ_No:	/* mdoc(7) */
                case REQ_Ns:	/* mdoc(7) */
                case REQ_Tn:	/* mdoc(7) */
                case REQ_nN:	/* mdoc(7) */
                {
                    trans_char(c,'"','\a');
                    c=c+j;
                    if (*c=='\n') c++;
                    out_html(change_to_font('B'));
                    c=scan_troff_mandoc(c, 1, NULL);
                    out_html(change_to_font('R'));
                    out_html(NEWLINE);
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_perc_A:	/* mdoc(7) biblio stuff */
                case REQ_perc_D:
                case REQ_perc_N:
                case REQ_perc_O:
                case REQ_perc_P:
                case REQ_perc_Q:
                case REQ_perc_V:
                {
                    c=c+j;
                    if (*c=='\n') c++;
                    c=scan_troff(c, 1, NULL); /* Don't allow embedded mandoc coms */
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_perc_B:
                case REQ_perc_J:
                case REQ_perc_R:
                case REQ_perc_T:
                {
                    c=c+j;
                    out_html(change_to_font('I'));
                    if (*c=='\n') c++;
                    c=scan_troff(c, 1, NULL); /* Don't allow embedded mandoc coms */
                    out_html(change_to_font('R'));
                    if (fillout)
                        curpos++;
                    else
                        curpos=0;
                    break;
                }
                case REQ_UR: // ### FIXME man(7) "URl"
                {
                    ignore_links=true;
                    c+=j;
                    char* newc;
                    h=fill_words(c, wordlist, &words, false, &newc);
                    *h=0;
                    if (words>0)
                    {
                        h=wordlist[0];
                        // A parameter : means that we do not want an URL, not here and not until .UE
                        ur_ignore=(!qstrcmp(h,":"));
                    }
                    else
                    {
                        // We cannot find the URL, assume :
                        ur_ignore=true;
                        h=0;
                    }
                    if (!ur_ignore && words>0)
                    {
                        out_html("<a href=\"");
                        out_html(h);
                        out_html("\">");
                    }
                    c=newc; // Go to next line
                    break;
                }
                case REQ_UE: // ### FIXME man(7) "Url End"
                {
                    c+=j;
                    c = skip_till_newline(c);
                    if (!ur_ignore)
                    {
                        out_html("</a>");
                    }
                    ur_ignore=false;
                    ignore_links=false;
                    break;
                }
                case REQ_UN: // ### FIXME man(7) "Url Named anchor"
                {
                    c+=j;
                    char* newc;
                    h=fill_words(c, wordlist, &words, false, &newc);
                    *h=0;
                    if (words>0)
                    {
                        h=wordlist[0];
                        out_html("<a name=\">");
                        out_html(h);
                        out_html("\" id=\"");
                        out_html(h);
                        out_html("\"></a>");
                    }
                    c=newc;
                    break;
                }
                case REQ_nroff: // groff(7)  "NROFF mode"
                    mode = true;
                case REQ_troff: // groff(7) "TROFF mode"
                {
                    s_nroff = mode;
                    c+=j;
                    c = skip_till_newline(c);
                }
                case REQ_als: // groff(7) "ALias String"
                {
                    /*
                     * Note an alias is supposed to be something like a hard link
                     * However to make it simplier, we only copy the string.
                     */
                    // Be careful: unlike .rm, the destination is first, origin is second
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "start .als" << endl;
#endif
                    c+=j;
                    h=c;
                    while (*h && *h!='\n' && (isalnum(*h) || *h=='@')) ++h;
                    if (*h)
                        *h=0;
                    const QCString name=c;
                    if ( name.isEmpty() )
                    {
#ifndef SIMPLE_MAN2HTML
                            kdDebug(7107) << "EXCEPTION: empty destination string to alias" << endl;
#endif
                            c=h;
                            break;
                    }
                    c=h+1;
                    while (*c && isspace(*c) && *c!='\n') ++c;
                    h=c;
                    while (*h && *h!='\n' && (isalnum(*h) || *h=='@')) ++h;
                    if (*h)
                        *h=0;
                    const QCString name2=c;
                    if ( name2.isEmpty() )
                    {
#ifndef SIMPLE_MAN2HTML
                            kdDebug(7107) << "EXCEPTION: empty origin string to alias" << endl;
#endif
                            c=h;
                            break;
                    }
                    c=h+1;
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "Alias " << name2 << " to " << name << endl;
#endif
                    while (*c && *c!='\n') c++;
                    c++;
                    if ( name == name2 )
                    {
#ifndef SIMPLE_MAN2HTML
                        kdDebug(7107) << "EXCEPTION: same origin and destination string to alias" << endl;
#endif
                        break;
                    }
                    // Second parametr is origin (unlike in .rm)
                    QMap<QCString,StringDefinition>::iterator it=s_stringDefinitionMap.find(name2);
                    if (it==s_stringDefinitionMap.end())
                    {
#ifndef SIMPLE_MAN2HTML
                        kdDebug(7107) << "EXCEPTION: cannot find string to make alias: " << name2 << endl;
#endif
                    }
                    else
                    {
                        StringDefinition def=(*it);
                        s_stringDefinitionMap.insert(name,def);
                    }
#ifndef SIMPLE_MAN2HTML
                    kdDebug(7107) << "end .als" << endl;
#endif
                    break;
                }
                default:
                {
                    if (mandoc_command &&
                        ((isupper(*c) && islower(*(c+1)))
                        || (islower(*c) && isupper(*(c+1)))) )
                    {
                        /* Let through any mdoc(7) commands that haven't
                         * been delt with.
                         * I don't want to miss anything out of the text.
                         */
                        char buf[4];
                        qstrncpy(buf,c,2);
                        buf[2] = ' ';
                        buf[3] = '\0';
                        out_html(buf);	/* Print the command (it might just be text). */
                        c=c+j;
                        trans_char(c,'"','\a');
                        if (*c=='\n') c++;
                        out_html(change_to_font('R'));
                        c=scan_troff(c, 1, NULL);
                        out_html(NEWLINE);
                        if (fillout)
                            curpos++;
                        else
                            curpos=0;
                    }
                    else
                        c=skip_till_newline(c);
                    break;
                }
            }
        }
    }
    if (fillout)
    {
        out_html(NEWLINE);
        curpos++;
    }
    return c;
}

static int contained_tab=0;
static bool mandoc_line=false;	/* Signals whether to look for embedded mandoc
				 * commands.
				 */

static char *scan_troff(char *c, bool san, char **result)
{   /* san : stop at newline */
    char *h;
    char intbuff[NULL_TERMINATED(MED_STR_MAX)];
    int ibp=0;
#define FLUSHIBP  if (ibp) { intbuff[ibp]=0; out_html(intbuff); ibp=0; }
    char *exbuffer;
    int exbuffpos, exbuffmax, exnewline_for_fun;
    bool exscaninbuff;
    int usenbsp=0;

    exbuffer=buffer;
    exbuffpos=buffpos;
    exbuffmax=buffmax;
    exnewline_for_fun=newline_for_fun;
    exscaninbuff=scaninbuff;
    newline_for_fun=0;
    if (result) {
	if (*result) {
	    buffer=*result;
	    buffpos=qstrlen(buffer);
	    buffmax=buffpos;
	} else {
            buffer = stralloc(LARGE_STR_MAX);
            buffpos=0;
            buffmax=LARGE_STR_MAX;
	}
	scaninbuff=true;
    }
    h=c; // ### FIXME below are too many tests that may go before the posiiton of c
    /* start scanning */

    // ### VERIFY: a dot must be at first position, we cannot add newlines or it would allow spaces before a dot
    while (*h == ' ')
    {
#if 1
        ++h;
#else
        *h++ = '\n';
#endif
    }

    while (h && *h && (!san || newline_for_fun || *h!='\n')) {

	if (*h==escapesym) {
	    h++;
	    FLUSHIBP;
	    h = scan_escape(h);
        } else if (*h==controlsym && h[-1]=='\n') {
	    h++;
	    FLUSHIBP;
	    h = scan_request(h);
	    if (h && san && h[-1]=='\n') h--;
	} else if (mandoc_line
            && ((*(h-1)) && (isspace(*(h-1)) || (*(h-1))=='\n'))
		   && *(h) && isupper(*(h))
		   && *(h+1) && islower(*(h+1))
                   && *(h+2) && isspace(*(h+2))) {
	    // mdoc(7) embedded command eg ".It Fl Ar arg1 Fl Ar arg2"
	    FLUSHIBP;
	    h = scan_request(h);
	    if (san && h[-1]=='\n') h--;
        } else if (*h==nobreaksym && h[-1]=='\n') {
	    h++;
	    FLUSHIBP;
	    h = scan_request(h);
	    if (san && h[-1]=='\n') h--;
	} else {
	    /* int mx; */
            if (still_dd && isalnum(*h) && h[-1]=='\n') {
		/* sometimes a .HP request is not followed by a .br request */
		FLUSHIBP;
		out_html("<DD>");
		curpos=0;
		still_dd=false;
	    }
	    switch (*h) {
	    case '&':
		intbuff[ibp++]='&';
		intbuff[ibp++]='a';
		intbuff[ibp++]='m';
		intbuff[ibp++]='p';
		intbuff[ibp++]=';';
		curpos++;
		break;
	    case '<':
		intbuff[ibp++]='&';
		intbuff[ibp++]='l';
		intbuff[ibp++]='t';
		intbuff[ibp++]=';';
		curpos++;
		break;
	    case '>':
		intbuff[ibp++]='&';
		intbuff[ibp++]='g';
		intbuff[ibp++]='t';
		intbuff[ibp++]=';';
		curpos++;
		break;
	    case '"':
		intbuff[ibp++]='&';
		intbuff[ibp++]='q';
		intbuff[ibp++]='u';
		intbuff[ibp++]='o';
		intbuff[ibp++]='t';
		intbuff[ibp++]=';';
		curpos++;
		break;
	    case '\n':
		if (h != c && h[-1]=='\n' && fillout) {
		    intbuff[ibp++]='<';
		    intbuff[ibp++]='P';
		    intbuff[ibp++]='>';
		}
		if (contained_tab && fillout) {
		    intbuff[ibp++]='<';
		    intbuff[ibp++]='B';
		    intbuff[ibp++]='R';
		    intbuff[ibp++]='>';
		}
		contained_tab=0;
		curpos=0;
		usenbsp=0;
		intbuff[ibp++]='\n';
		break;
	    case '\t':
		{
		    int curtab=0;
		    contained_tab=1;
		    FLUSHIBP;
		    /* like a typewriter, not like TeX */
		    tabstops[19]=curpos+1;
		    while (curtab<maxtstop && tabstops[curtab]<=curpos)
			curtab++;
		    if (curtab<maxtstop) {
			if (!fillout) {
			    while (curpos<tabstops[curtab]) {
				intbuff[ibp++]=' ';
				if (ibp>480) { FLUSHIBP; }
				curpos++;
			    }
			} else {
			    out_html("<TT>");
			    while (curpos<tabstops[curtab]) {
				out_html("&nbsp;");
				curpos++;
			    }
			    out_html("</TT>");
			}
		    }
		}
		break;
	    default:
		if (*h==' ' && (h[-1]=='\n' || usenbsp)) {
		    FLUSHIBP;
		    if (!usenbsp && fillout) {
			out_html("<BR>");
			curpos=0;
		    }
		    usenbsp=fillout;
		    if (usenbsp) out_html("&nbsp;"); else intbuff[ibp++]=' ';
		} else if (*h>31 && *h<127) intbuff[ibp++]=*h;
		else if (((unsigned char)(*h))>127) {
                    intbuff[ibp++]=*h;
		}
		curpos++;
		break;
	    }
	    if (ibp > (MED_STR_MAX - 20)) FLUSHIBP;
	    h++;
	}
    }
    FLUSHIBP;
    if (buffer) buffer[buffpos]='\0';
    if (san && h && *h) h++;
    newline_for_fun=exnewline_for_fun;
    if (result) {
	*result = buffer;
	buffer=exbuffer;
	buffpos=exbuffpos;
	buffmax=exbuffmax;
	scaninbuff=exscaninbuff;
    }

    return h;
}


static char *scan_troff_mandoc(char *c, bool san, char **result)
{
    char *ret;
    char *end = c;
    bool oldval = mandoc_line;
    mandoc_line = true;
    while (*end && *end != '\n') {
        end++;
    }

    if (end > c + 2
        && ispunct(*(end - 1))
	&& isspace(*(end - 2)) && *(end - 2) != '\n') {
      /* Don't format lonely punctuation E.g. in "xyz ," format
       * the xyz and then append the comma removing the space.
       */
        *(end - 2) = '\n';
	ret = scan_troff(c, san, result);
        *(end - 2) = *(end - 1);
        *(end - 1) = ' ';
    }
    else {
	ret = scan_troff(c, san, result);
    }
    mandoc_line = oldval;
    return ret;
}

// Entry point
void scan_man_page(const char *man_page)
{
    if (!man_page)
        return;

#ifndef SIMPLE_MAN2HTML
    kdDebug(7107) << "Start scanning man page" << endl;
#endif

    // ## Do more init
    // Unlike man2html, we actually call this several times, hence the need to
    // properly cleanup all those static vars
    s_ifelseval.clear();
    
    s_stringDefinitionMap.clear();
    InitStringDefinitions();
    
    s_numberDefinitionMap.clear();
    // ### TODO: initialoze default variables
    
    section = 0;

    s_dollarZero = ""; // No macro called yet!
    
    output_possible = false;
    int strLength = qstrlen(man_page);
    char *buf = new char[strLength + 2];
    qstrcpy(buf+1, man_page);
    buf[0] = '\n';

#ifndef SIMPLE_MAN2HTML
    kdDebug(7107) << "Parse man page" << endl;
#endif

    scan_troff(buf+1,0,NULL);

#ifndef SIMPLE_MAN2HTML
    kdDebug(7107) << "Man page parsed!" << endl;
#endif

    while (itemdepth || dl_set[itemdepth]) {
        out_html("</DL>\n");
        if (dl_set[itemdepth]) dl_set[itemdepth]=0;
        else if (itemdepth > 0) itemdepth--;
    }

    out_html(change_to_font(0));
    out_html(change_to_size(0));
    if (!fillout) {
	fillout=1;
	out_html("</PRE>");
    }
    out_html(NEWLINE);

    if (section) {
        output_real("<div style=\"margin-left: 2cm\">\n");
        section = 0;
    }

    if (output_possible) {
      output_real("</div>\n");
      output_real("<div class=\"bannerBottom\" style=\"background-image: url(");
      output_real(cssPath);
      output_real("/bottom-middle.png); background-repeat: x-repeat; width: 100%; height: 100pt; bottom:0pt;\">\n");
      output_real("<div class=\"bannerBottomLeft\">\n");
      output_real("<img src=\"");
      output_real(cssPath);
      output_real("/bottom-left.png\" style=\"margin: 0pt;\" alt=\"Bottom left of the banner\">\n");
      output_real("</div>\n");
      output_real("<div class=\"bannerBottomRight\">\n");
      output_real("<img src=\"");
      output_real(cssPath);
      output_real("/bottom-right.png\" style=\"margin: 0pt\" alt=\"Bottom right of the banner\">\n");
      output_real("</div>\n");
      output_real("</div>\n");

      output_real("</BODY>\n</HTML>\n");
    }
    delete [] buf;

    // Release memory
    s_stringDefinitionMap.clear();
    
    // reinit static variables for reuse
    s_numberDefinitionMap.clear();

    delete [] buffer;
    buffer = 0;

    escapesym='\\';
    nobreaksym='\'';
    controlsym='.';
    fieldsym=0;
    padsym=0;

    buffpos=0;
    buffmax=0;
    scaninbuff=false;
    itemdepth=0;
    for (int i = 0; i < 20; i++)
        dl_set[i] = 0;
    still_dd=false;
    for (int i = 0; i < 12; i++)
        tabstops[i] = (i+1)*8;
    maxtstop=12;
    curpos=0;

    argument = 0;
    mandoc_name_count = 0;
}

#ifdef SIMPLE_MAN2HTML
void output_real(const char *insert)
{
    printf("%s", insert);
}

char *read_man_page(const char *filename)
{
    int man_pipe = 0;
    char *man_buf = NULL;

    FILE *man_stream = NULL;
    struct stat stbuf;
    size_t buf_size;
    if (stat(filename, &stbuf) == -1) {
        std::cerr << "read_man_page: can't find " << filename << endl;
        return NULL;
    }
    if (!S_ISREG(stbuf.st_mode)) {
        std::cerr << "read_man_page: no file " << filename << endl;
        return NULL;
    }
    buf_size = stbuf.st_size;
    man_buf = stralloc(buf_size+5);
    man_pipe = 0;
    man_stream = fopen(filename, "r");
    if (man_stream) {
        man_buf[0] = '\n';
        if (fread(man_buf+1, 1, buf_size, man_stream) == buf_size) {
            man_buf[buf_size] = '\n';
            man_buf[buf_size + 1] = man_buf[buf_size + 2] = '\0';
        }
        else {
            man_buf = NULL;
        }
        fclose(man_stream);
    }
    return man_buf;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "call: " << argv[0] << " <filename>\n";
        return 1;
    }
    if (chdir(argv[1])) {
        char *buf = read_man_page(argv[1]);
        if (buf) {
            scan_man_page(buf);
            delete [] buf;
        }
    } else {
        DIR *dir = opendir(".");
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            fprintf(stderr, "converting %s\n", ent->d_name);
            char *buf = read_man_page(ent->d_name);
            if (buf) {
                scan_man_page(buf);
                delete [] buf;
            }
        }
        closedir(dir);
    }
    return 0;
}


#endif
