/*
    exeutils.cpp - Extract Microsoft Window icons from Microsoft Windows executables

    SPDX-FileCopyrightText: 2023 John Chadwick <john@jchw.io>

    SPDX-License-Identifier: LGPL-2.0-or-later OR BSD-2-Clause
*/

#include "exeutils.h"

#include <QDataStream>
#include <QIODevice>
#include <QMap>
#include <QVector>

#include <optional>

namespace
{

// Eexecutable file (.exe)
struct DosHeader {
    char signature[2];
    quint32 newHeaderOffset;
};

QDataStream &operator>>(QDataStream &s, DosHeader &v)
{
    s.readRawData(v.signature, sizeof(v.signature));
    s.device()->skip(58);
    s >> v.newHeaderOffset;
    return s;
}

// Resource Directory
enum class ResourceType : quint32 {
    Icon = 3,
    GroupIcon = 14,
};

struct RtGroupIconDirectory {
    quint16 reserved;
    quint16 type;
    quint16 count;
};

struct RtGroupIconDirectoryEntry {
    quint8 width;
    quint8 height;
    quint8 colorCount;
    quint8 reserved;
    quint16 numPlanes;
    quint16 bpp;
    quint32 size;
    quint16 resourceId;
};

QDataStream &operator>>(QDataStream &s, RtGroupIconDirectory &v)
{
    s >> v.reserved >> v.type >> v.count;
    return s;
}

QDataStream &operator>>(QDataStream &s, RtGroupIconDirectoryEntry &v)
{
    s >> v.width >> v.height >> v.colorCount >> v.reserved >> v.numPlanes >> v.bpp >> v.size >> v.resourceId;
    return s;
}

// Icon file (.ico)
struct IconDir {
    quint16 reserved;
    quint16 type;
    quint16 count;
};

constexpr int IconDirSize = 6;

struct IconDirEntry {
    quint8 width;
    quint8 height;
    quint8 colorCount;
    quint8 reserved;
    quint16 numPlanes;
    quint16 bpp;
    quint32 size;
    quint32 imageOffset;

    IconDirEntry(const RtGroupIconDirectoryEntry &entry, quint32 dataOffset)
    {
        width = entry.width;
        height = entry.height;
        colorCount = entry.colorCount;
        reserved = entry.reserved;
        numPlanes = entry.numPlanes;
        bpp = entry.bpp;
        size = entry.size;
        imageOffset = dataOffset;
    }
};

constexpr int IconDirEntrySize = 16;

QDataStream &operator<<(QDataStream &s, const IconDir &v)
{
    s << v.reserved << v.type << v.count;
    return s;
}

QDataStream &operator<<(QDataStream &s, const IconDirEntry &v)
{
    s << v.width << v.height << v.colorCount << v.reserved << v.numPlanes << v.bpp << v.size << v.imageOffset;
    return s;
}

// Win16 New Executable

struct NeFileHeader {
    quint16 offsetOfResourceTable;
    quint16 numberOfResourceSegments;
};

struct NeResource {
    quint16 dataOffsetShifted;
    quint16 dataLength;
    quint16 flags;
    quint16 resourceId;
    quint16 resource[2];
};

struct NeResourceTable {
    struct Type {
        quint16 typeId;
        quint16 numResources;
        quint16 resource[2];
        QVector<NeResource> resources;
    };

