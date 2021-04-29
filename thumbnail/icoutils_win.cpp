/*
    icoutils_win.cpp - Extract Microsoft Window icons and images using icoutils package

    SPDX-FileCopyrightText: 2013 Andrius da Costa Ribas <andriusmao@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "icoutils.h"

#include <windows.h>

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QList>

extern "C"
{
    // icon structs, as per https://msdn.microsoft.com/en-us/library/ms997538.aspx
#pragma pack( push )
#pragma pack( 2 )
    typedef struct
    {
        BYTE        bWidth;          // Width, in pixels, of the image
        BYTE        bHeight;         // Height, in pixels, of the image
        BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
        BYTE        bReserved;       // Reserved ( must be 0)
        WORD        wPlanes;         // Color Planes
        WORD        wBitCount;       // Bits per pixel
        DWORD       dwBytesInRes;    // How many bytes in this resource?
        DWORD       dwImageOffset;   // Where in the file is this image?
    } ICONDIRENTRY, *LPICONDIRENTRY;

    typedef struct
    {
        WORD           idReserved;   // Reserved (must be 0)
        WORD           idType;       // Resource Type (1 for icons)
        WORD           idCount;      // How many images?
        ICONDIRENTRY   idEntries[1]; // An entry for each image (idCount of 'em)
    } ICONDIR, *LPICONDIR;

    typedef struct
    {
        BYTE   bWidth;               // Width, in pixels, of the image
        BYTE   bHeight;              // Height, in pixels, of the image
        BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
        BYTE   bReserved;            // Reserved
        WORD   wPlanes;              // Color Planes
        WORD   wBitCount;            // Bits per pixel
        DWORD  dwBytesInRes;         // how many bytes in this resource?
        WORD   nID;                  // the ID
    } GRPICONDIRENTRY, *LPGRPICONDIRENTRY;

    typedef struct
    {
        WORD            idReserved;   // Reserved (must be 0)
        WORD            idType;       // Resource type (1 for icons)
        WORD            idCount;      // How many images?
        GRPICONDIRENTRY   idEntries[1]; // The entries for each image
    } GRPICONDIR, *LPGRPICONDIR;
#pragma pack( pop )
}


BOOL CALLBACK enumResNameCallback( HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam )
{
    QList<LPTSTR>* iconResources = (QList<LPTSTR>*) lParam;
    LPTSTR copyString;
    if ( IS_INTRESOURCE(lpszName) )
    {
        copyString = lpszName;
    }
    else
    {
        copyString = new WCHAR[lstrlen(lpszName) + 1];
        lstrcpy(copyString, lpszName);
    }

    if ( lpszType == RT_GROUP_ICON )
    {
        (*iconResources) << copyString;
    }
    return TRUE;
}

bool IcoUtils::loadIcoImageFromExe(const QString &inputFileName, QIODevice *outputDevice)
{

    HMODULE hModule;
    LPCTSTR fileName;
    QList<LPTSTR> iconResources;

    fileName = (TCHAR*) QDir::toNativeSeparators( inputFileName ).utf16();

    hModule = LoadLibraryEx ( fileName, 0, LOAD_LIBRARY_AS_DATAFILE );

    if ( !hModule )
    {
        return false;
    }

    EnumResourceNames ( hModule, RT_GROUP_ICON, enumResNameCallback, (LONG_PTR) &iconResources );

    if (!iconResources.isEmpty() )
    {
        HRSRC resourceInfo = FindResourceW ( hModule, (LPCTSTR) iconResources.at(0), RT_GROUP_ICON );
        if ( resourceInfo == 0 )
        {
            FreeLibrary( hModule );
            return false;
        }

        // we can get rid of the iconResources list now
        for (LPTSTR iconRes : qAsConst(iconResources)) {
            if ( !IS_INTRESOURCE(iconRes) )
            {
                delete [] iconRes;
            }
        }

        HGLOBAL resourceData = LoadResource ( hModule, resourceInfo );
        LPVOID resourcePointer = LockResource ( resourceData );
        int resourceSize = SizeofResource( hModule, resourceInfo );
        if ( resourceSize == 0 )
        {
            FreeLibrary( hModule );
            return false;
        }

        LPGRPICONDIR grpIconDir = (LPGRPICONDIR) resourcePointer;

        QBuffer outBuffer;
        outBuffer.open(QIODevice::ReadWrite);

        // helper
        const int iconDirHeaderSize = sizeof(grpIconDir->idReserved) + sizeof(grpIconDir->idType) + sizeof(grpIconDir->idCount);

        // copy the common part of GRPICONDIR and ICONDIR structures to the file
        outBuffer.write( (char*) resourcePointer, iconDirHeaderSize );

        DWORD imageOffset = iconDirHeaderSize + (grpIconDir->idCount) * sizeof(ICONDIRENTRY) ;

        for ( int i = 0 ; i < (grpIconDir->idCount) ; ++i )
        {
            // copy the common part of GRPICONDIRENTRY and ICONDIRENTRY structures to the respective ICONDIRENTRY's position
            LPGRPICONDIRENTRY grpIconDirEntry = &(grpIconDir->idEntries[i]);
            outBuffer.seek( iconDirHeaderSize + i * sizeof(ICONDIRENTRY) );
            outBuffer.write( (char*) grpIconDirEntry, sizeof(GRPICONDIRENTRY) - sizeof(grpIconDirEntry->nID) );

            // now, instead of nID, write the image offset
            outBuffer.write( (char*) &imageOffset, sizeof(DWORD) );

            // find the icon resource
            resourceInfo = FindResourceW ( hModule, MAKEINTRESOURCE(grpIconDirEntry->nID), RT_ICON );
            if ( resourceInfo == 0 )
            {
                FreeLibrary( hModule );
                return false;
            }
            resourceData = LoadResource ( hModule, resourceInfo );
            resourcePointer = LockResource ( resourceData );
            resourceSize = SizeofResource( hModule, resourceInfo );
            if ( resourceSize == 0 )
            {
                FreeLibrary( hModule );
                return false;
            }

            // seek to imageOffset
            outBuffer.seek(imageOffset);
            // write the icon data
            outBuffer.write( (char*) resourcePointer, resourceSize );
            // increment imageOffset
            imageOffset += resourceSize;
        }

        const bool ok = (outputDevice->write(outBuffer.data()) == outBuffer.size());

        FreeLibrary( hModule );
        return ok;
    }

    FreeLibrary( hModule );
    return false;
}
