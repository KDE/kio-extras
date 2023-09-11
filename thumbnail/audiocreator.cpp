
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

#include <KPluginFactory>

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

K_PLUGIN_CLASS_WITH_JSON(AudioCreator, "audiothumbnail.json")

AudioCreator::AudioCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
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

template<class T>
static KIO::ThumbnailResult parseID3v2Tag(T &file)
{
    if (!file.hasID3v2Tag() || !file.ID3v2Tag()) {
        return KIO::ThumbnailResult::fail();
    }
    const auto &map = file.ID3v2Tag()->frameListMap();
    if (map["APIC"].isEmpty()) {
        return KIO::ThumbnailResult::fail();
    }
    auto apicFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(map["APIC"].front());
    if (!apicFrame) {
        return KIO::ThumbnailResult::fail();
    }
    const auto coverData = apicFrame->picture();
    QImage img;
    bool okay = img.loadFromData((uchar *)coverData.data(), coverData.size());
    return okay ? KIO::ThumbnailResult::pass(img) : KIO::ThumbnailResult::fail();
}

template<class T>
static KIO::ThumbnailResult parseFlacTag(T &file)
{
    const auto pictureList = file.pictureList();
    for (const auto &picture : pictureList) {
        if (picture->type() != TagLib::FLAC::Picture::FrontCover) {
            continue;
        }
        const auto coverData = picture->data();
        QImage img;
        bool okay = img.loadFromData((uchar *)coverData.data(), coverData.size());
        return okay ? KIO::ThumbnailResult::pass(img) : KIO::ThumbnailResult::fail();
    }
    return KIO::ThumbnailResult::fail();
}

template<class T>
static KIO::ThumbnailResult parseMP4Tag(T &file)
{
    if (!file.hasMP4Tag() || !file.tag()) {
        return KIO::ThumbnailResult::fail();
    }
    const auto &map = file.tag()->itemMap();
    for (const auto &coverList : map) {
        auto coverArtList = coverList.second.toCoverArtList();
        if (coverArtList.isEmpty()) {
            continue;
        }
        const auto coverData = coverArtList[0].data();
        QImage img;
        bool okay = img.loadFromData((uchar *)coverData.data(), coverData.size());
        return okay ? KIO::ThumbnailResult::pass(img) : KIO::ThumbnailResult::fail();
    }
    return KIO::ThumbnailResult::fail();
}

template<class T>
static KIO::ThumbnailResult parseAPETag(T &file)
{
    if (!file.hasAPETag() || !file.APETag()) {
        return KIO::ThumbnailResult::fail();
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
                QImage img;
                bool okay = img.loadFromData((uchar *)start, size - (start - data));
                return okay ? KIO::ThumbnailResult::pass(img) : KIO::ThumbnailResult::fail();
                ;
            }
        }
    }
    return KIO::ThumbnailResult::fail();
}

KIO::ThumbnailResult AudioCreator::create(const KIO::ThumbnailRequest &request)
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForName(request.mimeType());

    const QByteArray fileNameBytes = QFile::encodeName(request.url().toLocalFile());
    const char *fileName = fileNameBytes.constData();

    if (!type.isValid()) {
        return KIO::ThumbnailResult::fail();
    }

    if (type.inherits("audio/mpeg")) {
        TagLib::MPEG::File file(fileName);

        if (auto result = parseID3v2Tag(file); result.isValid()) {
            return result;
        }

        return parseAPETag(file);
    }
    if (type.inherits("audio/x-flac") || type.inherits("audio/flac")) {
        TagLib::FLAC::File file(fileName);

        if (auto result = parseFlacTag(file); result.isValid()) {
            return result;
        }

        return parseID3v2Tag(file);
    }
    if (type.inherits("audio/mp4") || type.inherits("audio/x-m4a") ||
            type.inherits("audio/vnd.audible.aax")) {
        TagLib::MP4::File file(fileName);
        return parseMP4Tag(file);
    }
    if (type.inherits("audio/x-ape")) {
        TagLib::APE::File file(fileName);
        return parseAPETag(file);
    }
    if (type.inherits("audio/x-wavpack") || type.inherits("audio/x-vw")) {
        TagLib::WavPack::File file(fileName);
        return parseAPETag(file);
    }
    if (type.inherits("audio/x-musepack")) {
        TagLib::MPC::File file(fileName);
        return parseAPETag(file);
    }
    if (type.inherits("audio/ogg") || type.inherits("audio/vorbis")) {
        TagLib::FileRef fileRef(fileName);
        if (fileRef.isNull()) {
            return KIO::ThumbnailResult::fail();
        }
        auto xiphComment = dynamic_cast<TagLib::Ogg::XiphComment*>(fileRef.tag());
        if (!xiphComment || xiphComment->isEmpty()) {
            return KIO::ThumbnailResult::fail();
        }
        return parseFlacTag(*xiphComment);
    }
    if (type.inherits("audio/x-aiff") || type.inherits("audio/x-aifc")) {
        TagLib::RIFF::AIFF::FileExt file(fileName);
        return parseID3v2Tag(file);
    }
    if (type.inherits("audio/x-wav")) {
        TagLib::RIFF::WAV::File file(fileName);
        return parseID3v2Tag(file);
    }
    return KIO::ThumbnailResult::fail();
}

#include "audiocreator.moc"
#include "moc_audiocreator.cpp"
