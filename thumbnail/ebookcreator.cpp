/*
 * Copyright (c) 2019 Kai Uwe Broulik <kde@broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ebookcreator.h"

#include <QFile>
#include <QImage>
#include <QMimeDatabase>
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

            const auto *entry = zip.directory()->entry(entryPath);
            if (!entry || !entry->isFile()) {
                return false;
            }

            zipDevice.reset(static_cast<const KZipFileEntry *>(entry)->createDevice());
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
    QString coverId;
    QString coverHref;

    // First figure out where the OPF file with metadata is
    const auto *entry = zip.directory()->entry(QStringLiteral("META-INF/container.xml"));

    if (!entry || !entry->isFile()) {
        return false;
    }

    const auto *zipEntry = static_cast<const KZipFileEntry *>(entry);

    zipDevice.reset(zipEntry->createDevice());

    QXmlStreamReader xml(zipDevice.data());
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QLatin1String("rootfile")) {
            opfPath = xml.attributes().value(QStringLiteral("full-path")).toString();
        }
    }

    if (opfPath.isEmpty()) {
        return false;
    }

    // Now read the OPF file and look for a <meta name="cover" content="...">
    entry = zip.directory()->entry(opfPath);
    if (!entry || !entry->isFile()) {
        return false;
    }

    zipEntry = static_cast<const KZipFileEntry *>(entry);

    zipDevice.reset(zipEntry->createDevice());

    xml.setDevice(zipDevice.data());

    bool inMetadata = false;

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.name() == QLatin1String("metadata")) {
            if (xml.isStartElement()) {
                inMetadata = true;
            } else if (xml.isEndElement()) {
                break;
            }
        }

        if (!inMetadata) {
            continue;
        }

        if (xml.isStartElement() && xml.name() == QLatin1String("meta")) {
            const auto attributes = xml.attributes();
            if (attributes.value(QStringLiteral("name")) == QLatin1String("cover")) {
                coverId = attributes.value(QStringLiteral("content")).toString();
                break;
            }
        }
    }

    if (coverId.isEmpty()) {
        // Maybe we're lucky and the archive contains an iTunesArtwork file from iBooks
        entry = zip.directory()->entry(QStringLiteral("iTunesArtwork"));
        if (entry && entry->isFile()) {
            return image.loadFromData(static_cast<const KZipFileEntry *>(entry)->data());
        }

        // Maybe there's a file called "cover" somewhere
        const QStringList entries = getEntryList(zip.directory(), QString());

        for (const QString &name : entries) {
            if (!name.contains(QLatin1String("cover"), Qt::CaseInsensitive)) {
                continue;
            }

            entry = zip.directory()->entry(name);
            if (!entry || !entry->isFile()) {
                continue;
            }

            if (image.loadFromData(static_cast<const KZipFileEntry *>(entry)->data())) {
                return true;
            }
        }
    }

    // Read OPF file again from beginning and look for <item id="our id from above" href="...">
    zipDevice->seek(0);
    xml.setDevice(zipDevice.data());

    bool inManifest = false;

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.name() == QLatin1String("manifest")) {
            if (xml.isStartElement()) {
                inManifest = true;
            } else if (xml.isEndElement()) {
                break;
            }
        }

        if (!inManifest) {
            continue;
        }

        if (xml.isStartElement() && xml.name() == QLatin1String("item")) {
            const auto attributes = xml.attributes();
            if (attributes.value(QStringLiteral("id")).compare(coverId, Qt::CaseInsensitive) == 0) {
                coverHref = attributes.value(QStringLiteral("href")).toString();
                break;
            }
        }
    }

    if (coverHref.isEmpty()) {
        return false;
    }

    // Make coverHref relative to OPF location
    const int lastOpfSlash = opfPath.lastIndexOf(QLatin1Char('/'));
    if (lastOpfSlash > -1) {
        QString basePath = opfPath.left(lastOpfSlash + 1);
        coverHref.prepend(basePath);
    }

    // Finally, just load the cover image file
    entry = zip.directory()->entry(coverHref);
    if (!entry || !entry->isFile()) {
        return false;
    }

    return image.loadFromData(static_cast<const KZipFileEntry *>(entry)->data());
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
