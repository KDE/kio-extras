/*
  Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>
  Copyright (C) 2000, 2001 Michael Matz <matz@kde.org>
  Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>

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

#ifdef HAVE_LAME
#include <lame/lame.h>
#endif

#ifdef HAVE_VORBIS
#include <time.h>
#include <vorbis/vorbisenc.h>
#define KDE_VORBIS_TAG "Track encoded by KDE 2.x"
#endif

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

enum Which_dir { Unknown = 0, Device, ByName, ByTrack, Title, Info, Root,
                 MP3, Vorbis };

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
      s_mp3  = "MP3";
      s_vorbis = "Ogg Vorbis";
    }

    void clear()
    {
      path = QString::null;
      paranoiaLevel = 2;
      useCDDB = true;
      cddbServer = "freedb.freedb.org";
      cddbPort = 888;
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
    QString s_mp3;
    QString s_vorbis;

#ifdef HAVE_LAME
    lame_global_flags *gf;
#endif

#ifdef HAVE_VORBIS
    ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
    ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
    ogg_packet       op; /* one raw packet of data for decode */
    
    vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
    vorbis_comment   vc; /* struct that stores all the user comments */

    vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
    vorbis_block     vb; /* local working space for packet->PCM decode */
#endif

    Which_dir which_dir;
    int req_track;
    QString fname;
};

AudioCDProtocol::AudioCDProtocol (const QCString & pool, const QCString & app)
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

static QString determineFiletype(QString filename)
{
    int len = filename.length();
    int pos = filename.findRev(".",-1);
    return filename.right(len - pos - 1);
}

struct cdrom_drive *
AudioCDProtocol::initRequest(const KURL & url)
{

#ifdef HAVE_LAME
  if (NULL == (d->gf = lame_init())) { // init the lame_global_flags structure with defaults
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return 0;
  }

  d->gf->brate = 128;
  id3tag_init (d->gf);
#endif

#ifdef HAVE_VORBIS
  /* choose an encoding mode */
  /* (mode 0: 44kHz stereo uncoupled, roughly 128kbps VBR) */
  vorbis_info_init(&d->vi);
  vorbis_encode_init(&d->vi, 2, 44100, -1, 128000, -1);

  vorbis_comment_init(&d->vc);
  char *kde_vorbis_comment = KDE_VORBIS_TAG;
  vorbis_comment_add(&d->vc, kde_vorbis_comment);
#endif

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
       d->fname == d->s_mp3 || d->fname == d->s_vorbis || d->fname == "dev"))
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
  else if (dname == d->s_mp3)
    d->which_dir = MP3;
  else if (dname == d->s_vorbis)
    d->which_dir = Vorbis;
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

 QString filetype = determineFiletype(d->fname);

#ifdef HAVE_LAME
  if (filetype == "mp3" && d->based_on_cddb) {
    /* If CDDB is used to determine the filenames, tell lame to append ID3v1 TAG to MP3 Files */
    const char *tname =   d->titles[trackNumber-1].latin1();    // set trackname
    id3tag_set_album(d->gf, d->cd_title.latin1());
    id3tag_set_artist(d->gf, d->cd_artist.latin1());
    id3tag_set_title(d->gf, tname+3); // since titles has preleading tracknumbers, start at position 3
  }


  if (lame_init_params(d->gf) < 0) { // tell lame the new parameters
    kdDebug(7101) << "lame init params failed" << endl;
    return;
  }
#endif

#ifdef HAVE_VORBIS
  if (filetype == "ogg" && d->based_on_cddb ) {
    const char *tname =   d->titles[trackNumber-1].latin1(); 
    vorbis_comment_add_tag(&d->vc,"title",tname+3);
    vorbis_comment_add_tag(&d->vc,"artist",d->cd_artist.latin1());
    vorbis_comment_add_tag(&d->vc,"album",d->cd_title.latin1());
  }
#endif

 
  long firstSector    = cdda_track_firstsector(drive, trackNumber);
  long lastSector     = cdda_track_lastsector(drive, trackNumber);
  long totalByteCount = CD_FRAMESIZE_RAW * (lastSector - firstSector);

#ifdef HAVE_LAME
  long time_secs = (8 * totalByteCount) / (44100 * 2 * 16);

  if (filetype == "mp3")
    totalSize((time_secs * d->gf->brate * 1000)/8);
