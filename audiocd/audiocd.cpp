/*
  Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>
  Copyright (C) 2000 Michael Matz <matz@kde.org>

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
#include "cddb.h"

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

enum Which_dir { Unknown = 0, Device, ByName, ByTrack, Title, Info, Root };

class AudioCDProtocol::Private
{
  public:

    Private()
    {
      clear();
      discid = 0;
      cddb = 0;
      based_on_cddb = false;
      s_byname = i18n("By Name");
      s_bytrack = i18n("By Track");
      s_track = i18n("Track %1");
      s_info = i18n("Information");
    }

    void clear()
    {
      path = QString::null;
      paranoiaLevel = 2;
      useCDDB = false;
      cddbServer = QString::null;
      cddbPort = -1;
      which_dir = Unknown;
      req_track = -1;
    }

    QString path;
    int paranoiaLevel;
    bool useCDDB;
    QString cddbServer;
    int cddbPort;
    unsigned int discid;
    int tracks;
    QString cd_title;
    QString cd_artist;
    QStringList titles;
    bool is_audio[100];
    CDDB *cddb;
    bool based_on_cddb;
    QString s_byname;
    QString s_bytrack;
    QString s_track;
    QString s_info;

    Which_dir which_dir;
    int req_track;
    QString fname;
};

AudioCDProtocol::AudioCDProtocol(const QCString & pool, const QCString & app)
  : SlaveBase("audiocd", pool, app)
{
  d = new Private;
  d->cddb = new CDDB;
}

AudioCDProtocol::~AudioCDProtocol()
{
  delete d->cddb;
  delete d;
}

struct cdrom_drive *
AudioCDProtocol::initRequest(const KURL & url)
{
  parseArgs(url);

  struct cdrom_drive * drive = pickDrive();

  if (0 == drive)
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return 0;
  }

  if (0 != cdda_open(drive))
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return 0;
  }

  updateCD(drive);
  d->fname = url.filename(false);
  QString dname = url.directory(true, false);
  if (!dname.isEmpty() && dname[0] == '/')
    dname = dname.mid(1);

  /* A hack, for when konqi wants to list the directory audiocd:/Bla
     it really submits this URL, instead of audiocd:/Bla/ to us. We could
     send (in listDir) the UDS_NAME as "Bla/" for directories, but then
     konqi shows them as "Bla//" in the status line.  */
  if (dname.isEmpty() &&
      (d->fname == d->cd_title || d->fname == d->s_byname ||
       d->fname == d->s_bytrack || d->fname == d->s_info ||
       d->fname == "dev"))
    {
      dname = d->fname;
      d->fname = "";
    }

  if (dname.isEmpty())
    d->which_dir = Root;
  else if (dname == d->cd_title)
    d->which_dir = Title;
  else if (dname == d->s_byname)
    d->which_dir = ByName;
  else if (dname == d->s_bytrack)
    d->which_dir = ByTrack;
  else if (dname == d->s_info)
    d->which_dir = Info;
  else if (dname.left(4) == "dev/")
    {
      d->which_dir = Device;
      dname = dname.mid(4);
    }
  else if (dname == "dev")
    {
      d->which_dir = Device;
      dname = "";
    }  
  else
    d->which_dir = Unknown;

  d->req_track = -1;
  if (!d->fname.isEmpty())
    {
      QString n(d->fname);
      int pi = n.findRev('.');
      if (pi >= 0)
        n.truncate(pi);
      int i;
      for (i = 0; i < d->tracks; i++)
        if (d->titles[i] == n)
          break;
      if (i < d->tracks)
        d->req_track = i;
      else
        {
          /* Not found in title list.  Try hard to find a number in the
             string.  */
          unsigned int ui, j;
          ui = 0;
          while (ui < n.length())
            if (n[ui++].isDigit())
              break;
          for (j = ui; j < n.length(); j++)
            if (!n[j].isDigit())
              break;
          if (ui < n.length())
            {
              bool ok;
              /* The external representation counts from 1.  */
              d->req_track = n.mid(ui, j - i).toInt(&ok) - 1;
              if (!ok)
                d->req_track = -1;
            }
        }
    }
  if (d->req_track >= d->tracks)
    d->req_track = -1;

  kdDebug(7101) << "audiocd: dir=" << dname << " file=" << d->fname
    << " req_track=" << d->req_track << " which_dir=" << d->which_dir << endl;
  return drive;
}

  void
