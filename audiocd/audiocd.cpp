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
#include <qfileinfo.h>

typedef Q_INT16 size16;
typedef Q_INT32 size32;

#undef HAVE_LAME

extern "C"
{
#include <cdda_interface.h>
#include <cdda_paranoia.h>

// XXX: This is by Michael Matz.
// This is in support for the Mega Hack, if cdparanoia ever is fixed, or we
// use another ripping library we can remove this.

#include <linux/cdrom.h>
#include <sys/ioctl.h>

#ifdef HAVE_LAME

#include <lame/lame.h>
#endif

#ifdef HAVE_VORBIS

#include <time.h>
#include <vorbis/vorbisenc.h>

#endif // HAVE_VORBIS

  // Called by cdparanoia when it has something interesting to tell
  // us about the rip progress. We can actually get all the info we're
  // interested in when this is a stub, but perhaps we'll want to know
  // about scratches etc. in the future.

  void paranoiaCallback(long, int);
}

#include <kconfig.h>
#include <kdebug.h>
#include <kurl.h>
#include <kprotocolmanager.h>
#include <kinstance.h>
#include <klocale.h>

#include "audiocd.h"
#include "cddb.h"

using namespace KIO;

#define MAX_IPC_SIZE (1024*32)

static const char * defaultDevice     = "/dev/cdrom";
static const char * defaultCDDBServer = "freedb.freedb.org:8880";

extern "C"
{
  int kdemain(int argc, char ** argv);
  int FixupTOC(cdrom_drive *d, int tracks);
}

int start_of_first_data_as_in_toc;
int hack_track;

// XXX: This is by Michael Matz.
// Mega hack.  This function comes from libcdda_interface, and is called by
// it.  We need to override it, so we implement it ourself in the hope, that
// shared lib semantics make the calls in libcdda_interface to FixupTOC end
// up here, instead of it's own copy.  This usually works.
// You don't want to know the reason for this.

int FixupTOC(cdrom_drive *d, int tracks)
{
  int j;
  for (j = 0; j < tracks; j++) {
    if (d->disc_toc[j].dwStartSector < 0)
      d->disc_toc[j].dwStartSector = 0;
    if (j < tracks-1
      && d->disc_toc[j].dwStartSector > d->disc_toc[j+1].dwStartSector)
      d->disc_toc[j].dwStartSector = 0;
  }
  long last = d->disc_toc[0].dwStartSector;
  for (j = 1; j < tracks; j++) {
    if (d->disc_toc[j].dwStartSector < last)
      d->disc_toc[j].dwStartSector = last;
  }
  start_of_first_data_as_in_toc = -1;
  hack_track = -1;
  if (d->ioctl_fd != -1) {
    struct cdrom_multisession ms_str;
    ms_str.addr_format = CDROM_LBA;
    if (ioctl(d->ioctl_fd, CDROMMULTISESSION, &ms_str) == -1)
      return -1;
    if (ms_str.addr.lba > 100) {
      for (j = tracks-1; j >= 0; j--)
        if (j > 0 && !IS_AUDIO(d,j) && IS_AUDIO(d,j-1)) {
          if (d->disc_toc[j].dwStartSector > ms_str.addr.lba - 11400) {
            /* The next two code lines are the purpose of duplicating this
             * function, all others are an exact copy of paranoias FixupTOC().
             * The gory details: CD-Extra consist of N audio-tracks in the
             * first session and one data-track in the next session.  This
             * means, the first sector of the data track is not right behind
             * the last sector of the last audio track, so all length
             * calculation for that last audio track would be wrong.  For this
             * the start sector of the data track is adjusted (we don't need
             * the real start sector, as we don't rip that track anyway), so
             * that the last audio track end in the first session.  All well
             * and good so far.  BUT: The CDDB disc-id is based on the real
             * TOC entries so this adjustment would result in a wrong Disc-ID.
             * We can only solve this conflict, when we save the old
             * (toc-based) start sector of the data track.  Of course the
             * correct solution would be, to only adjust the _length_ of the
             * last audio track, not the start of the next track, but the
             * internal structures of cdparanoia are as they are, so the
             * length is only implicitely given.  Bloody sh*.  */
            start_of_first_data_as_in_toc = d->disc_toc[j].dwStartSector;
            hack_track = j + 1;
            d->disc_toc[j].dwStartSector = ms_str.addr.lba - 11400;
          }
          break;
        }
      return 1;
    }
  }
  return 0;
}

/* libcdda returns for cdda_disc_lastsector() the last sector of the last
   _audio_ track.  How broken.  For CDDB Disc-ID we need the real last sector
   to calculate the disc length.  */