    quint16 alignmentShiftCount;
    QMap<ResourceType, Type> types;
};

QDataStream &operator>>(QDataStream &s, NeFileHeader &v)
{
    s.device()->skip(34);
    s >> v.offsetOfResourceTable;
    s.device()->skip(14);
    s >> v.numberOfResourceSegments;
    return s;
}

QDataStream &operator>>(QDataStream &s, NeResource &v)
{
    s >> v.dataOffsetShifted >> v.dataLength >> v.flags >> v.resourceId >> v.resource[0] >> v.resource[1];
    v.resourceId ^= 0x8000;
    return s;
}

QDataStream &operator>>(QDataStream &s, NeResourceTable::Type &v)
{
    s >> v.typeId;
    if (v.typeId == 0) {
        return s;
    }
    s >> v.numResources >> v.resource[0] >> v.resource[1];
    for (int i = 0; i < v.numResources; i++) {
        NeResource resource;
        s >> resource;
        v.resources.append(resource);
    }
    return s;
}

QDataStream &operator>>(QDataStream &s, NeResourceTable &v)
{
    s >> v.alignmentShiftCount;
    while (1) {
        NeResourceTable::Type type;
        s >> type;
        if (!type.typeId) {
            break;
        }
        v.types[ResourceType(type.typeId ^ 0x8000)] = type;
    }
    return s;
}

bool readNewExecutablePrimaryIcon(QDataStream &ds, const DosHeader &dosHeader, QIODevice *outputDevice)
{
    NeFileHeader fileHeader;
    NeResourceTable resources;
    QDataStream out{outputDevice};

    out.setByteOrder(QDataStream::LittleEndian);

    if (!ds.device()->seek(dosHeader.newHeaderOffset)) {
        return false;
    }

    char signature[2];
    ds.readRawData(signature, sizeof(signature));

    if (signature[0] != 'N' || signature[1] != 'E') {
        return false;
    }

    ds >> fileHeader;
    if (!ds.device()->seek(dosHeader.newHeaderOffset + fileHeader.offsetOfResourceTable)) {
        return false;
    }

    ds >> resources;

    if (!resources.types.contains(ResourceType::GroupIcon) || !resources.types.contains(ResourceType::Icon)) {
        return false;
    }

    auto iconResources = resources.types[ResourceType::Icon].resources;

    auto iconGroupResources = resources.types[ResourceType::GroupIcon];
    if (iconGroupResources.resources.empty()) {
        return false;
    }

    auto iconGroupResource = iconGroupResources.resources.first();
    if (!ds.device()->seek(iconGroupResource.dataOffsetShifted << resources.alignmentShiftCount)) {
        return false;
    }

    RtGroupIconDirectory iconGroup;
    ds >> iconGroup;

    IconDir icoHeader;
    icoHeader.reserved = 0;
    icoHeader.type = 1; // Always 1 for ico files.
    icoHeader.count = iconGroup.count;
    out << icoHeader;

    quint32 dataOffset = IconDirSize + IconDirEntrySize * iconGroup.count;
    QVector<QPair<qint64, quint32>> resourceOffsetSizePairs;

    for (int i = 0; i < iconGroup.count; i++) {
        RtGroupIconDirectoryEntry entry;
        ds >> entry;

        auto it = std::find_if(iconResources.begin(), iconResources.end(), [&entry](const NeResource &res) {
            return res.resourceId == entry.resourceId;
        });
        if (it == iconResources.end()) {
            return false;
        }

        IconDirEntry icoEntry{entry, dataOffset};
        out << icoEntry;

        NeResource iconRes = *it;
        resourceOffsetSizePairs.append({iconRes.dataOffsetShifted << resources.alignmentShiftCount, entry.size});
        dataOffset += entry.size;
    }

    for (auto offsetSizePair : resourceOffsetSizePairs) {
        if (!ds.device()->seek(offsetSizePair.first)) {
            return false;
        }
        outputDevice->write(ds.device()->read(offsetSizePair.second));
    }

    return true;
}

// Win32 Portable Executable

constexpr quint16 PeOptionalHeaderMagicPe32 = 0x010b;
constexpr quint16 PeOptionalHeaderMagicPe32Plus = 0x020b;
constexpr quint32 PeSubdirBitMask = 0x80000000;

constexpr int PeSignatureSize = 4;
constexpr int PeFileHeaderSize = 20;
constexpr int PeOffsetToDataDirectoryPe32 = 120;
constexpr int PeOffsetToDataDirectoryPe32Plus = 136;
constexpr int PeDataDirectorySize = 8;

enum class PeDataDirectoryIndex {
    Resource = 2,
};

struct PeFileHeader {
    quint16 machine;
    quint16 numSections;
    quint32 timestamp;
    quint32 offsetToSymbolTable;
    quint32 numberOfSymbols;
    quint16 sizeOfOptionalHeader;
    quint16 fileCharacteristics;
};

struct PeDataDirectory {
    quint32 virtualAddress, size;
};

struct PeSection {
    char name[8];
    quint32 virtualSize, virtualAddress;
    quint32 sizeOfRawData, pointerToRawData;
    quint32 pointerToRelocs, pointerToLineNums;
    quint16 numRelocs, numLineNums;
    quint32 characteristics;
};

struct PeResourceDirectoryTable {
    quint32 characteristics;
    quint32 timestamp;
    quint16 majorVersion, minorVersion;
    quint16 numNameEntries, numIDEntries;
};

struct PeResourceDirectoryEntry {
    quint32 resourceId, offset;
};

struct PeResourceDataEntry {
    quint32 dataAddress;
    quint32 size;
    quint32 codepage;
    quint32 reserved;
};

QDataStream &operator>>(QDataStream &s, PeFileHeader &v)
{
    s >> v.machine >> v.numSections >> v.timestamp >> v.offsetToSymbolTable >> v.numberOfSymbols >> v.sizeOfOptionalHeader >> v.fileCharacteristics;
    return s;
}

QDataStream &operator>>(QDataStream &s, PeDataDirectory &v)
{
    s >> v.virtualAddress >> v.size;
    return s;
}

QDataStream &operator>>(QDataStream &s, PeSection &v)
{
    s.readRawData(v.name, sizeof(v.name));
    s >> v.virtualSize >> v.virtualAddress >> v.sizeOfRawData >> v.pointerToRawData >> v.pointerToRelocs >> v.pointerToLineNums >> v.numRelocs >> v.numLineNums
        >> v.characteristics;
    return s;
}

QDataStream &operator>>(QDataStream &s, PeResourceDirectoryTable &v)
{
    s >> v.characteristics >> v.timestamp >> v.majorVersion >> v.minorVersion >> v.numNameEntries >> v.numIDEntries;
    return s;
}

QDataStream &operator>>(QDataStream &s, PeResourceDirectoryEntry &v)
{
    s >> v.resourceId >> v.offset;
    return s;
}

QDataStream &operator>>(QDataStream &s, PeResourceDataEntry &v)
{
    s >> v.dataAddress >> v.size >> v.codepage >> v.reserved;
    return s;
}

qint64 addressToOffset(const QVector<PeSection> &sections, quint32 rva)
{
    for (int i = 0; i < sections.size(); i++) {
        auto sectionBegin = sections[i].virtualAddress;
        auto sectionEnd = sections[i].virtualAddress + sections[i].sizeOfRawData;
        if (rva >= sectionBegin && rva < sectionEnd) {
            return rva - sectionBegin + sections[i].pointerToRawData;
        }
    }
    return -1;
}

QVector<PeResourceDirectoryEntry> readResourceDataDirectoryEntry(QDataStream &ds)
{
    PeResourceDirectoryTable table;
    ds >> table;
    QVector<PeResourceDirectoryEntry> entries;
    for (int i = 0; i < table.numNameEntries + table.numIDEntries; i++) {
        PeResourceDirectoryEntry entry;
        ds >> entry;
        entries.append(entry);
    }
    return entries;
}

bool readPortableExecutablePrimaryIcon(QDataStream &ds, const DosHeader &dosHeader, QIODevice *outputDevice)
{
    PeFileHeader fileHeader;
    bool isPe32Plus;
    QMap<quint32, PeResourceDataEntry> iconResources;
    std::optional<PeResourceDataEntry> primaryIconGroupResource;
    QVector<PeSection> sections;
    QDataStream out{outputDevice};

    out.setByteOrder(QDataStream::LittleEndian);

    // Seek to + verify PE header. We're at the file header after this.
    if (!ds.device()->seek(dosHeader.newHeaderOffset)) {
        return false;
    }

    char signature[4];
    if (ds.readRawData(signature, sizeof(signature)) == -1) {
        return false;
    }

    if (signature[0] != 'P' || signature[1] != 'E' || signature[2] != 0 || signature[3] != 0) {
        return false;
    }

    ds >> fileHeader;

    // Read optional header magic to determine if this is PE32 or PE32+.
    quint16 optMagic;
    ds >> optMagic;

    switch (optMagic) {
    case PeOptionalHeaderMagicPe32:
        isPe32Plus = false;
        break;

    case PeOptionalHeaderMagicPe32Plus:
        isPe32Plus = true;
        break;

    default:
        return false;
    }

    // Read section table now, so we can interpret RVAs.
    quint64 sectionTableOffset = dosHeader.newHeaderOffset;
    sectionTableOffset += PeSignatureSize + PeFileHeaderSize;
    sectionTableOffset += fileHeader.sizeOfOptionalHeader;
    if (!ds.device()->seek(sectionTableOffset)) {
        return false;
    }

    for (int i = 0; i < fileHeader.numSections; i++) {
        PeSection section;
        ds >> section;
        sections.append(section);
    }

    // Find resource directory.
    qint64 dataDirOffset = dosHeader.newHeaderOffset;
    if (isPe32Plus) {
        dataDirOffset += PeOffsetToDataDirectoryPe32Plus;
    } else {
        dataDirOffset += PeOffsetToDataDirectoryPe32;
    }
    dataDirOffset += qint64(PeDataDirectoryIndex::Resource) * PeDataDirectorySize;
    if (!ds.device()->seek(dataDirOffset)) {
        return false;
    }
    PeDataDirectory resourceDirectory;
    ds >> resourceDirectory;

    // Read resource tree.
    auto resourceOffset = addressToOffset(sections, resourceDirectory.virtualAddress);
    if (resourceOffset < 0) {
        return false;
    }

    if (!ds.device()->seek(resourceOffset)) {
        return false;
    }

    auto level1 = readResourceDataDirectoryEntry(ds);

    for (auto entry1 : level1) {
        if ((entry1.offset & PeSubdirBitMask) == 0)
            continue;
        if (!ds.device()->seek(resourceOffset + (entry1.offset & ~PeSubdirBitMask))) {
            return false;
        }

        auto level2 = readResourceDataDirectoryEntry(ds);

        for (auto entry2 : level2) {
            if ((entry2.offset & PeSubdirBitMask) == 0)
                continue;
            if (!ds.device()->seek(resourceOffset + (entry2.offset & ~PeSubdirBitMask))) {
                return false;
            }

            // Read subdirectory.
            auto level3 = readResourceDataDirectoryEntry(ds);

            for (auto entry3 : level3) {
                if ((entry3.offset & PeSubdirBitMask) == PeSubdirBitMask)
                    continue;
                if (!ds.device()->seek(resourceOffset + (entry3.offset & ~PeSubdirBitMask))) {
                    return false;
                }

                // Read data.
                PeResourceDataEntry dataEntry;
                ds >> dataEntry;

                switch (ResourceType(entry1.resourceId)) {
                case ResourceType::Icon:
                    iconResources[entry2.resourceId] = dataEntry;
                    break;

                case ResourceType::GroupIcon:
                    if (!primaryIconGroupResource.has_value()) {
                        primaryIconGroupResource = dataEntry;
                    }
                }
            }
        }
    }

    if (!primaryIconGroupResource.has_value()) {
        return false;
    }

    if (!ds.device()->seek(addressToOffset(sections, primaryIconGroupResource->dataAddress))) {
        return false;
    }

    RtGroupIconDirectory primaryIconGroup;
    ds >> primaryIconGroup;

    IconDir icoFileHeader;
    icoFileHeader.reserved = 0;
    icoFileHeader.type = 1; // Always 1 for ico files.
    icoFileHeader.count = primaryIconGroup.count;
    out << icoFileHeader;

    quint32 dataOffset = IconDirSize + IconDirEntrySize * primaryIconGroup.count;
    QVector<QPair<qint64, quint32>> resourceOffsetSizePairs;

    for (int i = 0; i < primaryIconGroup.count; i++) {
        RtGroupIconDirectoryEntry entry;
        ds >> entry;

        IconDirEntry icoFileEntry{entry, dataOffset};
        out << icoFileEntry;

        if (auto it = iconResources.find(entry.resourceId); it != iconResources.end()) {
            PeResourceDataEntry iconResource = *it;
            resourceOffsetSizePairs.append({addressToOffset(sections, iconResource.dataAddress), iconResource.size});
            dataOffset += iconResource.size;
        } else {
            return false;
        }
    }

    for (auto offsetSizePair : resourceOffsetSizePairs) {
        if (!ds.device()->seek(offsetSizePair.first)) {
            return false;
        }
        outputDevice->write(ds.device()->read(offsetSizePair.second));
    }

    return true;
}

}

bool ExeUtils::loadIcoDataFromExe(QIODevice *inputDevice, QIODevice *outputDevice)
{
    QDataStream ds{inputDevice};
    ds.setByteOrder(QDataStream::LittleEndian);

    // Read DOS header.
    DosHeader dosHeader;
    ds >> dosHeader;

    // Verify the MZ header.
    if (dosHeader.signature[0] != 'M' || dosHeader.signature[1] != 'Z') {
        return false;
    }

    if (readPortableExecutablePrimaryIcon(ds, dosHeader, outputDevice)) {
        return true;
    }

    if (readNewExecutablePrimaryIcon(ds, dosHeader, outputDevice)) {
        return true;
    }

    return false;
}
