/*  This file is part of the KDE libraries
    Copyright (C) 2001 Malte Starostik <malte@kde.org>

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

// $Id$


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
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <qfile.h>
#include <qimage.h>


#include "gscreator.h"



extern "C"
{
    ThumbCreator *new_creator()
    {
        return new GSCreator;
    }
};

// This PS snippet will be prepended to the actual file so that only
// the first page is output.
static const char *prolog =
    "%!PS-Adobe-3.0\n"
    "/.showpage.orig /showpage load def\n"
    "/.showpage.firstonly {\n"
    "    .showpage.orig\n"
    "    quit\n"
    "} def\n"
    "/showpage { .showpage.firstonly } def\n";

static const char *gsargs[] = {
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

bool correctDVI(QString filename);

bool GSCreator::create(const QString &path, int, int, QImage &img)
{
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
  
  pid_t pid = fork(); 
  if (pid == 0) {
    // Child process (1)

    //    close(STDERR_FILENO);

    // find first zero entry in gsargs and put the filename 
    // or - (stdin) there, if DVI 
    const char **arg = gsargs;
    while (*arg)
      ++arg;
    if( no_dvi )
      *arg = path.latin1();
    else if( !no_dvi ) 
      *arg = "-";

    // find first zero entry in dvipsargs and put the filename there    
    arg = dvipsargs;
    while (*arg)
      ++arg;
    *arg = path.latin1();
    
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
    int count = write(input[1], prolog, strlen(prolog));
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
	  if (select(output[0] + 1, &fds, 0, 0, &tv) <= 0) 
	    break; // error or timeout
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
      kill(pid, SIGTERM);
    
    int status;
    
    if (waitpid(pid, &status, 0) != pid || (status != 0  && status != 256) )
      ok = false;
  } 
  else {
    // fork() (1) failed, close these
    close(input[0]);
    close(input[1]);
    close(output[0]);
  }
  close(output[1]);
  
  int l = img.loadFromData( data );
  
  return ok && l;
}

ThumbCreator::Flags GSCreator::flags() const
{
    return static_cast<Flags>(DrawFrame);
}


// Quick function to check if the filename corresponds to a valid DVI
// file. Returns true if <filename> is a DVI file, false otherwise.

bool correctDVI(QString filename)
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
