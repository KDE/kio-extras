
/***************************************************************************
 *   This file is part of the KDE KIO-Extras project                       *
 *   Copyright (C) 2009 Vytautas Mickus <vmickus@gmail.com>                *
 *   Copyright (C) 2016 Anthony Fieroni <bvbfan@abv.com>                   *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

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
