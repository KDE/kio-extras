#ifndef _XSLT_H_
#define _XSLT_H_

#include <libxml/parser.h>

QString transform(const QString &file, const QString& stylesheet);
xmlParserInputPtr meinExternalEntityLoader(const char *URL, const char *ID,
					   xmlParserCtxtPtr ctxt);
QString splitOut(const QString &parsed, int index);
void fillInstance(KInstance &ins);

#endif