long my_last_sector(cdrom_drive *drive)
{
  return cdda_track_lastsector(drive, drive->tracks);
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

  static void
appendEntry(UDSEntry& e, unsigned int uds, const QString& str)
{
  UDSAtom a;
  a.m_uds = uds;
  a.m_str = str;
  e.append(a);
}

  static void
appendEntry(UDSEntry& e, unsigned int uds, long l)
{
  UDSAtom a;
  a.m_uds = uds;
  a.m_long = l;
  e.append(a);
}

  static void
appendDir(UDSEntry& e, const QString & n, size_t s)
{
  e.clear();
  appendEntry(e, KIO::UDS_NAME, n);
  appendEntry(e, KIO::UDS_FILE_TYPE, S_IFDIR);
  appendEntry(e, KIO::UDS_ACCESS, 0400);
  appendEntry(e, KIO::UDS_SIZE, s);
}

  static void
appendFile(UDSEntry& e, const QString & n, size_t s)
{
  e.clear();
  appendEntry(e, KIO::UDS_NAME, n);
  appendEntry(e, KIO::UDS_FILE_TYPE, S_IFREG);
  appendEntry(e, KIO::UDS_ACCESS, 0400);
  appendEntry(e, KIO::UDS_SIZE, s);
}


class AudioCDProtocol::Private
{
  public:

    Private()
    {
      clear();

      discid        = 0;
      cddb          = 0;
      based_on_cddb = false;
      s_byname      = i18n("By Name");
      s_bytrack     = i18n("By Track");
      s_track       = i18n("Track %1");
      s_info        = i18n("Information");
      s_mp3         = "MP3";
      s_vorbis      = "Ogg Vorbis";
    }

    void clear()
    {
      dirType = DirTypeUnknown;
      requestedTrackOK = false;
    }

    QString       path;
    int           paranoiaLevel;
    bool          useCDDB;
    QString       cddbServer;
    uint          cddbPort;
    uint          discid;
    uint          trackCount;
    QString       cd_title;
    QString       cd_artist;
    QStringList   titles;
    bool          is_audio[100];
    CDDB        * cddb;
    bool          based_on_cddb;
    QString       s_byname;
    QString       s_bytrack;
    QString       s_track;
    QString       s_info;
    QString       s_mp3;
    QString       s_vorbis;

#ifdef HAVE_LAME
    lame_global_flags * gf;
    int                 bitrate;
    bool                write_id3;
#endif

#ifdef HAVE_VORBIS
    // Take physical pages, weld into a logical stream of packets.
    ogg_stream_state  oggStreamState;

    // One Ogg bitstream page.  Vorbis packets are inside.
    ogg_page          oggPage;

    // One raw packet of data for decode.
    ogg_packet        oggPacket;

    // struct that stores all the static vorbis bitstream settings.
    vorbis_info       vorbisInfo;

    // struct that stores all the user comments */
    vorbis_comment    vorbisComment;

    // Central working state for the packet->PCM decoder.
    vorbis_dsp_state  vorbisDSPState;

    // Local working space for packet->PCM decode.
    vorbis_block      vorbisBlock;

    bool              writeVorbisComments;
    long              vorbisBitrateLower;
    long              vorbisBitrateUpper;
    long              vorbisBitrateNominal;
    int               vorbisBitrate;
#endif

    DirType dirType;
    uint    requestedTrack;
    bool    requestedTrackOK;
    QString filename;
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


  AudioCDProtocol::FileType
AudioCDProtocol::fileType(const QString & filename)
{
  QString extension(filename.right(filename.findRev('.')));

  if ("ogg" == extension)
    return FileTypeOggVorbis;

  else if ("mp3" == extension)
    return FileTypeMP3;

  else if ("wav" == extension)
    return FileTypeWAV;

  else
    return FileTypeUnknown;
}

  struct cdrom_drive *
AudioCDProtocol::initRequest(const KURL & url)
{
#ifdef HAVE_LAME

  if (0 == (d->gf = lame_init()))
  {
    // init the lame_global_flags structure with defaults
    error(KIO::ERR_DOES_NOT_EXIST, url.url());
    return 0;
  }

  id3tag_init(d->gf);

#endif

#ifdef HAVE_VORBIS

  vorbis_info_init    (&d->vorbisInfo);
  vorbis_comment_init (&d->vorbisComment);

  vorbis_comment_add_tag
    (
     &d->vorbisComment,
     const_cast<char *>("kde-encoder"),
     const_cast<char *>(QString::fromUtf8("kio_audiocd").utf8().data())
    );

#endif

  // first get the parameters from the Kontrol Center Module
  getParameters();

  // then these parameters can be overruled by args in the URL
  parseArgs(url);


#ifdef HAVE_VORBIS

  vorbis_encode_init
    (
     &d->vorbisInfo,
     2,
     44100,
     d->vorbisBitrateUpper,
     d->vorbisBitrateNominal,
     d->vorbisBitrateLower
    );

#endif

  bool noPermission = false;

  struct cdrom_drive * drive = findDrive(noPermission);

  if (noPermission)
  {
    error(KIO::ERR_ACCESS_DENIED, url.url());
    return 0;
  }

  if (0 == drive)
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.url());
    return 0;
  }

  if (0 != cdda_open(drive))
  {
    error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.url());
    return 0;
  }

  updateCD(drive);

  d->filename   = url.filename(false);

  QString dirName = url.directory(true, false);

  if (!dirName.isEmpty() && dirName[0] == '/')
    dirName = dirName.mid(1);

  /* A hack, for when konqi wants to list the directory audiocd:/Bla
     it really submits this URL, instead of audiocd:/Bla/ to us. We could
     send (in listDir) the UDS_NAME as "Bla/" for directories, but then
     konqi shows them as "Bla//" in the status line.  */
  if
    (
     dirName.isEmpty()
     &&
     (
      d->filename == d->cd_title
      ||
      d->filename == d->s_byname
      ||
      d->filename == d->s_bytrack
      ||
      d->filename == d->s_info
      ||
      d->filename == d->s_mp3
      ||
      d->filename == d->s_vorbis
      ||
      d->filename == "dev"
     )
    )
  {
    dirName = d->filename;
    d->filename = "";
  }

  if (dirName.isEmpty())
  {
    d->dirType = DirTypeRoot;
  }
  else if (dirName == d->cd_title)
  {
    d->dirType = DirTypeTitle;
  }
  else if (dirName == d->s_byname)
  {
    d->dirType = DirTypeByName;
  }
  else if (dirName == d->s_bytrack)
  {
    d->dirType = DirTypeByTrack;
  }
  else if (dirName == d->s_info)
  {
    d->dirType = DirTypeInfo;
  }
  else if (dirName == d->s_mp3)
  {
    d->dirType = DirTypeMP3;
  }
  else if (dirName == d->s_vorbis)
  {
    d->dirType = DirTypeVorbis;
  }
  else if (dirName.left(4) == "dev/")
  {
    d->dirType = DirTypeDevice;
    dirName = dirName.mid(4);
  }
  else if (dirName == "dev")
  {
    d->dirType = DirTypeDevice;
    dirName = "";
  }
  else
  {
    d->dirType = DirTypeUnknown;
  }

  d->requestedTrackOK = false;

  if (!d->filename.isEmpty())
  {
    QString n(d->filename);

    int pi = n.findRev('.');

    if (pi >= 0)
      n.truncate(pi);

    for (uint i = 0; i < d->trackCount; i++)
    {
      if (d->titles[i] == n)
      {
        d->requestedTrack = i;
        break;
      }
    }

    if (!d->requestedTrackOK)
    {
      // Not found in title list. Try hard to find a number in the string.

      uint ui = 0;
      uint j  = 0;

      while (ui < n.length())
      {
        if (n[ui++].isDigit())
          break;
      }

      for (j = ui; j < n.length(); j++)
      {
        if (!n[j].isDigit())
          break;
      }

      if (ui < n.length())
      {
        // The external representation counts from 1.
        d->requestedTrack =
          n.mid(ui, j - d->requestedTrack).toInt(&d->requestedTrackOK) - 1;
      }
    }
  }

  if (d->requestedTrack >= d->trackCount)
  {
    d->requestedTrackOK = false;
  }

  kdDebug(7101) << "audiocd: dir=" << dirName << " file=" << d->filename
    << " req_track=" << d->requestedTrack << " dirType=" << d->dirType << endl;
  return drive;
}

  void
