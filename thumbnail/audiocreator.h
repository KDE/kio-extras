
/*
 *   This file is part of the KDE KIO-Extras project
 *   SPDX-FileCopyrightText: 2009 Vytautas Mickus <vmickus@gmail.com>
 *   SPDX-FileCopyrightText: 2016 Anthony Fieroni <bvbfan@abv.com>
 *
 *   SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef _AUDIO_CREATOR_H_
#define _AUDIO_CREATOR_H_

#include <QObject>
#include <KIO/ThumbCreator>

class AudioCreator : public QObject, public ThumbCreator
{
    Q_OBJECT
public:
    AudioCreator();
    ~AudioCreator() override;
    bool create(const QString &path, int w, int h, QImage &img) override;
};

#endif // _AUDIO_CREATOR_H_
