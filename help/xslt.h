#ifndef _XSLT_H_
#define _XSLT_H_

QString transform(const QString &file);
xmlParserInputPtr meinExternalEntityLoader(const char *URL, const char *ID,
					   xmlParserCtxtPtr ctxt);
QString splitOut(const QString &parsed, int index);
void fillInstance(KInstance &ins);

#endif
