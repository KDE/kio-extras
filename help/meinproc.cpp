/*
 * xsltproc.c: user program for the XSL Transformation 1.0 engine
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <libxml/xmlversion.h>
#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLtree.h>
#include <libxml/xmlIO.h>
#include <libxml/parserInternals.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <qstring.h>
#include <kstddirs.h>
#include <kinstance.h>
#include <xslt.h>
#include <iostream>
#include <qfile.h>

using namespace std;

extern int xmlLoadExtDtdDefaultValue;

int
main(int argc, char **argv) {

    //    xsltSetGenericDebugFunc(stderr, NULL);

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s xml\n", argv[0]);
        return 1;
    }
    KInstance ins("meinproc");
    fillInstance(ins);

    LIBXML_TEST_VERSION
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 1;
    xmlSetExternalEntityLoader(meinExternalEntityLoader);

    QString output = transform(argv[1]);
    if (output.isEmpty()) {
        fprintf(stderr, "unable to parse %s\n", argv[1]);
        return(1);
    }

    if (output.find( "<FILENAME " ) == -1 )
    {
        QFile file("index.html");
        file.open(IO_WriteOnly);
        QCString data = output.local8Bit();
        file.writeBlock(data.data(), data.length());
        file.close();
    } else {
        int index = 0;
        while (true) {
            index = output.find("<FILENAME ", index);
            if (index == -1)
                break;
            int filename_index = index + strlen("<FILENAME filename=\"");

            QString filename = output.mid(filename_index,
                                          output.find("\"", filename_index) -
                                          filename_index);

            QString filedata = splitOut(output, index);
            QFile file(filename);
            file.open(IO_WriteOnly);
            QCString data = filedata.local8Bit();
            file.writeBlock(data.data(), data.length());
            file.close();

            index += 8;
        }
    }
    xmlCleanupParser();
    xmlMemoryDump();
    return(0);
}

