/***************************************************************************
                          sftpfileattr.cpp  -  description
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

#include "sftpfileattr.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <qstring.h>
#include <qdatastream.h>

#include <kio/global.h>
#include <kremoteencoding.h>

using namespace KIO;

sftpFileAttr::sftpFileAttr(){
    clear();
    mDirAttrs = false;
}

sftpFileAttr::sftpFileAttr(const char* encoding){
    clear();
    mDirAttrs = false;
    mEncoding = encoding;
}


sftpFileAttr::~sftpFileAttr(){
}

/** Constructor to initialize the file attributes on declaration. */
sftpFileAttr::sftpFileAttr(Q_UINT64 size, uid_t uid, gid_t gid,
                    mode_t permissions, time_t atime,
                    time_t mtime, Q_UINT32 extendedCount) {
    clear();
    mDirAttrs = false;
    mSize  = size;
    mUid   = uid;
    mGid   = gid;
    mAtime = atime;
    mMtime = mtime;
    mPermissions   = permissions;
    mExtendedCount = extendedCount;
}

/** Returns a UDSEntry describing the file.
The UDSEntry is generated from the sftp file attributes. */
UDSEntry sftpFileAttr::entry() {
    UDSEntry entry;
    UDSAtom atom;
   
    atom.m_uds = UDS_NAME;
    atom.m_str = mFilename;
    entry.append(atom);

    if( mFlags & SSH2_FILEXFER_ATTR_SIZE ) {
        atom.m_uds = UDS_SIZE;
        atom.m_long = mSize;
        entry.append(atom);
    }

    if( mFlags & SSH2_FILEXFER_ATTR_ACMODTIME ) {
        atom.m_uds = UDS_ACCESS_TIME;
        atom.m_long = mAtime;
        entry.append(atom);

        atom.m_uds = UDS_MODIFICATION_TIME;
        atom.m_long = mMtime;
        entry.append(atom);
    }

    if( mFlags & SSH2_FILEXFER_ATTR_UIDGID ) {
        if( mUserName.isEmpty() || mGroupName.isEmpty() )
            getUserGroupNames();

        atom.m_uds = UDS_USER;
        atom.m_str = mUserName;
        entry.append(atom);

        atom.m_uds = UDS_GROUP;
        atom.m_str = mGroupName;
        entry.append(atom);
    }

    if( mFlags & SSH2_FILEXFER_ATTR_PERMISSIONS ) {
        atom.m_uds = UDS_ACCESS;
        atom.m_long = mPermissions;
        entry.append(atom);

        mode_t type = fileType();
        
        // Set the type if we know what it is
        if( type != 0 ) {
            atom.m_uds = UDS_FILE_TYPE;
            atom.m_long = (mLinkType ? mLinkType:type);
            entry.append(atom);
        }
        
        if( S_ISLNK(type) ) {
            atom.m_uds = UDS_LINK_DEST;
            atom.m_str = mLinkDestination;
            entry.append(atom);
        }
    }

    return entry;
}

/** Use to output the file attributes to a sftp packet */
QDataStream& operator<< (QDataStream& s, const sftpFileAttr& fa) {
    s << (Q_UINT32)fa.mFlags;

    if( fa.mFlags & SSH2_FILEXFER_ATTR_SIZE )
        { s << (Q_UINT64)fa.mSize; }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_UIDGID )
        { s << (Q_UINT32)fa.mUid << (Q_UINT32)fa.mGid; }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_PERMISSIONS )
        { s << (Q_UINT32)fa.mPermissions; }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_ACMODTIME )
        { s << (Q_UINT32)fa.mAtime << (Q_UINT32)fa.mMtime; }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_EXTENDED ) {
        s << (Q_UINT32)fa.mExtendedCount;
        // XXX: Write extensions to data stream here
        // s.writeBytes(extendedtype).writeBytes(extendeddata);
    }
    return s;
}


