/***************************************************************************
                          sftpfileattr.h  -  description
                             -------------------
    begin                : Sat Jun 30 2001
    copyright            : (C) 2001 by Lucas Fisher
    email                : ljfisher@iastate.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SFTPFILEATTR_H
#define SFTPFILEATTR_H

#include <sys/types.h>

#include <qglobal.h>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QString>

#include <kio/global.h>
#include <kdebug.h>

#include "sftp.h"

/**
  *@author Lucas Fisher
  */

class KRemoteEncoding;

class sftpFileAttr {

private: // Private attributes
    /** Name of file. */
    QString mFilename;

    /** Specifies which fields of the file attribute are available. */
    quint32 mFlags;

    /** Size of the file in bytes. Should be 64 bit safe. */
    quint64 mSize;

    /** User id of the owner of the file. */
    uid_t mUid;

    /** Group id of the group to which the file belongs. */
    gid_t mGid;

    /** POSIX permissions of the file. */
    mode_t mPermissions;

    /** Last access time of the file in seconds from Jan 1, 1970. */
    time_t mAtime;

    /** Last modification time of file in seconds since Jan. 1, 1970. */
    time_t mMtime;

    /** Number of file attribute extensions.
        Not currently implemented */
    quint32 mExtendedCount;

     /** Longname of the file as found in a SSH_FXP_NAME sftp packet.
          These contents are parse to return the file's owner name and
          gr oup name. */
    QByteArray mLongname;

    QString mUserName;
    QString mGroupName;

    /** If file is a link, contains the destination of the link */
    QString mLinkDestination;

    /** If resource is a link, contains the type the link,e.g. file,dir... */
    mode_t mLinkType;

    /** Whether >> operator should read filename and longname from the stream. */
    bool mDirAttrs;

    /** Holds the encoding of the remote host */
    KRemoteEncoding* mEncoding;

public:
    sftpFileAttr();

    sftpFileAttr(KRemoteEncoding* encoding);

    ~sftpFileAttr();

    /** Constructor to initialize the file attributes on declaration. */
    sftpFileAttr(quint64 size_, uid_t uid_, gid_t gid_, mode_t permissions_,
                 time_t atime_, time_t mtime_, quint32 extendedCount_ = 0);

    /** Return the size of the sftp attribute not including filename or longname*/
    quint32 size() const;

    /** Clear all attributes and flags. */
    void clear();

    /** Set the size of the file. */
    void setFileSize(quint64 s)
        { mSize = s; mFlags |= SSH2_FILEXFER_ATTR_SIZE; }

    /** The size file attribute will not be included in the UDSEntry
        or when the file attribute is written to the sftp packet. */
    void clearFileSize()
        { mSize = 0; mFlags &= ~SSH2_FILEXFER_ATTR_SIZE; }

    /** Returns the size of the file. */
    quint64 fileSize() const { return mSize; }

    /** Sets the POSIX permissions of the file. */
    void setPermissions(mode_t p)
        { mPermissions = p; mFlags |= SSH2_FILEXFER_ATTR_PERMISSIONS; }

    /** The permissions file attribute will not be included in the UDSEntry
        or when the file attribute is written to the sftp packet. */
    void clearPermissions()
        { mPermissions = 0; mFlags &= ~SSH2_FILEXFER_ATTR_PERMISSIONS; }

    /** Returns the POSIX permissons of the file. */
    mode_t permissions() const { return mPermissions; }

    /** Sets the group id of the file. */
    void setGid(gid_t id)
        { mGid = id; mFlags |= SSH2_FILEXFER_ATTR_UIDGID; }

    /** Neither the gid or uid file attributes will not be included in the UDSEntry
        or when the file attribute is written to the sftp packet. This is
        equivalent to clearUid() */
    void clearGid()
        { mGid = 0; mFlags &= SSH2_FILEXFER_ATTR_UIDGID; }

    /** Returns the group id of the file. */
    gid_t gid() const { return mGid; }

    /** Sets the uid of the file. */
    void setUid(uid_t id)
        { mUid = id; mFlags |= SSH2_FILEXFER_ATTR_UIDGID; }

    /** Neither the gid or uid file attributes will not be included in the UDSEntry
        or when the file attribute is written to the sftp packet. This is
        equivalent to clearGid() */
    void clearUid()
        { mUid = 0; mFlags &= SSH2_FILEXFER_ATTR_UIDGID; }

