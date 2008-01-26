/*  This file is part of the KDE libraries
    Copyright (C) 2001 Malte Starostik <malte@kde.org>
    Copyright (C) 2001 Leon Bottou <leon@bottou.org>

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/


#include <config-runtime.h>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#include <QFile>
#include <QImage>

#include <kdemacros.h>

#include "djvucreator.h"



extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new DjVuCreator;
    }
}

bool DjVuCreator::create(const QString &path, int width, int height, QImage &img)
{
  int output[2];
  QByteArray data(1024, 'k');
  bool ok = false;

  if (pipe(output) == -1)
    return false;
  
  const char* argv[8];
  QByteArray sizearg, fnamearg;
  sizearg = QByteArray::number(width) + 'x' + QByteArray::number(height);
  fnamearg = QFile::encodeName( path );
  argv[0] = "ddjvu";
  argv[1] = "-page";
  argv[2] = "1";
  argv[3] = "-size";
  argv[4] = sizearg.data(); 
  argv[5] = fnamearg.data(); 
  argv[6] = 0;
  
  pid_t pid = fork(); 
  if (pid == 0)
    {
      close(output[0]);
      dup2(output[1], STDOUT_FILENO);	  
      execvp(argv[0], const_cast<char *const *>(argv));
      exit(1);
    }
  else if (pid >= 0)
    {
      close(output[1]);
      int offset = 0;
      while (!ok) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(output[0], &fds);
        struct timeval tv;
        tv.tv_sec = 20;
        tv.tv_usec = 0;
        if (select(output[0] + 1, &fds, 0, 0, &tv) <= 0) {
          if (errno == EINTR || errno == EAGAIN)
            continue;
          break; // error or timeout
        }
        if (FD_ISSET(output[0], &fds)) {
          int count = read(output[0], data.data() + offset, 1024);
          if (count == -1)
            break;
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
      if (!ok)
        kill(pid, SIGTERM);
      int status = 0;
      if (waitpid(pid, &status, 0) != pid || (status != 0  && status != 256) )
        ok = false;
    }
  else
    {
      close(output[1]);
    }
  
  close(output[0]);
  int l = img.loadFromData( data );
  return ok && l;
}


ThumbCreator::Flags DjVuCreator::flags() const
{
  return static_cast<Flags>(DrawFrame);
}