AudioCDProtocol::get(const KURL & url)
{
  struct cdrom_drive * drive = initRequest(url);

  if (0 == drive)
    return;

  if (!d->requestedTrackOK || d->requestedTrack + 1 > uint(cdda_tracks(drive)))
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.url());
    return;
  }

  int type = fileType(d->filename);

#ifdef HAVE_LAME

  if (FileTypeMP3 == type && d->based_on_cddb && d->write_id3)
  {
    // If CDDB is used to determine the filenames, tell lame to append
    // ID3v1 TAG to MP3 files.

    // Set track name.
    const char * tname = d->titles[d->requestedTrack - 1].latin1();

    id3tag_set_album  (d->gf, d->cd_title.latin1());
    id3tag_set_artist (d->gf, d->cd_artist.latin1());

    // Since titles have leading numbers, start at position 3.
    id3tag_set_title  (d->gf, tname + 3);
  }

  if (lame_init_params(d->gf) < 0)
  {
    // Tell lame the new parameters.

    kdDebug(7101) << "lame init params failed" << endl;
    return;
  }

#endif

#ifdef HAVE_VORBIS

  if
    (
     FileTypeOggVorbis == type
     &&
     d->based_on_cddb
     &&
     d->writeVorbisComments
    )
  {
    QString trackName(d->titles[d->requestedTrack - 1].mid(3));

    vorbis_comment_add_tag
      (
       &d->vorbisComment,
       const_cast<char *>("title"),
       const_cast<char *>(trackName.utf8().data())
      );

    vorbis_comment_add_tag
      (
       &d->vorbisComment,
       const_cast<char *>("artist"),
       const_cast<char *>(d->cd_artist.utf8().data())
      );

    vorbis_comment_add_tag
      (
       &d->vorbisComment,
       const_cast<char *>("album"),
       const_cast<char *>(d->cd_title.utf8().data())
      );

    vorbis_comment_add_tag
      (
       &d->vorbisComment,
       const_cast<char *>("tracknumber"),
       const_cast<char *>(QString::number(d->requestedTrack).utf8().data())
      );
  }