    /** Returns the user id of the file. */
    gid_t uid() const { return mUid; }

    /** Set the modificatoin time of the file in seconds since Jan. 1, 1970. */
    void setMtime(time_t t)
        { mMtime = t; mFlags |= SSH2_FILEXFER_ATTR_ACMODTIME; }

    /** Neither the mtime or atime file attributes will not be included in the UDSEntry
        or when the file attribute is written to the sftp packet. This is
        equivalent to clearAtime() */
    void clearMtime()
        { mMtime = 0; mFlags &= SSH2_FILEXFER_ATTR_ACMODTIME; }

    /** Returns the modification time of the file in seconds since Jan. 1, 1970. */
    time_t mtime() const { return mMtime; }

    /** Sets the access time of the file in seconds since Jan. 1, 1970. */
    void setAtime(time_t t)
        { mAtime = t; mFlags |= SSH2_FILEXFER_ATTR_ACMODTIME; }

    /** Neither the atime or mtime file attributes will not be included in the UDSEntry
        or when the file attribute is written to the sftp packet. This is
        equivalent to clearMtime() */
    void clearAtime()
        { mAtime = 0; mFlags &= SSH2_FILEXFER_ATTR_ACMODTIME; }

    /** Returns the last access time of the file in seconds since Jan. 1, 1970. */
    time_t atime() const { return mAtime; }

    /** Sets the number of file attribute extensions. */
    void setExtendedCount(unsigned int c)
        { mExtendedCount = c; mFlags |= SSH2_FILEXFER_ATTR_EXTENDED; }

    /** No extensions will be included when the file attribute is written
        to a sftp packet. */
    void clearExtensions()
        { mExtendedCount = 0; mFlags &= ~SSH2_FILEXFER_ATTR_EXTENDED; }

    /** Returns the number of file attribute extentsions. */
    unsigned int extendedCount() const { return mExtendedCount; }

    /** Returns the flags for the sftp file attributes. */
    unsigned int flags() const { return mFlags; }

    /** Sets file's longname. See sftpFileAttr::longname. */
    void setLongname(QString l) { mLongname = l.toLatin1(); }

    /** Returns a string describing the file attributes. The format is specific
        to the implementation of the sftp server.  In most cases (ie OpenSSH)
        this is similar to the long output of 'ls'. */
    QString longname() const { return mLongname; }

    void setLinkDestination(const QString& target)
        { mLinkDestination = target; }

    QString linkDestination()
        { return mLinkDestination; }

    /** Sets the actual type a symbolic link points to. */
    void setLinkType (mode_t type) { mLinkType = type; }

    mode_t linkType() const { return mLinkType; }

    /** No descriptions */
    void setFilename(const QString& fn)
        { mFilename = fn; }

    QString filename() const
        { return mFilename; }

    /** Returns a UDSEntry describing the file.
       The UDSEntry is generated from the sftp file attributes. */
    KIO::UDSEntry entry();

    /** Use to output the file attributes to a sftp packet
        This will only write the sftp ATTR structure to the stream.
        It will never write the filename and longname because the client
        never sends those to the server. */
    friend QDataStream& operator<< (QDataStream&, const sftpFileAttr&);

    /** Use to read a file attribute from a sftp packet.
        Read this carefully! If the DirAttrs flag is true, this will
        read the filename, longname, and file attributes from the stream.
        This is for use with listing directories.
        If the DirAttrs flag is false, this will only read file attributes
        from the stream.
        BY DEFAULT, A NEW INSTANCE HAS DirAttrs == false */
    friend QDataStream& operator>> (QDataStream&, sftpFileAttr&);

    /** Parse longname for the owner and group names. */
    void getUserGroupNames();

    /** Sets the DirAttrs flag.  This flag affects how the >> operator works on data streams. */
    void setDirAttrsFlag(bool flag){ mDirAttrs = flag; }

    /** Gets the DirAttrs flag. */
    bool getDirAttrsFlag() const { return mDirAttrs; }

    friend kdbgstream& operator<< (kdbgstream& s, sftpFileAttr& a);
    friend kndbgstream& operator<< (kndbgstream& s, sftpFileAttr& a);

    /** Returns the file type as determined from the file permissions */
    mode_t fileType() const;

    /** Set the encoding of the remote file system */
    void setEncoding( KRemoteEncoding* encoding );
};

#endif