AudioCDProtocol::get(const KURL & url)
{
  struct cdrom_drive * drive = initRequest(url);
  if (!drive)
    return;

  int trackNumber = d->req_track + 1;

  if (trackNumber <= 0 || trackNumber > cdda_tracks(drive))
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
  struct cdrom_drive * drive = initRequest(url);
  if (!drive)
    return;

  bool isFile = !d->fname.isEmpty();

  int trackNumber = d->req_track + 1;

  if (isFile && (trackNumber < 1 || trackNumber > d->tracks))
    {
      error(KIO::ERR_DOES_NOT_EXIST, url.path());
      return;
    }

  UDSEntry entry;

  UDSAtom atom;
  atom.m_uds = KIO::UDS_NAME;
  atom.m_str = url.filename();
  entry.append(atom);

  atom.m_uds = KIO::UDS_FILE_TYPE;
  atom.m_long = isFile ? S_IFREG : S_IFDIR;
  entry.append(atom);

  atom.m_uds = KIO::UDS_ACCESS;
  atom.m_long = 0400;
  entry.append(atom);

  atom.m_uds = KIO::UDS_SIZE;
  if (!isFile)
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

  unsigned int
AudioCDProtocol::get_discid(struct cdrom_drive * drive)
{
  unsigned int id = 0;
  for (int i = 1; i <= drive->tracks; i++)
    {
      unsigned int n = cdda_track_firstsector (drive, i) + 150;
      n /= 75;
      while (n > 0)
        {
          id += n % 10;
          n /= 10;
        }
    }
  unsigned int l = (cdda_track_lastsector(drive, drive->tracks));
  l -= cdda_track_firstsector(drive, 1);
  l /= 75;
  id = ((id % 255) << 24) | (l << 8) | drive->tracks;
  return id;
}

void
AudioCDProtocol::updateCD(struct cdrom_drive * drive)
{
  unsigned int id = get_discid(drive);
  if (id == d->discid)
    return;
  d->discid = id;
  d->tracks = cdda_tracks(drive);
  d->cd_title = i18n("No Title");
  d->titles.clear();
  QValueList<int> qvl;
  for (int i = 0; i < d->tracks; i++)
    {
      d->is_audio[i] = IS_AUDIO (drive, i + 1);
      qvl.append(cdda_track_firstsector(drive, i + 1) + 150);
    }
  qvl.append(cdda_track_lastsector(drive, d->tracks) + 150 + 1);

  d->cddb->set_server("freedb.freedb.org", 888);

  if (d->cddb->queryCD(qvl))
    {
      d->based_on_cddb = true;
      d->cd_title = d->cddb->title();
      d->cd_artist = d->cddb->artist();
      for (int i = 0; i < d->tracks; i++)
        {
          QString n;
          n.sprintf("%02d ", i + 1);
          d->titles.append (n + d->cddb->track(i));
        }
      return;
    }

  d->based_on_cddb = false;
  for (int i = 0; i < d->tracks; i++)
    {
      int ti = i + 1;
      QString s;
      if (IS_AUDIO(drive, ti))
        s = d->s_track.arg(ti);
      else
        s.sprintf("data%02d", ti);
      d->titles.append( s );
    }
}

static void
app_entry(UDSEntry& e, unsigned int uds, const QString& str)
{
  UDSAtom a;
  a.m_uds = uds;
  a.m_str = str;
  e.append(a);
}

static void
app_entry(UDSEntry& e, unsigned int uds, long l)
{
  UDSAtom a;
  a.m_uds = uds;
  a.m_long = l;
  e.append(a);
}

static void
app_dir(UDSEntry& e, const QString & n, size_t s)
{
  e.clear();
  app_entry(e, KIO::UDS_NAME, n);
  app_entry(e, KIO::UDS_FILE_TYPE, S_IFDIR);
  app_entry(e, KIO::UDS_ACCESS, 0400);
  app_entry(e, KIO::UDS_SIZE, s);
}

static void
app_file(UDSEntry& e, const QString & n, size_t s)
{
  e.clear();
  app_entry(e, KIO::UDS_NAME, n);
  app_entry(e, KIO::UDS_FILE_TYPE, S_IFREG);
  app_entry(e, KIO::UDS_ACCESS, 0400);
  app_entry(e, KIO::UDS_SIZE, s);
}

  void
AudioCDProtocol::listDir(const KURL & url)
{
  struct cdrom_drive * drive = initRequest(url);
  if (!drive)
    return;

  UDSEntry entry;

  if (d->which_dir == Unknown)
    {
      error(KIO::ERR_DOES_NOT_EXIST, url.path());
      return;
    }

  if (!d->fname.isEmpty() && d->which_dir != Device)
    {
      error(KIO::ERR_IS_FILE, url.path());
      return;
    }

  /* XXX We can't handle which_dir == Device for now */

  bool do_tracks = true;

  if (d->which_dir == Root)
    {
      /* List our virtual directories.  */
      if (d->based_on_cddb)
        {
          app_dir(entry, d->s_byname, d->tracks);
          listEntry(entry, false);
        }
      app_dir(entry, d->s_info, 1);
      listEntry(entry, false);
      app_dir(entry, d->cd_title, d->tracks);
      listEntry(entry, false);
      app_dir(entry, d->s_bytrack, d->tracks);
      listEntry(entry, false);
      app_dir(entry, QString("dev"), 1);
      listEntry(entry, false);
    }
  else if (d->which_dir == Device && url.path().length() <= 5) // "/dev{/}"
    {
      app_dir(entry, QString("cdrom"), d->tracks);
      listEntry(entry, false);
      do_tracks = false;
    }
  else if (d->which_dir == Info)
    {
      /* List some text files */
      /* XXX */
      do_tracks = false;
    }

  if (do_tracks)
    for (int i = 1; i <= d->tracks; i++)
    {
      if (d->is_audio[i-1])
      {
        QString s;
        long size = CD_FRAMESIZE_RAW *
          ( cdda_track_lastsector(drive, i) - cdda_track_firstsector(drive, i));

        if (i==1)
          s.sprintf("_%08x.wav", d->discid);
        else
          s.sprintf(".wav");

        QString name;
        switch (d->which_dir)
          {
            case Device:
            case Root: name.sprintf("track%02d.cda", i); break;
            case ByTrack: name = d->s_track.arg(i) + s; break;
            case ByName:
            case Title: name = d->titles[i - 1] + s; break;
            case Info:
            case Unknown:
              error(KIO::ERR_INTERNAL, url.path());
              return;
          }
        app_file(entry, name, size);
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