#endif


  long firstSector    = cdda_track_firstsector(drive, d->requestedTrack);
  long lastSector     = cdda_track_lastsector(drive, d->requestedTrack);
  long totalByteCount = CD_FRAMESIZE_RAW * (lastSector - firstSector);
  long time_secs      = (8 * totalByteCount) / (44100 * 2 * 16);

#ifdef HAVE_LAME

  if (FileTypeMP3 == type)
  {
    totalSize((time_secs * d->bitrate * 1000)/8);
  }

#endif

#ifdef HAVE_VORBIS

  if (FileTypeOggVorbis == type)
  {
    totalSize((time_secs * d->vorbisBitrate)/8);
  }

#endif

  if (FileTypeWAV == type)
  {
    totalSize(44 + totalByteCount); // Include RIFF header length.
    writeHeader(totalByteCount);    // Write RIFF header.
  }

  paranoiaRead(drive, firstSector, lastSector, type);

  // Send an empty QByteArray to signal end of data.

  data(QByteArray());

  cdda_close(drive);

  finished();
}

  void
AudioCDProtocol::stat(const KURL & url)
{
  struct cdrom_drive * drive = initRequest(url);

  if (0 == drive)
    return;

  bool isFile = !d->filename.isEmpty();

  if (isFile && (!d->requestedTrackOK || d->requestedTrack + 1 > d->trackCount))
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.url());
    return;
  }

  // Create an UDSEntry and add some UDSAtoms to it.

  UDSEntry entry;

  // Create an UDSAtom representing the name of the requested file.

  UDSAtom nameAtom;

  nameAtom.m_uds = KIO::UDS_NAME;
  nameAtom.m_str = url.filename();

  entry.append(nameAtom);

  // Create an UDSAtom representing the type of the requested file.

  UDSAtom typeAtom;

  typeAtom.m_uds = KIO::UDS_FILE_TYPE;
  typeAtom.m_long = isFile ? S_IFREG : S_IFDIR;

  entry.append(typeAtom);

  // Create an UDSAtom representing the permissions of the requested file.

  UDSAtom accessAtom;

  accessAtom.m_uds = KIO::UDS_ACCESS;
  accessAtom.m_long = 0400;

  entry.append(accessAtom);

  // Create an UDSAtom representing the size of the requested file.

  UDSAtom sizeAtom;

  sizeAtom.m_uds = KIO::UDS_SIZE;

  if (!isFile)
  {
    // If a file was not requested, assume a dir and give the number of
    // tracks as the size.

    sizeAtom.m_long = cdda_tracks(drive);
  }
  else
  {
    // A file was requested. The size we report depends on the type of
    // the file.

    long filesize = CD_FRAMESIZE_RAW *
      (
       cdda_track_lastsector(drive, d->requestedTrack)
       -
       cdda_track_firstsector(drive, d->requestedTrack)
      );

    long length_seconds = (filesize) / 176400;

    int type(fileType(d->filename));

#ifdef HAVE_LAME

    if (FileTypeMP3 == type)
    {
      sizeAtom.m_long = (length_seconds * d->bitrate*1000) / 8;
    }

#endif

#ifdef HAVE_VORBIS

    if (FileTypeOggVorbis == type)
    {
      sizeAtom.m_long = (length_seconds * d->vorbisBitrate) / 8;
    }

#endif

    if (FileTypeWAV == type)
    {
      sizeAtom.m_long = filesize + 44;
    }
  }

  entry.append(sizeAtom);

  statEntry(entry);

  cdda_close(drive);

  finished();
}

  unsigned int
