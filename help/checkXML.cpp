/*
 * tester.c : a small tester program for parsing using the SAX API.
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@w3.org
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h> /* only for xmlNewInputFromFile() */
#include <libxml/tree.h>
#include <libxml/debugXML.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlerror.h>

#include <qstring.h>
#include <kstddirs.h>
#include <kinstance.h>
#include <xslt.h>

static int debug = 0;
static int copy = 0;
static int recovery = 0;
static int push = 0;
static int speed = 0;

extern int xmlLoadExtDtdDefaultValue;

void parseAndPrintFile(char *filename) {

/*
 * Debug callback
 */
    if (!push) {
        /*
         * Empty callbacks for checking
         */
	if (!xmlParseFile(filename))
            exit(1);
	return;

    } else {
        assert(false);
    }
}

int main(int argc, char **argv) {
    int i;
    int files = 0;

    xmlLoadExtDtdDefaultValue = 1;

    KInstance ins("meinproc");
    fillInstance(ins);

    LIBXML_TEST_VERSION
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 1;
    xmlSetExternalEntityLoader(meinExternalEntityLoader);

    for (i = 1; i < argc ; i++) {
	if ((!strcmp(argv[i], "-debug")) || (!strcmp(argv[i], "--debug")))
	    debug++;
	else if ((!strcmp(argv[i], "-copy")) || (!strcmp(argv[i], "--copy")))
	    copy++;
	else if ((!strcmp(argv[i], "-recover")) ||
	         (!strcmp(argv[i], "--recover")))
	    recovery++;
	else if ((!strcmp(argv[i], "-push")) ||
	         (!strcmp(argv[i], "--push")))
	    push++;
	else if ((!strcmp(argv[i], "-speed")) ||
	         (!strcmp(argv[i], "--speed")))
	    speed++;
    }
    for (i = 1; i < argc ; i++) {
	if (argv[i][0] != '-') {
	    parseAndPrintFile(argv[i]);
	    files ++;
	}
    }
    xmlCleanupParser();
    xmlMemoryDump();

    return(0);
}
