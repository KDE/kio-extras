/*
 * SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "ebookcreator.h"

#include <QFile>
#include <QImage>
#include <QMap>
#include <QMimeDatabase>
#include <QUrl>
#include <QXmlStreamReader>

#include <KZip>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new EbookCreator;
    }
}

EbookCreator::EbookCreator() = default;

EbookCreator::~EbookCreator() = default;

bool EbookCreator::create(const QString &path, int width, int height, QImage &image)
{
    Q_UNUSED(width);
    Q_UNUSED(height);

    QMimeType mimeType = QMimeDatabase().mimeTypeForFile(path);

    if (mimeType.name() == QLatin1String("application/epub+zip")) {
        return createEpub(path, image);

    } else if (mimeType.name() == QLatin1String("application/x-fictionbook+xml")) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        return createFb2(&file, image);

    } else if (mimeType.name() == QLatin1String("application/x-zip-compressed-fb2")) {
        KZip zip(path);
        if (!zip.open(QIODevice::ReadOnly)) {
            return false;
        }

        QScopedPointer<QIODevice> zipDevice;

        const auto entries = zip.directory()->entries();
        for (const QString &entryPath : entries) {
            if (entries.count() > 1 && !entryPath.endsWith(QLatin1String(".fb2"))) { // can this done a bit more cleverly?
                continue;
            }

            const auto *entry = zip.directory()->file(entryPath);
            if (!entry) {
                return false;
            }

            zipDevice.reset(entry->createDevice());
        }

        return createFb2(zipDevice.data(), image);
    }

    return false;
}

bool EbookCreator::createEpub(const QString &path, QImage &image)
{
    KZip zip(path);
    if (!zip.open(QIODevice::ReadOnly)) {
        return false;
    }

    QScopedPointer<QIODevice> zipDevice;
    QString opfPath;
    QString coverHref;

    // First figure out where the OPF file with metadata is
    const auto *entry = zip.directory()->file(QStringLiteral("META-INF/container.xml"));

    if (!entry) {
        return false;
    }

    zipDevice.reset(entry->createDevice());

    QXmlStreamReader xml(zipDevice.data());
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QLatin1String("rootfile")) {
            opfPath = xml.attributes().value(QStringLiteral("full-path")).toString();
            break;
        }
    }

    if (opfPath.isEmpty()) {
        return false;
    }

    // Now read the OPF file and look for a <meta name="cover" content="...">
    entry = zip.directory()->file(opfPath);
    if (!entry) {
        return false;
    }

    zipDevice.reset(entry->createDevice());

    xml.setDevice(zipDevice.data());

    bool inMetadata = false;
    bool inManifest = false;
    QString coverId;
    QMap<QString, QString> itemHrefs;

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.name() == QLatin1String("metadata")) {
            if (xml.isStartElement()) {
                inMetadata = true;
            } else if (xml.isEndElement()) {
                inMetadata = false;
            }
            continue;
        }

        if (xml.name() == QLatin1String("manifest")) {
            if (xml.isStartElement()) {
                inManifest = true;
            } else if (xml.isEndElement()) {
                inManifest = false;
            }
            continue;
        }

        if (xml.isStartElement()) {
            if (inMetadata && (xml.name() == QLatin1String("meta"))) {
                const auto attributes = xml.attributes();
                if (attributes.value(QStringLiteral("name")) == QLatin1String("cover")) {
                    coverId = attributes.value(QStringLiteral("content")).toString();
                }
            } else if (inManifest && (xml.name() == QLatin1String("item"))) {
                const auto attributes = xml.attributes();
                const QString href = attributes.value(QStringLiteral("href")).toString();
                const QString id = attributes.value(QStringLiteral("id")).toString();
                if (!id.isEmpty() && !href.isEmpty()) {
                    // EPUB 3 has the "cover-image" property set
                    const auto properties = attributes.value(QStringLiteral("properties")).toString();
                    const auto propertyList = properties.split(QChar(' '), Qt::SkipEmptyParts);
                    if (propertyList.contains(QLatin1String("cover-image"))) {
                        coverHref = href;
                        break;
                    }
                    itemHrefs[id] = href;
                }
            } else {
                continue;
            }

            if (!coverId.isEmpty() && itemHrefs.contains(coverId)) {
                coverHref = itemHrefs[coverId];
                break;
            }
        }
    }

    if (coverHref.isEmpty()) {
        // Maybe we're lucky and the archive contains an iTunesArtwork file from iBooks
        entry = zip.directory()->file(QStringLiteral("iTunesArtwork"));
        if (entry) {
            zipDevice.reset(entry->createDevice());
            return image.load(zipDevice.data(), "");
        }

        // Maybe there's a file called "cover" somewhere
        const QStringList entries = getEntryList(zip.directory(), QString());

        for (const QString &name : entries) {
            if (!name.contains(QLatin1String("cover"), Qt::CaseInsensitive)) {
                continue;
            }

            entry = zip.directory()->file(name);
            if (!entry) {
                continue;
            }

            zipDevice.reset(entry->createDevice());
            if (image.load(zipDevice.data(), "")) {
                return true;
            }
        }
        return false;
    }

    // Decode percent encoded URL
    QByteArray encoded = itemHrefs[coverId].toUtf8();
    coverHref = QUrl::fromPercentEncoding(encoded);

    // Make coverHref relative to OPF location
    const int lastOpfSlash = opfPath.lastIndexOf(QLatin1Char('/'));
    if (lastOpfSlash > -1) {
        QString basePath = opfPath.left(lastOpfSlash + 1);
        coverHref.prepend(basePath);
    }

    // Finally, just load the cover image file
    entry = zip.directory()->file(coverHref);
    if (entry) {
        zipDevice.reset(entry->createDevice());
        return image.load(zipDevice.data(), "");
    }

    return false;
}

bool EbookCreator::createFb2(QIODevice *device, QImage &image)
{
    QString coverId;

    QXmlStreamReader xml(device);

    bool inFictionBook = false;
    bool inDescription = false;
    bool inTitleInfo = false;
    bool inCoverPage = false;

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.name() == QLatin1String("FictionBook")) {
            if (xml.isStartElement()) {
                inFictionBook = true;
            } else if (xml.isEndElement()) {
                break;
            }
        } else if (xml.name() == QLatin1String("description")) {
            if (xml.isStartElement()) {
                inDescription = true;
            } else if (xml.isEndElement()) {
                inDescription = false;
            }
        } else if (xml.name() == QLatin1String("title-info")) {
            if (xml.isStartElement()) {
                inTitleInfo = true;
            } else if (xml.isEndElement()) {
                inTitleInfo = false;
            }
        } else if (xml.name() == QLatin1String("coverpage")) {
            if (xml.isStartElement()) {
                inCoverPage = true;
            } else if (xml.isEndElement()) {
                inCoverPage = false;
            }
        }

        if (!inFictionBook) {
            continue;
        }

        if (inDescription) {
            if (inTitleInfo && inCoverPage) {
                if (xml.isStartElement() && xml.name() == QLatin1String("image")) {
                    const auto attributes = xml.attributes();

                    // value() wants a namespace but we don't care, so iterate until we find any "href"
                    for (const auto &attribute : attributes) {
                        if (attribute.name() == QLatin1String("href")) {
                            coverId = attribute.value().toString();
                            if (coverId.startsWith(QLatin1Char('#'))) {
                                coverId = coverId.mid(1);
                            }
                        }
                    }
                }
            }
        } else {
            if (!coverId.isEmpty() && xml.isStartElement() && xml.name() == QLatin1String("binary")) {
                if (xml.attributes().value(QStringLiteral("id")) == coverId) {
                    return image.loadFromData(QByteArray::fromBase64(xml.readElementText().toLatin1()));
                }
            }
        }
    }

    return false;
}

QStringList EbookCreator::getEntryList(const KArchiveDirectory *dir, const QString &path)
{
    QStringList list;

    const QStringList entries = dir->entries();
    for (const QString &name : entries) {
        const KArchiveEntry *entry = dir->entry(name);

        QString fullPath = name;

        if (!path.isEmpty()) {
            fullPath.prepend(QLatin1Char('/'));
            fullPath.prepend(path);
        }

        if (entry->isFile()) {
            list << fullPath;
        } else {
            list << getEntryList(static_cast<const KArchiveDirectory *>(entry), fullPath);
        }
    }

    return list;
}
