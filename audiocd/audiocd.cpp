/*
  Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include <qfile.h>
#include <qstrlist.h>
#include <qdatetime.h>

typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C"
{
#include <cdda_interface.h>
#include <cdda_paranoia.h>

void paranoiaCallback(long, int);
}

#include <kdebug.h>
#include <kurl.h>
#include <kprotocolmanager.h>
#include <kinstance.h>
#include <klocale.h>

#include "audiocd.h"

using namespace KIO;

#define MAX_IPC_SIZE (1024*32)

extern "C"
{
  int kdemain(int argc, char ** argv);
}

  int
kdemain(int argc, char ** argv)
{
  KInstance instance("kio_audiocd");

  kdDebug(7101) << "Starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr,
         "Usage: kio_audiocd protocol domain-socket1 domain-socket2\n"
     );

     exit(-1);
  }

  AudioCDProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();

  kdDebug(7101) << "Done" << endl;
  return 0;
}

class AudioCDProtocol::Private
{
  public:

    Private()
    {
      clear();
    }

    void clear()
    {
      path = QString::null;
      paranoiaLevel = 2;
      useCDDB = false;
      cddbServer = QString::null;
      cddbPort = -1;
    }

    QString path;
    int paranoiaLevel;
    bool useCDDB;
    QString cddbServer;
    int cddbPort;
};

AudioCDProtocol::AudioCDProtocol(const QCString & pool, const QCString & app)
  : SlaveBase("audiocd", pool, app)
{
  d = new Private;
}

AudioCDProtocol::~AudioCDProtocol()
{
  delete d;
}

  void
AudioCDProtocol::get(const KURL & url)
{
  parseArgs(url);

  struct cdrom_drive * drive = pickDrive();

  if (0 == drive)
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return;
  }

  if (0 != cdda_open(drive))
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return;
  }

  // Track name looks like this: trackNN.wav
  int trackNumber = url.filename().mid(5, 2).toInt();

  if (trackNumber < 0 || trackNumber > cdda_tracks(drive))
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return;
  }

  long firstSector    = cdda_track_firstsector(drive, trackNumber);
  long lastSector     = cdda_track_lastsector(drive, trackNumber);
  long totalByteCount = CD_FRAMESIZE_RAW * (lastSector - firstSector);

  totalSize(44 + totalByteCount); // Include RIFF header length.

  writeHeader(totalByteCount);

  paranoiaRead(drive, firstSector, lastSector);

  data(QByteArray());

  totalSize(44 + totalByteCount); // Include RIFF header length.

  cdda_close(drive);

  finished();
}

  void
AudioCDProtocol::stat(const KURL & url)
{
  parseArgs(url);

  struct cdrom_drive * drive = pickDrive();

  if (0 == drive)
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return;
  }

  if (0 != cdda_open(drive))
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return;
  }

  QString filename(url.path());

  bool isFile = filename.length() > 1;

  int trackNumber = 0;

  if (isFile)
  {
    // Track name looks like this: trackNN.wav
    trackNumber = url.filename().mid(5, 2).toInt();

    if (trackNumber < 0 || trackNumber > cdda_tracks(drive))
    {
      error(KIO::ERR_DOES_NOT_EXIST, filename);
      return;
    }
  }


  UDSEntry entry;

  UDSAtom atom;
  atom.m_uds = KIO::UDS_NAME;
  atom.m_str = url.path();
  entry.append(atom);

  atom.m_uds = KIO::UDS_FILE_TYPE;
  atom.m_long = isFile ? S_IFREG : S_IFDIR;
  entry.append(atom);

  atom.m_uds = KIO::UDS_ACCESS;
  atom.m_long = 0400;
  entry.append(atom);

  atom.m_uds = KIO::UDS_SIZE;
  if (isFile)
  {
    atom.m_long = cdda_tracks(drive);
  }
  else
  {
    atom.m_long = CD_FRAMESIZE_RAW * (
      cdda_track_lastsector(drive, trackNumber) -
      cdda_track_firstsector(drive, trackNumber)
    );
  }

  entry.append(atom);

  statEntry(entry);

  cdda_close(drive);

  finished();
}

  void
AudioCDProtocol::listDir(const KURL & url)
{
  parseArgs(url);

  struct cdrom_drive * drive = pickDrive();

  if (0 == drive)
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return;
  }

  if (0 != cdda_open(drive))
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return;
  }

  int trackCount = drive->tracks;

  QStrList entryNames;

  UDSEntry entry;

  for (int i = 0; i < trackCount; i++)
  {
    if (IS_AUDIO(drive, i))
    {
      QCString s;
      s.sprintf("track%02d.wav", i);

      entry.clear();
      UDSAtom atom;

      atom.m_uds = KIO::UDS_NAME;
      atom.m_str = s;
      entry.append(atom);

      atom.m_uds = KIO::UDS_FILE_TYPE;
      atom.m_long = S_IFREG;
      entry.append(atom);

      atom.m_uds = KIO::UDS_ACCESS;
      atom.m_long = 0400;
      entry.append(atom);

      atom.m_uds = KIO::UDS_SIZE;
      atom.m_long = CD_FRAMESIZE_RAW * (
          cdda_track_lastsector(drive, i) -
          cdda_track_firstsector(drive, i)
          );
      entry.append(atom);

      listEntry(entry, false);
    }
  }

  totalSize(entry.count());
  listEntry(entry, true);

  cdda_close(drive);

  finished();
}

  void
AudioCDProtocol::writeHeader(long byteCount)
{
  static char riffHeader[] =
  {
    0x52, 0x49, 0x46, 0x46, // 0  "AIFF"
    0x00, 0x00, 0x00, 0x00, // 4  wavSize
    0x57, 0x41, 0x56, 0x45, // 8  "WAVE"
    0x66, 0x6d, 0x74, 0x20, // 12 "fmt "
    0x10, 0x00, 0x00, 0x00, // 16
    0x01, 0x00, 0x02, 0x00, // 20
    0x44, 0xac, 0x00, 0x00, // 24
    0x10, 0xb1, 0x02, 0x00, // 28
    0x04, 0x00, 0x10, 0x00, // 32
    0x64, 0x61, 0x74, 0x61, // 36 "data"
    0x00, 0x00, 0x00, 0x00  // 40 byteCount
  };

  Q_INT32 wavSize(byteCount + 44 - 8);


  riffHeader[4]   = (wavSize   >> 0 ) & 0xff;
  riffHeader[5]   = (wavSize   >> 8 ) & 0xff;
  riffHeader[6]   = (wavSize   >> 16) & 0xff;
  riffHeader[7]   = (wavSize   >> 24) & 0xff;

  riffHeader[40]  = (byteCount >> 0 ) & 0xff;
  riffHeader[41]  = (byteCount >> 8 ) & 0xff;
  riffHeader[42]  = (byteCount >> 16) & 0xff;
  riffHeader[43]  = (byteCount >> 24) & 0xff;

  QByteArray output;
  output.setRawData(riffHeader, 44);
  data(output);
  output.resetRawData(riffHeader, 44);
  processedSize(44);
}

  struct cdrom_drive *
AudioCDProtocol::pickDrive()
{
  QCString path(QFile::encodeName(d->path));

  struct cdrom_drive * drive = 0;

  if (!path.isEmpty() && path != "/")
    drive = cdda_identify(path, CDDA_MESSAGE_PRINTIT, 0);

  else
  {
    drive = cdda_find_a_cdrom(CDDA_MESSAGE_PRINTIT, 0);

    if (0 == drive)
    {
      if (QFile("/dev/cdrom").exists())
        drive = cdda_identify("/dev/cdrom", CDDA_MESSAGE_PRINTIT, 0);
    }
  }

  return drive;
}

  void
AudioCDProtocol::parseArgs(const KURL & url)
{
  d->clear();

  QString query(KURL::decode_string(url.query()));

  if (query.isEmpty() || query[0] != '?')
    return;

  query = query.mid(1); // Strip leading '?'.

  QStringList tokens(QStringList::split('&', query));

  for (QStringList::ConstIterator it(tokens.begin()); it != tokens.end(); ++it)
  {
    QString token(*it);

    int equalsPos(token.find('='));

    if (-1 == equalsPos)
      continue;

    QString attribute(token.left(equalsPos));
    QString value(token.mid(equalsPos + 1));

    if (attribute == "device")
    {
      d->path = value;
    }
    else if (attribute == "paranoia_level")
    {
      d->paranoiaLevel = value.toInt();
    }
    else if (attribute == "use_cddb")
    {
      d->useCDDB = (0 != value.toInt());
    }
    else if (attribute == "cddb_server")
    {
      int portPos = value.find(':');

      if (-1 == portPos)
        d->cddbServer = value;

      else
      {
        d->cddbServer = value.left(portPos);
        d->cddbPort = value.mid(portPos + 1).toInt();
      }
    }
  }
}

  void
AudioCDProtocol::paranoiaRead(
    struct cdrom_drive * drive,
    long firstSector,
    long lastSector
)
{
  cdrom_paranoia * paranoia = paranoia_init(drive);

  if (0 == paranoia)
  {
    kdDebug(7101) << "paranoia_init failed" << endl;
    return;
  }

  int paranoiaLevel = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;

  switch (d->paranoiaLevel)
  {
    case 0:
      paranoiaLevel = PARANOIA_MODE_DISABLE;
      break;

    case 1:
      paranoiaLevel |=  PARANOIA_MODE_OVERLAP;
      paranoiaLevel &= ~PARANOIA_MODE_VERIFY;
      break;

    case 2:
      paranoiaLevel |= PARANOIA_MODE_NEVERSKIP;
    default:
      break;
  }

  paranoia_modeset(paranoia, paranoiaLevel);

  cdda_verbose_set(drive, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);

  paranoia_seek(paranoia, firstSector, SEEK_SET);

  long processed(0);
  long currentSector(firstSector);

  QTime timer;
  timer.start();

  int lastElapsed = 0;

  while (currentSector < lastSector)
  {
    int16_t * buf = paranoia_read(paranoia, paranoiaCallback);

    if (0 == buf)
    {
      kdDebug(7101) << "Unrecoverable error in paranoia_read" << endl;
      break;
    }
    else
    {
      ++currentSector;

      QByteArray output;
      char * cbuf = reinterpret_cast<char *>(buf);
      output.setRawData(cbuf, CD_FRAMESIZE_RAW);
      data(output);
      output.resetRawData(cbuf, CD_FRAMESIZE_RAW);

      processed += CD_FRAMESIZE_RAW;

      int elapsed = timer.elapsed() / 1000;

      if (elapsed != lastElapsed)
      {
        processedSize(processed);

        if (0 != elapsed)
          speed(processed / elapsed);
      }

      lastElapsed = elapsed;
    }
  }

  paranoia_free(paranoia);
  paranoia = 0;
}

  void
paranoiaCallback(long, int)
{
  // Do we want to show info somewhere ?
  // Not yet.
}

// vim:ts=2:sw=2:tw=78:et:
