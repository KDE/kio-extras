/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Malte Starostik <malte@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _TEXTCREATOR_H_
#define _TEXTCREATOR_H_

#include <KIO/ThumbnailCreator>
#include <KSyntaxHighlighting/Repository>

#include <QPixmap>

class TextCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    TextCreator(QObject *parent, const QVariantList &args);
    ~TextCreator() override;
    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;

private:
    char *m_data;
    int m_dataSize;
    QPixmap m_pixmap;

    KSyntaxHighlighting::Repository m_highlightingRepository;
};

#endif