/** Use to read a file attribute from a sftp packet */
QDataStream& operator>> (QDataStream& s, sftpFileAttr& fa) {
    // XXX set all member variable to defaults

    // XXX Add some error checking in here in case
    //     we get a bad sftp packet.
    fa.clear();
    
    QByteArray fn;
    Q_UINT64 size;
    if( fa.mDirAttrs ) {
        s >> fn; 

        KRemoteEncoding encoder ( fa.mEncoding.data() );
        fa.mFilename = encoder.decode( QCString(fn.data(), fn.size()+1) );

        s >> fa.mLongname;
        size = fa.mLongname.size();
        fa.mLongname.resize(size+1);
        fa.mLongname[size] = 0;
        //kdDebug() << ">> sftpfileattr filename (" << fa.mFilename.size() << ")= " << fa.mFilename <<  endl;
    }

    Q_UINT32 x;
    s >> fa.mFlags;  // get flags

    if( fa.mFlags & SSH2_FILEXFER_ATTR_SIZE ) {
        // since we don't have a 64 bit int, ignore the top byte.
        // Very bad if we get a > 2^32 size
        s >> x;
        s >> x; fa.setFileSize(x);
    }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_UIDGID ) {
        s >> x; fa.setUid(x);
        s >> x; fa.setGid(x);
    }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_PERMISSIONS )
        { s >> x; fa.setPermissions(x); }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_ACMODTIME ) {
        s >> x; fa.setAtime(x);
        s >> x; fa.setMtime(x);
    }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_EXTENDED ) {
        s >> x; fa.setExtendedCount(x);
        // XXX: Read in extensions from data stream here
        // s.readBytes(extendedtype).readBytes(extendeddata);
    }
    fa.getUserGroupNames();
    return s;
}
/** Parse longname for the owner and group names. */
void sftpFileAttr::getUserGroupNames(){
    // Get the name of the owner and group of the file from longname.
    QString user, group;
    if( mLongname.isEmpty() ) {
        // do not have the user name so use the user id instead
        user.setNum(mUid);
        group.setNum(mGid);
    }
    else {
        int field = 0;
        int i = 0;
        int l = mLongname.length();
        
        KRemoteEncoding encoder( mEncoding.data() );
        QString longName = encoder.decode( mLongname );
        
        kdDebug(7120) << "Decoded:  " << longName << endl;
        
        // Find the beginning of the third field which contains the user name.
        while( field != 2 ) {
            if( longName[i].isSpace() ) {
                field++; i++;
                while( i < l && longName[i].isSpace() ) { i++; }
            }
            else { i++; }
        }
        // i is the index of the first character of the third field.
        while( i < l && !longName[i].isSpace() ) {
            user.append(longName[i]);
            i++;
        }

        // i is the first character of the space between fields 3 and 4
        // user contains the owner's user name
        while( i < l && longName[i].isSpace() ) {
            i++;
        }
        
        // i is the first character of the fourth field
        while( i < l && !longName[i].isSpace() ) {
            group.append(longName[i]);
            i++;
        }
        // group contains the name of the group.
    }
    
    mUserName = user;
    mGroupName = group;
}

/** No descriptions */
kdbgstream& operator<< (kdbgstream& s, sftpFileAttr& a) {
    s << "Filename: " << a.mFilename
      << ", Uid: " << a.mUid 
      << ", Gid: " << a.mGid
      << ", Username: " << a.mUserName 
      << ", GroupName: " << a.mGroupName
      << ", Permissions: " << a.mPermissions 
      << ", size: " << a.mSize
      << ", atime: " << a.mAtime 
      << ", mtime: " << a.mMtime
      << ", extended cnt: " << a.mExtendedCount;
    
    if (S_ISLNK(a.mLinkType)) {
      s << ", Link Type: " << a.mLinkType;
      s << ", Link Destination: " << a.mLinkDestination;
    }
      
    return s;
}

/** Make sure it builds with NDEBUG */
kndbgstream& operator<< (kndbgstream& s, sftpFileAttr& ) {
    return s;
}

/** Clear all attributes and flags. */
void sftpFileAttr::clear(){
    clearAtime();
    clearMtime();
    clearGid();
    clearUid();
    clearFileSize();
    clearPermissions();
    clearExtensions();
    mFilename = QString::null;
    mGroupName =  QString::null;
    mUserName = QString::null;
    mLinkDestination = QString::null;    
    mFlags = 0;
    mLongname = "\0";
    mLinkType = 0;
}

/** Return the size of the sftp attribute. */
Q_UINT32 sftpFileAttr::size() const{
    Q_UINT32 size = 4; // for the attr flag
    if( mFlags & SSH2_FILEXFER_ATTR_SIZE )
        size += 8;

    if( mFlags & SSH2_FILEXFER_ATTR_UIDGID )
        size += 8;

    if( mFlags & SSH2_FILEXFER_ATTR_PERMISSIONS )
        size += 4;

    if( mFlags & SSH2_FILEXFER_ATTR_ACMODTIME )
        size += 8;

    if( mFlags & SSH2_FILEXFER_ATTR_EXTENDED ) {
        size += 4;
        // add size of extensions
    }
    return size;
}

/** Returns the file type as determined from the file permissions */
mode_t sftpFileAttr::fileType() const{
    mode_t type = 0;
    
    if( S_ISLNK(mPermissions) )
      type |= S_IFLNK;
      
    if( S_ISREG(mPermissions) )
      type |= S_IFREG;
    else if( S_ISDIR(mPermissions) )
      type |= S_IFDIR;
    else if( S_ISCHR(mPermissions) )
      type |= S_IFCHR;
    else if( S_ISBLK(mPermissions) )
      type |= S_IFBLK;
    else if( S_ISFIFO(mPermissions) )
      type |= S_IFIFO;
    else if( S_ISSOCK(mPermissions) )
      type |= S_IFSOCK;
    
    return type;
}

void sftpFileAttr::setEncoding( const char* encoding )
{
    mEncoding = encoding;
}
// vim:ts=4:sw=4
