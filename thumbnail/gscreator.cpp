/*  This file is part of the KDE libraries
    Copyright (C) 2001 Malte Starostik <malte@kde.org>

    Handling of EPS previews Copyright (C) 2003 Philipp Hullmann <phull@gmx.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

/*  This function gets a path of a DVI, EPS, PS or PDF file and
    produces a PNG-Thumbnail which is stored as a QImage

    The program works as follows

    1. Test if file is a DVI file

    2. Create a child process (1), in which the 
       file is to be changed into a PNG
       
    3. Child-process (1) :

    4. If file is DVI continue with 6
    
    5. If file is no DVI continue with 9

    6. Create another child process (2), in which the DVI is
       turned into PS using dvips

    7. Parent process (2) :
       Turn the recently created PS file into a PNG file using gs
       
    8. continue with 10

    9. Turn the PS,PDF or EPS file into a PNG file using gs

    10. Parent process (1)
        store data in a QImage
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#include <qcolor.h>
#include <qfile.h>
#include <qimage.h>
#include <qregexp.h>


#include "gscreator.h"
#include "dscparse_adapter.h"
#include "dscparse.h"

extern "C"
{
    ThumbCreator *new_creator()
    {
        return new GSCreator;
    }
}

// This PS snippet will be prepended to the actual file so that only
// the first page is output.
static const char *psprolog =
    "%!PS-Adobe-3.0\n"
    "/.showpage.orig /showpage load def\n"
    "/.showpage.firstonly {\n"
    "    .showpage.orig\n"
    "    quit\n"
    "} def\n"
    "/showpage { .showpage.firstonly } def\n";

// This is the code recommended by Adobe tech note 5002 for including
// EPS files.
static const char *epsprolog =
    "%!PS-Adobe-3.0\n"
    "userdict begin /pagelevel save def /showpage { } def\n"
    "0 setgray 0 setlinecap 1 setlinewidth 0 setlinejoin 10 setmiterlimit\n"
    "[ ] 0 setdash newpath false setoverprint false setstrokeadjust\n";

static const char * gsargs_ps[] = {
    "gs",
    "-sDEVICE=png16m",
    "-sOutputFile=-",
    "-dSAFER",
    "-dPARANOIDSAFER",
    "-dNOPAUSE",
    "-dFirstPage=1",
    "-dLastPage=1",
    "-q",
    "-",
    0, // file name
    "-c",
    "showpage",
    "-c",
    "quit",
    0
};

static const char * gsargs_eps[] = {
    "gs",
    "-sDEVICE=png16m",
    "-sOutputFile=-",
    "-dSAFER",
    "-dPARANOIDSAFER",
    "-dNOPAUSE",
    0, // page size
    0, // resolution
    "-q",
    "-",
    0, // file name
    "-c",
    "pagelevel",
    "-c",
    "restore",
    "-c",
    "end",
    "-c",
    "showpage",
    "-c",
    "quit",
    0
};

static const char *dvipsargs[] = {
    "dvips",
    "-n", 
    "1",
    "-q",
    "-o",
    "-",
    0, // file name
    0
};

static bool correctDVI(const QString& filename);


namespace {
	bool got_sig_term = false;
	void handle_sigterm( int ) {
		got_sig_term = true;
	}
}


bool GSCreator::create(const QString &path, int width, int height, QImage &img)
{
// The code in the loop (when testing whether got_sig_term got set)
// should read some variation of:
// 		parentJob()->wasKilled()
//
// Unfortunatelly, that's currently impossible without breaking BIC.
// So we need to catch the signal ourselves.
// Otherwise, on certain funny PS files (for example
// http://www.tjhsst.edu/~Eedanaher/pslife/life.ps )
// gs would run forever after we were dead.
// #### Reconsider for KDE 4 ###
// (24/12/03 - luis_pedro)
//
  typedef void ( *sighandler_t )( int );
  // according to linux's "man signal" the above typedef is a gnu extension
  sighandler_t oldhandler = signal( SIGTERM, handle_sigterm );
  
  int input[2];
  int output[2];
  int dvipipe[2]; 

  QByteArray data(1024);

  bool ok = false;

  // Test if file is DVI
  bool no_dvi =!correctDVI(path);

  if (pipe(input) == -1) {
    return false;
  }
  if (pipe(output) == -1) {
    close(input[0]);
    close(input[1]);
    return false;
  }

  KDSC dsc;
  endComments = false;
  dsc.setCommentHandler(this);

  if (no_dvi)
  {
    FILE* fp = fopen(QFile::encodeName(path), "r");
    if (fp == 0) return false;

    char buf[4096];
    int count;
    while ((count = fread(buf, sizeof(char), 4096, fp)) != 0
           && !endComments) {
      dsc.scanData(buf, count);
    }
    fclose(fp);

    if (dsc.pjl() || dsc.ctrld()) {
      // this file is a mess.
      return false;
    }
  }

  const bool is_encapsulated = no_dvi &&
    (path.find(QRegExp("\\.epsi?$", false, false)) > 0) &&
    (dsc.bbox()->width() > 0) && (dsc.bbox()->height() > 0) && 
    (dsc.page_count() <= 1);

  char translation[64] = "";
  char pagesize[32] = "";
  char resopt[32] = "";
  std::auto_ptr<KDSCBBOX> bbox = dsc.bbox();
  if (is_encapsulated) {
    // GhostScript's rendering at the extremely low resolutions
    // required for thumbnails leaves something to be desired. To
    // get nicer images, we render to four times the required
    // resolution and let QImage scale the result.
    const int hres = (width * 72) / bbox->width();
    const int vres = (height * 72) / bbox->height();
    const int resolution = (hres > vres ? vres : hres) * 4;
    const int gswidth = ((bbox->urx() - bbox->llx()) * resolution) / 72;
    const int gsheight = ((bbox->ury() - bbox->lly()) * resolution) / 72;

    snprintf(pagesize, 31, "-g%ix%i", gswidth, gsheight);
    snprintf(resopt, 31, "-r%i", resolution);
    snprintf(translation, 63,
       " 0 %i sub 0 %i sub translate\n", bbox->llx(),
       bbox->lly());
  }

  const CDSC_PREVIEW_TYPE previewType = 
    static_cast<CDSC_PREVIEW_TYPE>(dsc.preview());

  switch (previewType) {
  case CDSC_TIFF:
  case CDSC_WMF:
  case CDSC_PICT:
    // FIXME: these should take precedence, since they can hold
    // color previews, which EPSI can't (or can it?).
     break;
  case CDSC_EPSI:
    {
      const int xscale = bbox->width() / width;
      const int yscale = bbox->height() / height;
      const int scale = xscale < yscale ? xscale : yscale;
      if (getEPSIPreview(path,
                         dsc.beginpreview(),
                         dsc.endpreview(),
                         img,
                         bbox->width() / scale,
                         bbox->height() / scale))
        return true;
      // If the preview extraction routine fails, gs is used to
      // create a thumbnail.
    }
    break;
  case CDSC_NOPREVIEW:
  default:
    // need to run ghostscript in these cases
    break;
  }
  
  pid_t pid = fork(); 
  if (pid == 0) {
    // Child process (1)

    //    close(STDERR_FILENO);

    // find first zero entry in gsargs and put the filename 
    // or - (stdin) there, if DVI 
    const char **gsargs = gsargs_ps;
    const char **arg = gsargs;

    if (no_dvi && is_encapsulated) {
      gsargs = gsargs_eps;
      arg = gsargs;

      // find first zero entry and put page size there
      while (*arg) ++arg;
      *arg = pagesize;

      // find second zero entry and put resolution there
      while (*arg) ++arg;
      *arg = resopt;
    }

    // find next zero entry and put the filename there    
    QCString fname = QFile::encodeName( path );
    while (*arg)
      ++arg;
    if( no_dvi )
      *arg = fname.data();
    else
      *arg = "-";

    // find first zero entry in dvipsargs and put the filename there    
    arg = dvipsargs;
    while (*arg)
      ++arg;
    *arg = fname.data();
    
    if( !no_dvi ){
      pipe(dvipipe);
      pid_t pid_two = fork();
      if( pid_two == 0 ){
	// Child process (2), reopen stdout on the pipe "dvipipe" and exec dvips
	
	close(input[0]);	    
	close(input[1]);
	close(output[0]);
	close(output[1]);
	close(dvipipe[0]);
	
	dup2( dvipipe[1], STDOUT_FILENO);
	
	execvp(dvipsargs[0], const_cast<char *const *>(dvipsargs));
	exit(1);
      } 
      else if(pid_two != -1){
	close(input[1]);
	close(output[0]);
	close(dvipipe[1]);

	dup2( dvipipe[0], STDIN_FILENO);
	dup2( output[1], STDOUT_FILENO);

	execvp(gsargs[0], const_cast<char *const *>(gsargs)); 	    
	exit(1);
      }
      else{
	// fork() (2) failed, close these
	close(dvipipe[0]);
	close(dvipipe[1]);
      }
	      
    } 
    else if( no_dvi ){
      // Reopen stdin/stdout on the pipes and exec gs
      close(input[1]);
      close(output[0]);

      dup2(input[0], STDIN_FILENO);
      dup2(output[1], STDOUT_FILENO);	  

      execvp(gsargs[0], const_cast<char *const *>(gsargs));
      exit(1);
    }
  } 
  else if (pid != -1) {
    // Parent process, write first-page-only-hack (the hack is not
    // used if DVI) and read the png output
    close(input[0]);
    close(output[1]);
    const char *prolog;
    if (is_encapsulated)
      prolog = epsprolog;
    else
      prolog = psprolog;
    int count = write(input[1], prolog, strlen(prolog));
    if (is_encapsulated)
      write(input[1], translation, strlen(translation));

    close(input[1]);
    if (count == static_cast<int>(strlen(prolog))) {
      int offset = 0;
	while (!ok) {
	  fd_set fds;
	  FD_ZERO(&fds);
	  FD_SET(output[0], &fds);
	  struct timeval tv;
	  tv.tv_sec = 20;
	  tv.tv_usec = 0;

	  got_sig_term = false;
	  if (select(output[0] + 1, &fds, 0, 0, &tv) <= 0) {
            if ( ( errno == EINTR || errno == EAGAIN ) && !got_sig_term ) continue;
	    break; // error, timeout or master wants us to quit (SIGTERM)
          }
	  if (FD_ISSET(output[0], &fds)) {
	    count = read(output[0], data.data() + offset, 1024);
	    if (count == -1)
	      break;
	    else
	      if (count) // prepare for next block
		{
		  offset += count;
		  data.resize(offset + 1024);
		}
	      else // got all data
		{
		  data.resize(offset);
		  ok = true;
		}
	  }
	}
    }
    if (!ok) // error or timeout, gs probably didn't exit yet
    {
      kill(pid, SIGTERM);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) != pid || (status != 0  && status != 256) )
      ok = false;
  } 
  else {
    // fork() (1) failed, close these
    close(input[0]);
    close(input[1]);
    close(output[1]);
  }
  close(output[0]);
  
  int l = img.loadFromData( data );

  if ( got_sig_term && 
	oldhandler != SIG_ERR &&
	oldhandler != SIG_DFL &&
	oldhandler != SIG_IGN ) {
	  oldhandler( SIGTERM ); // propagate the signal. Other things might rely on it
  }
  if ( oldhandler != SIG_ERR ) signal( SIGTERM, oldhandler );
  
  return ok && l;
}

ThumbCreator::Flags GSCreator::flags() const
{
    return static_cast<Flags>(DrawFrame);
}

void GSCreator::comment(Name name)
{
    switch (name) {
    case EndPreview:
    case BeginProlog:
    case Page:
      endComments = true;
      break;

    default:
      break;
    }
}

// Quick function to check if the filename corresponds to a valid DVI
// file. Returns true if <filename> is a DVI file, false otherwise.

static bool correctDVI(const QString& filename)
{
  QFile f(filename);
  if (!f.open(IO_ReadOnly))
    return FALSE;

  unsigned char test[4];
  if ( f.readBlock( (char *)test,2)<2 || test[0] != 247 || test[1] != 2  )
    return FALSE;

  int n = f.size();
  if ( n < 134 ) // Too short for a dvi file
    return FALSE;
  f.at( n-4 );

  unsigned char trailer[4] = { 0xdf,0xdf,0xdf,0xdf };

  if ( f.readBlock( (char *)test, 4 )<4 || strncmp( (char *)test, (char*) trailer, 4 ) )
    return FALSE;
  // We suppose now that the dvi file is complete and OK
  return TRUE;
}

bool GSCreator::getEPSIPreview(const QString &path, long start, long
			       end, QImage &outimg, int imgwidth, int imgheight)
{
  FILE *fp;
  fp = fopen(QFile::encodeName(path), "r");
  if (fp == 0) return false;

  const long previewsize = end - start + 1;

  char *buf = (char *) malloc(previewsize);
  fseek(fp, start, SEEK_SET);
  int count = fread(buf, sizeof(char), previewsize - 1, fp);
  fclose(fp);
  buf[previewsize - 1] = 0;
  if (count != previewsize - 1)
  {
    free(buf);
    return false;
  }

  QString previewstr = QString::fromLatin1(buf);
  free(buf);

  int offset = 0;
  while ((offset < previewsize) && !(previewstr[offset].isDigit())) offset++;
  int digits = 0;
  while ((offset + digits < previewsize) && previewstr[offset + digits].isDigit()) digits++;
  int width = previewstr.mid(offset, digits).toInt();
  offset += digits + 1;
  while ((offset < previewsize) && !(previewstr[offset].isDigit())) offset++;
  digits = 0;
  while ((offset + digits < previewsize) && previewstr[offset + digits].isDigit()) digits++;
  int height = previewstr.mid(offset, digits).toInt();
  offset += digits + 1;
  while ((offset < previewsize) && !(previewstr[offset].isDigit())) offset++;
  digits = 0;
  while ((offset + digits < previewsize) && previewstr[offset + digits].isDigit()) digits++;
  int depth = previewstr.mid(offset, digits).toInt();

  // skip over the rest of the BeginPreview comment
  while ((offset < previewsize) &&
         previewstr[offset] != '\n' &&
	 previewstr[offset] != '\r') offset++;
  while ((offset < previewsize) && previewstr[offset] != '%') offset++;

  unsigned int imagedepth;
  switch (depth) {
  case 1:
  case 2:
  case 4:
  case 8:
    imagedepth = 8;
    break;
  case 12: // valid, but not (yet) supported
  default: // illegal value
    return false;
  }

  unsigned int colors = (1U << depth);
  QImage img(width, height, imagedepth, colors);
  img.setAlphaBuffer(false);

  if (imagedepth <= 8) {
    for (unsigned int gray = 0; gray < colors; gray++) {
      unsigned int grayvalue = (255U * (colors - 1 - gray)) / 
	(colors - 1);
      img.setColor(gray, qRgb(grayvalue, grayvalue, grayvalue));
    }
  }

  const unsigned int bits_per_scan_line = width * depth;
  unsigned int bytes_per_scan_line = bits_per_scan_line / 8;
  if (bits_per_scan_line % 8) bytes_per_scan_line++;
  const unsigned int bindatabytes = height * bytes_per_scan_line;
  unsigned char bindata[bindatabytes];

  for (unsigned int i = 0; i < bindatabytes; i++) {
    if (offset >= previewsize)
      return false;

    while (!isxdigit(previewstr[offset].latin1()) && 
	   offset < previewsize) 
      offset++;

    bool ok = false;
    bindata[i] = static_cast<unsigned char>(previewstr.mid(offset, 2).toUInt(&ok, 16));
    if (!ok)
      return false;

    offset += 2;
  }

  for (int scanline = 0; scanline < height; scanline++) {
    unsigned char *scanlineptr = img.scanLine(scanline);

    for (int pixelindex = 0; pixelindex < width; pixelindex++) {
      unsigned char pixelvalue = 0;
      const unsigned int bitoffset = 
        scanline * bytes_per_scan_line * 8U + pixelindex * depth;
      for (int depthindex = 0; depthindex < depth;
           depthindex++) {
        const unsigned int byteindex = (bitoffset + depthindex) / 8U;
        const unsigned int bitindex = 
          7 - ((bitoffset + depthindex) % 8U);
        const unsigned char bitvalue = 
          (bindata[byteindex] & static_cast<unsigned char>(1U << bitindex)) >> bitindex;
        pixelvalue |= (bitvalue << depthindex);
      }
      scanlineptr[pixelindex] = pixelvalue;
    }
  }

  outimg = img.convertDepth(32).smoothScale(imgwidth, imgheight);
  
  return true;
}