#endif

#ifdef HAVE_VORBIS
  if (filetype == "ogg") {
    // TODO: Is there a way to forecast the filelength at VBR????
    totalSize(totalByteCount);
  }
#endif

  if (filetype == "wav") {
    totalSize(44 + totalByteCount); // Include RIFF header length.
    writeHeader(totalByteCount);    // Write RIFF header.
  }

  if (filetype == "cda")
    totalSize(totalByteCount);      // CDA is raw interleaved PCM Data with SampleRate 44100 and 16 Bit res. 

  paranoiaRead(drive, firstSector, lastSector, filetype);

  data(QByteArray());   // send an empty QByteArray to signal end of data.

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
      QString filetype = determineFiletype(d->fname);

      long filesize = CD_FRAMESIZE_RAW * (
            cdda_track_lastsector(drive, trackNumber) -
            cdda_track_firstsector(drive, trackNumber)
      );

      long length_seconds = (filesize) / 176400;
#ifdef HAVE_LAME
      if (filetype == "mp3")
        atom.m_long = (length_seconds * d->gf->brate*1000) / 8;
#endif

#ifdef HAVE_VORBIS
      if (filetype == "ogg")
        atom.m_long = (length_seconds * d->vi.bitrate_nominal) / 8;
#endif

      if (filetype == "cda") atom.m_long = filesize;

      if (filetype == "wav") atom.m_long = filesize + 44;
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

  if (d->useCDDB)
    {
      d->cddb->set_server(d->cddbServer.latin1(), d->cddbPort);

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
    }

  d->based_on_cddb = false;
  for (int i = 0; i < d->tracks; i++)
    {
      QString num;
      int ti = i + 1;
      QString s;
      num.sprintf("%02d", ti);
      if (IS_AUDIO(drive, ti))
        s = d->s_track.arg(num);
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

#ifdef HAVE_LAME
      app_dir(entry, d->s_mp3, d->tracks);
      listEntry(entry, false);
#endif

#ifdef HAVE_VORBIS
      app_dir(entry, d->s_vorbis, d->tracks);
      listEntry(entry, false);
#endif

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
        QString s,s2,s3;
        QString num2;

        long size = CD_FRAMESIZE_RAW *
          ( cdda_track_lastsector(drive, i) - cdda_track_firstsector(drive, i));
        long length_seconds = size / 176400;

        /*if (i==1)
          s.sprintf("_%08x.wav", d->discid);
        else*/
          s.sprintf(".wav");
        s2.sprintf(".mp3");
        s3.sprintf(".ogg");
        num2.sprintf("%02d", i);

        QString name;
        switch (d->which_dir)
          {
            case Device:
            case Root: name.sprintf("track%02d.cda", i); break;
            case ByTrack: name = d->s_track.arg(num2) + s; break;
#ifdef HAVE_LAME
            case MP3:
              name = d->titles[i - 1] + s2;
              size = (length_seconds * d->gf->brate*1000) / 8; // length * bitrate / 8;
              break;
#endif

#ifdef HAVE_VORBIS
            case Vorbis:
              name = d->titles[i - 1] + s3;
              size = (length_seconds * d->vi.bitrate_nominal) / 8; // length * bitrate / 8; 
              break;
#endif

            case ByName:
            case Title: name = d->titles[i - 1] + s; break;
            case Info:
            case Unknown:
            default:
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
  QString old_cddb_server = d->cddbServer;
  int old_cddb_port = d->cddbPort;
  bool old_use_cddb = d->useCDDB;

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
#ifdef HAVE_LAME
    else if (attribute == "br")
    {
        d->gf->brate = value.toLong();
    }
#endif
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

  /* We need to recheck the CD, if the user either enabled CDDB now, or
     changed the server (port).  We simply reset the saved discid, which
     forces a reread of CDDB information.  */

  if ((old_use_cddb != d->useCDDB && d->useCDDB == true)
      || old_cddb_server != d->cddbServer
      || old_cddb_port != d->cddbPort)
    d->discid = 0;

  kdDebug(7101) << "CDDB: use_cddb = " << d->useCDDB << endl;
}

  void
AudioCDProtocol::paranoiaRead(
    struct cdrom_drive * drive,
    long firstSector,
    long lastSector,
    QString filetype
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

#ifdef HAVE_LAME
#define mp3buffer_size  8000
static char mp3buffer[mp3buffer_size];
#endif

  long processed(0);
  long currentSector(firstSector);

#ifdef HAVE_VORBIS
  if (filetype == "ogg") {
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    vorbis_analysis_init(&d->vd,&d->vi);
    vorbis_block_init(&d->vd,&d->vb);

    srand(time(NULL));
    ogg_stream_init(&d->os,rand());

    vorbis_analysis_headerout(&d->vd,&d->vc,&header,&header_comm,&header_code);

    ogg_stream_packetin(&d->os,&header); 
    ogg_stream_packetin(&d->os,&header_comm);
    ogg_stream_packetin(&d->os,&header_code);

    while (int result = ogg_stream_flush(&d->os,&d->og)) {

      if (!result) break;

      QByteArray output;

      char * oggheader = reinterpret_cast<char *>(d->og.header);
      char * oggbody = reinterpret_cast<char *>(d->og.body);

      output.setRawData(oggheader, d->og.header_len);
      data(output);
      output.resetRawData(oggheader, d->og.header_len);

      output.setRawData(oggbody, d->og.body_len);
      data(output);
      output.resetRawData(oggbody, d->og.body_len);

    }
  }
#endif

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

#ifdef HAVE_LAME
      if ( filetype == "mp3" ) {
         int mp3bytes = lame_encode_buffer_interleaved(d->gf,buf,CD_FRAMESAMPLES,mp3buffer,(int)mp3buffer_size);

         if (mp3bytes < 0 ) {
            kdDebug(7101) << "lame encoding failed" << endl;
            break;
         }

         if (mp3bytes > 0) {
           QByteArray output;

           output.setRawData(mp3buffer, mp3bytes);
           data(output);
           output.resetRawData(mp3buffer, mp3bytes);
           processed += mp3bytes;
         }
      }
#endif

#ifdef HAVE_VORBIS
      if (filetype == "ogg") {
        int i;
        float **buffer=vorbis_analysis_buffer(&d->vd,CD_FRAMESAMPLES);

        /* uninterleave samples */
        for(i=0;i<CD_FRAMESAMPLES;i++){
          buffer[0][i]=buf[2*i]/32768.0;
          buffer[1][i]=buf[2*i+1]/32768.0;
        }

        vorbis_analysis_wrote(&d->vd,i);

        while(vorbis_analysis_blockout(&d->vd,&d->vb)==1) {
          vorbis_analysis(&d->vb,&d->op);
          ogg_stream_packetin(&d->os,&d->op);

          while(int result=ogg_stream_pageout(&d->os,&d->og)) {

            if (!result) break;

            QByteArray output;

            char * oggheader = reinterpret_cast<char *>(d->og.header);
            char * oggbody = reinterpret_cast<char *>(d->og.body);

            output.setRawData(oggheader, d->og.header_len);
            data(output);
            output.resetRawData(oggheader, d->og.header_len);

            output.setRawData(oggbody, d->og.body_len);
            data(output);
            output.resetRawData(oggbody, d->og.body_len);
          }
        }
        processed += CD_FRAMESIZE_RAW;
      }
#endif

      if (filetype == "wav" || filetype == "cda") {
        QByteArray output;
        char * cbuf = reinterpret_cast<char *>(buf);
        output.setRawData(cbuf, CD_FRAMESIZE_RAW);
        data(output);
        output.resetRawData(cbuf, CD_FRAMESIZE_RAW);
        processed += CD_FRAMESIZE_RAW;
      }


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
#ifdef HAVE_LAME
  if (filetype == "mp3") {
     int mp3bytes = lame_encode_finish(d->gf,mp3buffer,(int)mp3buffer_size);

     if (mp3bytes < 0 ) {
       kdDebug(7101) << "lame encoding failed" << endl;
     }

     if (mp3bytes > 0) {
       QByteArray output;
       output.setRawData(mp3buffer, mp3bytes);
       data(output);
       output.resetRawData(mp3buffer, mp3bytes);
     }
  }
#endif

#ifdef HAVE_VORBIS
  if (filetype == "ogg") {
    ogg_stream_clear(&d->os);
    vorbis_block_clear(&d->vb);
    vorbis_dsp_clear(&d->vd);
    vorbis_info_clear(&d->vi);
  }
#endif

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
