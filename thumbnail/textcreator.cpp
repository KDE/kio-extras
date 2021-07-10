/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000, 2002 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2000 Malte Starostik <malte@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "textcreator.h"

#include <QFile>
#include <QFontDatabase>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QTextCodec>
#include <QTextDocument>

#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Theme>
#include <KSyntaxHighlighting/Definition>
#include <KDesktopFile>

// TODO Fix or remove kencodingprober code
// #include <kencodingprober.h>

extern "C"
{
Q_DECL_EXPORT KIO::ThumbDevicePixelRatioDependentCreator *new_creator()
{
    return new TextCreator;
}
}

TextCreator::TextCreator()
    : KIO::ThumbDevicePixelRatioDependentCreator(),
      m_data(nullptr),
      m_dataSize(0) {}

TextCreator::~TextCreator()
{
    delete [] m_data;
}

static QTextCodec *codecFromContent(const char *data, int dataSize)
{
#if 0 // ### Use this when KEncodingProber does not return junk encoding for UTF-8 data)
    KEncodingProber prober;
    prober.feed(data, dataSize);
    return QTextCodec::codecForName(prober.encoding());
#else
    QByteArray ba = QByteArray::fromRawData(data, dataSize);
    // try to detect UTF text, fall back to locale default (which is usually UTF-8)
    return QTextCodec::codecForUtfText(ba, QTextCodec::codecForLocale());
#endif
}

bool TextCreator::create(const QString &path, int width, int height, QImage &img)
{
    // Desktop files, .directory files, and flatpakrefs aren't traditional
    // text files, so their icons should be shown instead
    if (KDesktopFile::isDesktopFile(path)
            || path.endsWith(QStringLiteral(".directory"))
            || path.endsWith(QStringLiteral(".flatpakref"))
       ) {
        return false;
    }

    bool ok = false;

    // determine some sizes...
    // example: width: 60, height: 64
    QSize pixmapSize( width * devicePixelRatio(), height * devicePixelRatio() );
    if (height * 3 > width * 4)
        pixmapSize.setHeight( width * 4 / 3 * devicePixelRatio());
    else
        pixmapSize.setWidth( height * 3 / 4 * devicePixelRatio());

    if ( pixmapSize != m_pixmap.size() ) {
        m_pixmap = QPixmap( pixmapSize );
        m_pixmap.setDevicePixelRatio(devicePixelRatio());
    }

    // one pixel for the rectangle, the rest. whitespace
    int xborder = 1 + pixmapSize.width()/16 / devicePixelRatio();  // minimum x-border
    int yborder = 1 + pixmapSize.height()/16 / devicePixelRatio(); // minimum y-border

    // this font is supposed to look good at small sizes
    QFont font = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);

    font.setPixelSize( qMax(7, qMin( 10, ( pixmapSize.height()/ devicePixelRatio() - 2 * yborder ) / 16 ) ) );
    QFontMetrics fm( font );

    // calculate a better border so that the text is centered
    const QSizeF canvasSize(pixmapSize.width() / devicePixelRatio() - 2 * xborder, pixmapSize.height() / devicePixelRatio() - 2 * yborder);
    const int numLines = (int) (canvasSize.height() / fm.height());

    // assumes an average line length of <= 120 chars
    const int bytesToRead = 120 * numLines;

    // create text-preview
    QFile file( path );
    if ( file.open( QIODevice::ReadOnly ))
    {
        if ( !m_data || m_dataSize < bytesToRead + 1 )
        {
            delete [] m_data;
            m_data = new char[bytesToRead+1];
            m_dataSize = bytesToRead + 1;
        }

        int read = file.read( m_data, bytesToRead );
        if ( read > 0 )
        {
            ok = true;
            m_data[read] = '\0';
            QString text = codecFromContent( m_data, read )->toUnicode( m_data, read ).trimmed();
            // FIXME: maybe strip whitespace and read more?

            // If the text contains tabs or consecutive spaces, it is probably
            // formatted using white space. Use a fixed pitch font in this case.
            const auto textLines = text.splitRef(QLatin1Char('\n'));
            for (const auto& line : textLines) {
                const auto trimmedLine = line.trimmed();
                if ( trimmedLine.contains( '\t' ) || trimmedLine.contains( "  " ) ) {
                    font.setFamily( QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
                    break;
                }
            }

            QColor bgColor = QColor ( 245, 245, 245 ); // light-grey background
            m_pixmap.fill( bgColor );

            QPainter painter( &m_pixmap );

            QTextDocument textDocument(text);

            // QTextDocument only supports one margin value for all borders,
            // so we do a page-in-page behind its back, and do our own borders
            textDocument.setDocumentMargin(0);
            textDocument.setPageSize(canvasSize);
            textDocument.setDefaultFont(font);

            QTextOption textOption( Qt::AlignTop | Qt::AlignLeft );
            textOption.setTabStopDistance(8 * painter.fontMetrics().horizontalAdvance(QLatin1Char(' ')));
            textOption.setWrapMode( QTextOption::WrapAtWordBoundaryOrAnywhere );
            textDocument.setDefaultTextOption(textOption);

            KSyntaxHighlighting::SyntaxHighlighter syntaxHighlighter;
            syntaxHighlighter.setDefinition(m_highlightingRepository.definitionForFileName(path));
            const auto highlightingTheme = m_highlightingRepository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme);
            syntaxHighlighter.setTheme(highlightingTheme);
            syntaxHighlighter.setDocument(&textDocument);
            syntaxHighlighter.rehighlight();

            // draw page-in-page, with clipping as needed
            painter.translate(xborder, yborder);
            textDocument.drawContents(&painter, QRectF(QPointF(0, 0), canvasSize));

            painter.end();

            img = m_pixmap.toImage();
        }

        file.close();
    }
    return ok;
}
