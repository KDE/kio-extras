
/*
 *   This file is part of the KDE KIO-Extras project
 *   SPDX-FileCopyrightText: 2009 Vytautas Mickus <vmickus@gmail.com>
 *   SPDX-FileCopyrightText: 2016 Anthony Fieroni <bvbfan@abv.com>
 *
 *   SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef _AUDIO_CREATOR_H_
#define _AUDIO_CREATOR_H_

#include <KIO/ThumbnailCreator>
#include <QObject>

class AudioCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    AudioCreator(QObject *parent, const QVariantList &args);
    ~AudioCreator() override;
    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
};

#endif // _AUDIO_CREATOR_H_
