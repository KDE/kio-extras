
/*
 *   This file is part of the KDE KIO-Extras project
 *   SPDX-FileCopyrightText: 2009 Vytautas Mickus <vmickus@gmail.com>
 *   SPDX-FileCopyrightText: 2016 Anthony Fieroni <bvbfan@abv.com>
 *
 *   SPDX-License-Identifier: LGPL-2.1-only
 */

#include "audiocreator.h"

#include <QFile>
#include <QImage>
#include <QMimeType>
#include <QMimeDatabase>

#include <apetag.h>
#include <mp4tag.h>
#include <id3v2tag.h>
#include <fileref.h>
#include <mp4file.h>
#include <wavfile.h>
#include <apefile.h>
#include <mpcfile.h>
#include <mpegfile.h>
#include <aifffile.h>
#include <flacfile.h>
#include <wavpackfile.h>
#include <xiphcomment.h>
#include <flacpicture.h>
#include <attachedpictureframe.h>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator* new_creator()
    {
        return new AudioCreator;
    }
}

AudioCreator::AudioCreator()
{
}

AudioCreator::~AudioCreator()
{
}

namespace TagLib
{
namespace RIFF
{
namespace AIFF
{
struct FileExt : public File
{
    using File::File;
    ID3v2::Tag* ID3v2Tag() const
    {
        return tag();
    }
};
}
}
}

template<class T> static
bool parseID3v2Tag(T &file, QImage &img)
{
    if (!file.hasID3v2Tag()) {
        return false;
    }
    const auto &map = file.ID3v2Tag()->frameListMap();
    if (map["APIC"].isEmpty()) {
        return false;
    }
    auto apicFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(map["APIC"].front());
    if (!apicFrame) {
        return false;
    }
    const auto coverData = apicFrame->picture();
    img.loadFromData((uchar *)coverData.data(), coverData.size());
    return true;
}

template<class T> static
bool parseFlacTag(T &file, QImage &img)
{
    const auto pictureList = file.pictureList();
    for (const auto &picture : pictureList) {
        if (picture->type() != TagLib::FLAC::Picture::FrontCover) {
            continue;
        }
        const auto coverData = picture->data();
        img.loadFromData((uchar *)coverData.data(), coverData.size());
        return true;
    }
    return false;
}

template<class T> static
bool parseMP4Tag(T &file, QImage &img)
{
    if (!file.hasMP4Tag()) {
        return false;
    }
    const auto &map = file.tag()->itemMap();
    for (const auto &coverList : map) {
        auto coverArtList = coverList.second.toCoverArtList();
        if (coverArtList.isEmpty()) {
            continue;
        }
        const auto coverData = coverArtList[0].data();
        img.loadFromData((uchar *)coverData.data(), coverData.size());
        return true;
    }
    return false;
}

template<class T> static
bool parseAPETag(T &file, QImage &img)
{
    if (!file.hasAPETag()) {
        return false;
    }
    const auto &map = file.APETag()->itemListMap();
    for (const auto &item : map) {
        if (item.second.type() != TagLib::APE::Item::Binary) {
            continue;
        }
        const auto coverData = item.second.binaryData();
        const auto data = coverData.data();
        const auto size = coverData.size();
        for (size_t i=0; i<size; ++i) {
            if (data[i] == '\0' && (i+1) < size) {
                const auto start = data+i+1;
                img.loadFromData((uchar *)start, size-(start-data));
                return true;
            }
        }
    }
    return false;
}

bool AudioCreator::create(const QString &path, int, int, QImage &img)
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(path);
    if (!type.isValid()) {
        return false;
    }

    if (type.inherits("audio/mpeg")) {
        TagLib::MPEG::File file(QFile::encodeName(path).data());
        return parseID3v2Tag(file, img) || parseAPETag(file, img);
    }
    if (type.inherits("audio/x-flac") || type.inherits("audio/flac")) {
        TagLib::FLAC::File file(QFile::encodeName(path).data());
        return parseFlacTag(file, img) || parseID3v2Tag(file, img);
    }
    if (type.inherits("audio/mp4") || type.inherits("audio/x-m4a") ||
            type.inherits("audio/vnd.audible.aax")) {
        TagLib::MP4::File file(QFile::encodeName(path).data());
        return parseMP4Tag(file, img);
    }
    if (type.inherits("audio/x-ape")) {
        TagLib::APE::File file(QFile::encodeName(path).data());
        return parseAPETag(file, img);
    }
    if (type.inherits("audio/x-wavpack") || type.inherits("audio/x-vw")) {
        TagLib::WavPack::File file(QFile::encodeName(path).data());
        return parseAPETag(file, img);
    }
    if (type.inherits("audio/x-musepack")) {
        TagLib::MPC::File file(QFile::encodeName(path).data());
        return parseAPETag(file, img);
    }
    if (type.inherits("audio/ogg") || type.inherits("audio/vorbis")) {
        TagLib::FileRef fileRef(QFile::encodeName(path).data());
        if (fileRef.isNull()) {
            return false;
        }
        auto xiphComment = dynamic_cast<TagLib::Ogg::XiphComment*>(fileRef.tag());
        if (!xiphComment || xiphComment->isEmpty()) {
            return false;
        }
        return parseFlacTag(*xiphComment, img);
    }
    if (type.inherits("audio/x-aiff") || type.inherits("audio/x-aifc")) {
        TagLib::RIFF::AIFF::FileExt file(QFile::encodeName(path).data());
        return parseID3v2Tag(file, img);
    }
    if (type.inherits("audio/x-wav")) {
        TagLib::RIFF::WAV::File file(QFile::encodeName(path).data());
        return parseID3v2Tag(file, img);
    }
    return false;
}
