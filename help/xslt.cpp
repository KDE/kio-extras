#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxml/xmlIO.h>
#include <libxml/parserInternals.h>
#include <kdebug.h>
#include <kstddirs.h>
#include <qdir.h>
#include <xslt.h>
#include <kinstance.h>

int writeToQString(void * context, const char * buffer, int len)
{
    QString *t = (QString*)context;
    *t += QString::fromUtf8(buffer, len);
    return len;
}

void closeQString(void * context) {
    QString *t = (QString*)context;
    *t += QString::fromLatin1("\n");
}

QString transform( const QString &pat )
{
    QString tss = locate("dtd", "customization/kde-chunk.xsl");
    xsltStylesheetPtr style_sheet = xsltParseStylesheetFile((const xmlChar *)tss.latin1());

    QString parsed;

    if (style_sheet != NULL) {
        if (style_sheet->indent == 1)
	    xmlIndentTreeOutput = 1;
	else
	    xmlIndentTreeOutput = 0;

	xmlDocPtr doc = xmlParseFile(pat.latin1());
	if (doc == NULL) {
            return parsed;
	}
	xmlDocPtr res = xsltApplyStylesheet(style_sheet, doc);
	xmlFreeDoc(doc);
	if (res != NULL) {
            xmlOutputBufferPtr outp = xmlOutputBufferCreateIO(writeToQString, closeQString, &parsed, 0);
            outp->written = 0;
            htmlDocContentDumpOutput(outp, res, 0);
            xmlOutputBufferFlush(outp);
            xmlFreeDoc(res);
        }
        xsltFreeStylesheet(style_sheet);
    } else {
        kdDebug() << "couldn't parse style sheet " << tss << endl;
    }

    return parsed;
}

xmlParserInputPtr meinExternalEntityLoader(const char *URL, const char *ID,
					   xmlParserCtxtPtr ctxt) {
    xmlParserInputPtr ret = NULL;

    // fprintf(stderr, "loading %s %s\n", URL, ID);

    if (URL == NULL) {
        if ((ctxt->sax != NULL) && (ctxt->sax->warning != NULL))
            ctxt->sax->warning(ctxt,
                    "failed to load external entity \"%s\"\n", ID);
        return(NULL);
    }
    if (!strcmp(ID, "-//OASIS//DTD DocBook XML V4.1.2//EN"))
        URL = "docbook/xml-dtd-4.1.2/docbookx.dtd";
    if (!strcmp(ID, "-//OASIS//DTD XML DocBook V4.1.2//EN"))
	URL = "docbook/xml-dtd-4.1.2/docbookx.dtd";
    if (!strcmp(ID, "-//KDE//DTD DocBook XML V4.1-Based Variant V1.0//EN"))
        URL = "customization/dtd/kdex.dtd";
    if (!strcmp(ID, "-//KDE//DTD DocBook XML V4.1.2-Based Variant V1.0//EN"))
        URL = "customization/dtd/kdex.dtd";

    QString file = locate("dtd", URL);
    if (!file.isEmpty())
        ret = xmlNewInputFromFile(ctxt, file.latin1());

    if (ret == NULL) {
        if ((ctxt->sax != NULL) && (ctxt->sax->warning != NULL))
            ctxt->sax->warning(ctxt,

                "failed to load external entity \"%s\"\n", URL);
    }
    return(ret);
}

QString splitOut(const QString &parsed, int index)
{
    int start_index = index + 1;
    while (parsed.at(start_index - 1) != '>') start_index++;

    int inside = 0;

    QString filedata;

    while (true) {
        int endindex = parsed.find("</FILENAME>", index);
        int startindex = parsed.find("<FILENAME ", index) + 1;

//        kdDebug() << "FILENAME " << startindex << " " << endindex << " " << inside << " " << parsed.mid(startindex + 18, 15)<< " " << parsed.length() << endl;

        if (startindex > 0) {
            if (startindex < endindex) {
                //              kdDebug() << "finding another" << endl;
                index = startindex + 8;
                inside++;
            } else {
                index = endindex + 8;
                inside--;
            }
        } else {
            inside--;
            index = endindex + 1;
        }

        if (inside == 0) {
            filedata = parsed.mid(start_index, endindex - start_index);
            break;
        }

    }

    index = filedata.find("<FILENAME ");

    if (index > 0) {
        int endindex = filedata.findRev("</FILENAME>");
        while (filedata.at(endindex) != '>') endindex++;
        endindex++;
        filedata = filedata.left(index) + filedata.mid(endindex);
    }

    return filedata;
}

void fillInstance(KInstance &ins) {
    ins.dirs()->addResourceType("dtd", KStandardDirs::kde_default("data") + "ksgmltools2/");
    ins.dirs()->addResourceDir("dtd", QDir::currentDirPath());
}