AudioCDProtocol::discid(struct cdrom_drive * drive)
{
  // Work out the 'id' of the CDROM. The algorithm must match that
  // used by the CDDB service or confusion will reign and the toast
  // will burn.

  uint id = 0;

  for (uint i = 1; i <= uint(drive->tracks); i++)
  {
    uint n = cdda_track_firstsector (drive, i) + 150;

    if (i == uint(hack_track))
      n = start_of_first_data_as_in_toc + 150;

    n /= 75;

    while (n > 0)
    {
      id += n % 10;
      n /= 10;
    }
  }

  uint l = (my_last_sector(drive));

  l -= cdda_disc_firstsector(drive);
  l /= 75;

  id = ((id % 255) << 24) | (l << 8) | drive->tracks;

  return id;
}

  void
AudioCDProtocol::updateCD(struct cdrom_drive * drive)
{
  unsigned int id = discid(drive);

  if (id == d->discid)
    return;

  d->discid = id;
  d->trackCount = cdda_tracks(drive);
  d->cd_title = i18n("No Title");

  d->titles.clear();

  QValueList<int> qvl;

  for (uint i = 0; i < d->trackCount; i++)
  {
    d->is_audio[i] = cdda_track_audiop(drive, i + 1);
    if (i + 1 != uint(hack_track))
      qvl.append(cdda_track_firstsector(drive, i + 1) + 150);
    else
      qvl.append(start_of_first_data_as_in_toc + 150);
  }

  qvl.append(cdda_disc_firstsector(drive));
  qvl.append(my_last_sector(drive));

  if (d->useCDDB)
  {
    d->cddb->set_server(d->cddbServer.latin1(), d->cddbPort);

    if (d->cddb->queryCD(qvl))
    {
      d->based_on_cddb = true;
      d->cd_title = d->cddb->title();
      d->cd_artist = d->cddb->artist();
      for (uint i = 0; i < d->trackCount; i++)
      {
        QString n;
        n.sprintf("%02d ", i + 1);
        d->titles.append (n + d->cddb->track(i));
      }
      return;
    }
  }

  d->based_on_cddb = false;
  for (uint i = 0; i < d->trackCount; i++)
  {
    QString num;
    int ti = i + 1;
    QString s;
    num.sprintf("%02d", ti);
    if (cdda_track_audiop(drive, ti))
      s = d->s_track.arg(num);
    else
      s.sprintf("data%02d", ti);
    d->titles.append( s );
  }
}

  void
