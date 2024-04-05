/**
 * \file man2html.h
 *
 * Converts man pages to HTML.  This KIO worker is not part of KDE Frameworks and is
 * not public API.
 * \internal
 */

#ifndef MAN2HTML_H
#define MAN2HTML_H

class QByteArray;

/**
  Try to detect the encoding of given man page content
  and convert it to UTF-8
*/
char *manPageToUtf8(const QByteArray &input, const QByteArray &dirName);

/** call this with the buffer you have */
void scan_man_page(const char *man_page);

/** implement this somewhere. It will be called
   with HTML contents
*/
extern void output_real(const char *insert);

/**
 * called for requested man pages. filename can be a
 * relative path! Return NULL on errors. The returned
 * char array is freed by man2html
 */
extern char *read_man_page(const char *filename);

#endif // MAN2HTML_H
