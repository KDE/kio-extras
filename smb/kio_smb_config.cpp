/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE2
//
// File:        kio_smb_config.cpp
//
// Abstract:    member function implementations for SMBSlave that deal with
//              KDE/SMB slave configuration
//
// Author(s):   Matthew Peterson <mpeterson@caldera.com>
//
//---------------------------------------------------------------------------
//
// Copyright (c) 2000  Caldera Systems, Inc.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program; see the file COPYING.  If not, please obtain
//     a copy from http://www.gnu.org/copyleft/gpl.html
//
/////////////////////////////////////////////////////////////////////////////

#include "kio_smb.h"
#include "kio_smb_internal.h"
#include <kconfig.h>
#include <kconfiggroup.h>

#include <QTextCodec>
//===========================================================================
void SMBSlave::reparseConfiguration()
{
  KConfig cfg("kioslaverc", KConfig::CascadeConfig);
  const KConfigGroup group = cfg.group("Browser Settings/SMBro");
  m_default_user = group.readEntry("User");
//  m_default_workgroup=group.readEntry("Workgroup");
//  m_showHiddenShares=group.readEntry("ShowHiddenShares", QVariant(false)).toBool();

  QString m_encoding = QTextCodec::codecForLocale()->name();
  m_default_encoding = group.readEntry( "Encoding", m_encoding.toLower() );

  // unscramble, taken from Nicola Brodu's smb ioslave
  //not really secure, but better than storing the plain password
  QString scrambled = group.readEntry( "Password" );
  m_default_password = "";
  for (int i=0; i<scrambled.length()/3; i++)
  {
     QChar qc1 = scrambled[i*3];
     QChar qc2 = scrambled[i*3+1];
     QChar qc3 = scrambled[i*3+2];
     unsigned int a1 = qc1.toLatin1() - '0';
     unsigned int a2 = qc2.toLatin1() - 'A';
     unsigned int a3 = qc3.toLatin1() - '0';
     unsigned int num = ((a1 & 0x3F) << 10) | ((a2& 0x1F) << 5) | (a3 & 0x1F);
     m_default_password[i] = QChar((uchar)((num - 17) ^ 173)); // restore
  }
}