AudioCDProtocol::listDir(const KURL & url)
{
  struct cdrom_drive * drive = initRequest(url);

  if (0 == drive)
    return;

  UDSEntry entry;

  if (DirTypeUnknown == d->dirType)
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.url());
    return;
  }

  if (!d->filename.isEmpty() && d->dirType != DirTypeDevice)
  {
    error(KIO::ERR_IS_FILE, url.url());
    return;
  }

  // XXX We can't handle dirType == Device for now.

  bool listTracks = true;

  if (DirTypeRoot == d->dirType)
  {
    // List our virtual directories.

    if (d->based_on_cddb)
    {
      appendDir(entry, d->s_byname, d->trackCount);
      listEntry(entry, false);
    }

    appendDir(entry, d->s_info, 1);
    listEntry(entry, false);

    appendDir(entry, d->cd_title, d->trackCount);
    listEntry(entry, false);

    appendDir(entry, d->s_bytrack, d->trackCount);
    listEntry(entry, false);

    appendDir(entry, QString("dev"), 1);
    listEntry(entry, false);

#ifdef HAVE_LAME
    appendDir(entry, d->s_mp3, d->trackCount);
    listEntry(entry, false);
#endif

#ifdef HAVE_VORBIS
    appendDir(entry, d->s_vorbis, d->trackCount);
    listEntry(entry, false);
#endif

  }
  else if
    (
     DirTypeDevice == d->dirType
     &&
     url.path().length() <= 5 // "/dev{/}"
    )
  {
    appendDir(entry, QString("cdrom"), d->trackCount);
    listEntry(entry, false);
    listTracks = false;
  }
  else if (DirTypeInfo == d->dirType)
  {
    /* List some text files */
    /* XXX */
    listTracks = false;
  }

  if (listTracks)
  {
    for (uint i = 1; i <= d->trackCount; i++)
    {
      if (d->is_audio[i-1])
      {
        long size =
          CD_FRAMESIZE_RAW *
          (cdda_track_lastsector(drive, i) - cdda_track_firstsector(drive, i));

        long length_seconds = size / 176400;

        QString indexString;

        indexString.sprintf("%02d", i);

        QString title(QString(d->titles[i - 1]).stripWhiteSpace());

        QString name;

        switch (d->dirType)
        {
          case DirTypeDevice:
          case DirTypeRoot:
            name.sprintf("track%02d.cda", i);
            break;

          case DirTypeByTrack:

            if (title.isEmpty())
              name = d->s_track.arg(indexString) + ".wav";
            else
              name = title + ".wav";

            size += 44;
            break;

#ifdef HAVE_LAME

          case DirTypeMP3:

            if (title.isEmpty())
              name = d->s_track.arg(indexString) + ".mp3";
            else
              name = title + ".mp3";

            size = (length_seconds * d->bitrate*1000) / 8;

            break;

#endif

#ifdef HAVE_VORBIS

          case DirTypeVorbis:

            if (title.isEmpty())
              name = d->s_track.arg(indexString) + ".ogg";
            else
              name = title + ".ogg";

            size = (length_seconds * d->vorbisBitrate) / 8;

            break;

#endif

          case DirTypeByName:
          case DirTypeTitle:

            if (title.isEmpty())
              name = d->s_track.arg(indexString) + ".wav";
            else
              name = title + ".wav";

            size += 44;

            break;

          case DirTypeInfo:
          case DirTypeUnknown:
          default:
            error(KIO::ERR_INTERNAL, url.url());
            return;
        }

        appendFile(entry, name, size);

        listEntry(entry, false);
      }
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
AudioCDProtocol::findDrive(bool &noPermission)
{
  QCString path(QFile::encodeName(d->path));

  struct cdrom_drive * drive = 0;

  if (!path.isEmpty() && path != "/")
  {
    if (!QFileInfo(path).isReadable())
    {
      noPermission = true;
    }
    else
    {
      drive = cdda_identify(path, CDDA_MESSAGE_PRINTIT, 0);
    }
  }
  else
  {
    drive = cdda_find_a_cdrom(CDDA_MESSAGE_PRINTIT, 0);

    if (0 == drive)
    {
      if (QFile(defaultDevice).exists())
        drive = cdda_identify(defaultDevice, CDDA_MESSAGE_PRINTIT, 0);
    }
  }

  if (0 == drive)
  {
    kdDebug(7101) << "Can't find an audio CD" << endl;
  }

  return drive;
}

  void
AudioCDProtocol::parseArgs(const KURL & url)
{
  QString old_cddb_server = d->cddbServer;
  uint    old_cddb_port   = d->cddbPort;
  bool    old_use_cddb    = d->useCDDB;

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

    if ("device" == attribute)
    {
      d->path = value;
    }
    else if ("paranoia_level" == attribute)
    {
      d->paranoiaLevel = value.toInt();
    }
    else if ("use_cddb" == attribute)
    {
      d->useCDDB = (0 != value.toInt());
    }
    else if ("cddb_server" == attribute)
    {
      int portPos = value.find(':');

      if (-1 == portPos)
        d->cddbServer = value;

      else
      {
        d->cddbServer = value.left(portPos);
        d->cddbPort   = value.mid(portPos + 1).toUInt();
      }
    }
  }

  // We need to recheck the CD if the user either enabled CDDB now,
  // or changed the server (port).  We simply reset the saved discid,
  // which forces a reread of CDDB information.

  if
    (
     (old_use_cddb != d->useCDDB && d->useCDDB == true)
     ||
     old_cddb_server != d->cddbServer
     ||
     old_cddb_port != d->cddbPort
    )
  {
    d->discid = 0;
  }

  kdDebug(7101) << "CDDB: use_cddb = " << d->useCDDB << endl;

}

  void
AudioCDProtocol::paranoiaRead
(
  struct cdrom_drive  * drive,
  long                  firstSector,
  long                  lastSector,
  int                   fileType
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
  const uint mp3BufferSize = 8000;
  static char mp3buffer[mp3BufferSize];
#endif

  long processed(0);
  long currentSector(firstSector);

#ifdef HAVE_VORBIS
  if (FileTypeOggVorbis == fileType)
  {
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    vorbis_analysis_init  (&d->vorbisDSPState,  &d->vorbisInfo);
    vorbis_block_init     (&d->vorbisDSPState,  &d->vorbisBlock);

    srand(time(NULL));

    ogg_stream_init(&d->oggStreamState,rand());

    vorbis_analysis_headerout
      (
       &d->vorbisDSPState,
       &d->vorbisComment,
       &header,
       &header_comm,
       &header_code
      );

    ogg_stream_packetin(&d->oggStreamState, &header);
    ogg_stream_packetin(&d->oggStreamState, &header_comm);
    ogg_stream_packetin(&d->oggStreamState, &header_code);

    while (int result = ogg_stream_flush(&d->oggStreamState, &d->oggPage))
    {
      if (0 == result)
        break;

      QByteArray output;

      char * oggheader  = reinterpret_cast<char *>(d->oggPage.header);
      char * oggbody    = reinterpret_cast<char *>(d->oggPage.body);

      output.setRawData(oggheader, d->oggPage.header_len);
      data(output);
      output.resetRawData(oggheader, d->oggPage.header_len);

      output.setRawData(oggbody, d->oggPage.body_len);
      data(output);
      output.resetRawData(oggbody, d->oggPage.body_len);
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

      if (FileTypeMP3 == fileType)
      {
        int mp3bytes =
          lame_encode_buffer_interleaved
          (
           d->gf,
           buf,
           CD_FRAMESAMPLES,
           reinterpret_cast<unsigned char *>(mp3buffer),
           int(mp3BufferSize)
          );

        if (mp3bytes < 0)
        {
          kdDebug(7101) << "lame encoding failed" << endl;
          break;
        }

        if (mp3bytes > 0)
        {
          QByteArray output;

          output.setRawData(mp3buffer, mp3bytes);
          data(output);
          output.resetRawData(mp3buffer, mp3bytes);
          processed += mp3bytes;
        }
      }

#endif

#ifdef HAVE_VORBIS

      if (FileTypeOggVorbis == fileType)
      {
        int i;
        float **buffer=vorbis_analysis_buffer(&d->vorbisDSPState,CD_FRAMESAMPLES);

        /* uninterleave samples */
        for(i=0;i<CD_FRAMESAMPLES;i++){
          buffer[0][i]=buf[2*i]/32768.0;
          buffer[1][i]=buf[2*i+1]/32768.0;
        }

        vorbis_analysis_wrote(&d->vorbisDSPState,i);

        while
          (
           1 == vorbis_analysis_blockout(&d->vorbisDSPState, &d->vorbisBlock)
          )
        {
          vorbis_analysis     (&d->vorbisBlock, &d->oggPacket);
          ogg_stream_packetin (&d->oggStreamState, &d->oggPacket);

          while
            (
             int result = ogg_stream_pageout(&d->oggStreamState, &d->oggPage)
            )
          {
            if (!result) break;

            QByteArray output;

            char * oggheader  = reinterpret_cast<char *>(d->oggPage.header);
            char * oggbody    = reinterpret_cast<char *>(d->oggPage.body);

            output.setRawData(oggheader, d->oggPage.header_len);
            data(output);
            output.resetRawData(oggheader, d->oggPage.header_len);

            output.setRawData(oggbody, d->oggPage.body_len);
            data(output);
            output.resetRawData(oggbody, d->oggPage.body_len);
            processed +=  d->oggPage.header_len + d->oggPage.body_len;
          }
        }
      }

#endif

      if (FileTypeWAV == fileType)
      {
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

  if (FileTypeMP3 == fileType)
  {
    int mp3bytes =
      lame_encode_finish
      (
       d->gf,
       reinterpret_cast<unsigned char *>(mp3buffer),
       int(mp3BufferSize)
      );

    if (mp3bytes < 0)
    {
      kdDebug(7101) << "lame encoding failed" << endl;
    }

    if (mp3bytes > 0)
    {
      QByteArray output;
      output.setRawData(mp3buffer, mp3bytes);
      data(output);
      output.resetRawData(mp3buffer, mp3bytes);
    }
  }

#endif

#ifdef HAVE_VORBIS

  if (FileTypeOggVorbis == fileType)
  {
    ogg_stream_clear    (&d->oggStreamState);
    vorbis_block_clear  (&d->vorbisBlock);
    vorbis_dsp_clear    (&d->vorbisDSPState);
    vorbis_info_clear   (&d->vorbisInfo);
  }

#endif

  paranoia_free(paranoia);
  paranoia = 0;
}


void AudioCDProtocol::getParameters()
{
  KConfig * config = new KConfig("kcmaudiocdrc");

  config->setGroup("CDDA");

  if (!config->readBoolEntry("autosearch", true))
  {
    d->path = config->readEntry("device", defaultDevice);
  }

  // Enable paranoia error correction, but allow skipping

  d->paranoiaLevel = 1;

  if (config->readBoolEntry("disable_paranoia",false))
  {
    // Disable all paranoia error correction.

    d->paranoiaLevel = 0;
  }

  if (config->readBoolEntry("never_skip",true))
  {
    // Try to get a perfect rip.

    d->paranoiaLevel = 2;
  }

  config->setGroup("CDDB");

  d->useCDDB = config->readBoolEntry("enable_cddb",true);

  QString cddbserver = config->readEntry("cddb_server", defaultCDDBServer);

  int portPos = cddbserver.find(':');

  if (-1 == portPos)
  {
    d->cddbServer = cddbserver;
  }
  else
  {
    d->cddbServer = cddbserver.left(portPos);
    d->cddbPort   = cddbserver.mid(portPos + 1).toUInt();
  }

#ifdef HAVE_LAME

  config->setGroup("MP3");

  int quality = config->readNumEntry("quality", 2);

  if (quality < 0) quality = 0;
  if (quality > 9) quality = 9;

  int method = config->readNumEntry("encmethod", 0);

  if (0 == method)
  {
    // Constant Bitrate Encoding
    lame_set_VBR(d->gf, vbr_off);
    lame_set_brate(d->gf,config->readNumEntry("cbrbitrate",160));
    d->bitrate = lame_get_brate(d->gf);
    lame_set_quality(d->gf, quality);

    d->gf->VBR      = vbr_off;
    d->gf->brate    = config->readNumEntry("cbrbitrate", 160);
    d->bitrate      = d->gf->brate;
    d->gf->quality  = quality;
  }
  else
  {
    // Variable Bitrate Encoding
      lame_set_VBR(d->gf,vbr_abr);
      lame_set_VBR_mean_bitrate_kbps(d->gf, config->readNumEntry("vbr_average_bitrate",0));

      d->bitrate = lame_get_VBR_mean_bitrate_kbps(d->gf);

      d->bitrate = d->gf->VBR_mean_bitrate_kbps;
    }
    else
    {
      if (d->gf->VBR == vbr_off)
      {
        d->gf->VBR = vbr_default;
      }

      if (lame_get_VBR(d->gf) == vbr_off) lame_set_VBR(d->gf, vbr_default);

      if (config->readBoolEntry("set_vbr_min",true)) 
	lame_set_VBR_min_bitrate_kbps(d->gf, config->readNumEntry("vbr_min_bitrate",0));
      if (config->readBoolEntry("vbr_min_hard",true))
	lame_set_VBR_hard_min(d->gf, 1);
      if (config->readBoolEntry("set_vbr_max",true)) 
	lame_set_VBR_max_bitrate_kbps(d->gf, config->readNumEntry("vbr_max_bitrate",0));

      d->bitrate = 128;
      lame_set_VBR_q(d->gf, quality);
      
    }

    if ( config->readBoolEntry("write_xing_tag",true) ) lame_set_bWriteVbrTag(d->gf, 1);

  }

  switch (config->readNumEntry("mode", 0))
  {
    case 0:
      lame_set_mode(d->gf, STEREO);
      break;

    case 1:
      lame_set_mode(d->gf, JOINT_STEREO);
      break;

  lame_set_copyright(d->gf, config->readBoolEntry("copyright",false));
  lame_set_original(d->gf, config->readBoolEntry("original",true));
  lame_set_strict_ISO(d->gf, config->readBoolEntry("iso",false));
  lame_set_error_protection(d->gf, config->readBoolEntry("crc",false));

    case 3:
      lame_set_mode(d->gf, MONO);
      break;

  if ( config->readBoolEntry("enable_lowpassfilter",false) ) {

    lame_set_lowpassfreq(d->gf, config->readNumEntry("lowpassfilter_freq",0));

    if (config->readBoolEntry("set_lowpassfilter_width",false)) {
      lame_set_lowpasswidth(d->gf, config->readNumEntry("lowpassfilter_width",0));
    }
  }

  if ( config->readBoolEntry("enable_highpassfilter",false) ) {

    lame_set_highpassfreq(d->gf, config->readNumEntry("highpassfilter_freq",0));

    if (config->readBoolEntry("set_highpassfilter_width",false)) {
      lame_set_highpasswidth(d->gf, config->readNumEntry("highpassfilter_width",0));
    }
  }

#endif // HAVE_LAME

#ifdef HAVE_VORBIS

  config->setGroup("Vorbis");

  if (config->readBoolEntry("set_vorbis_min_bitrate", false))
  {
    d->vorbisBitrateLower =
      config->readNumEntry("vorbis_min_bitrate",40) * 1000;
  }
  else
  {
    d->vorbisBitrateLower = -1;
  }

  if (config->readBoolEntry("set_vorbis_max_bitrate",false))
  {
    d->vorbisBitrateUpper =
      config->readNumEntry("vorbis_max_bitrate",350) * 1000;

  }
  else
  {
    d->vorbisBitrateUpper = -1;
  }

  if (d->vorbisBitrateUpper != -1 && d->vorbisBitrateLower != -1)
  {
    d->vorbisBitrate = 104000; // empirically determined ...?!
  }
  else
  {
    d->vorbisBitrate = 160 * 1000;
  }

  if (config->readBoolEntry("set_vorbis_nominal_bitrate", true))
  {
    d->vorbisBitrateNominal =
      config->readNumEntry("vorbis_nominal_bitrate",160) * 1000;

    d->vorbisBitrate = d->vorbisBitrateNominal;

  }
  else
  {
    d->vorbisBitrateNominal = -1;
  }

  d->writeVorbisComments = config->readBoolEntry("vorbis_comments",true);

#endif // HAVE_VORBIS

  delete config;
  config = 0;

  return;
}

void paranoiaCallback(long, int)
{
  // STUB

  // Do we want to show info somewhere ?
  // Not yet.
}

// vim:ts=2:sw=2:tw=78:et:
