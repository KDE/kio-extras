/*  This file is part of the KDE libraries
    Copyright (C) 2001 Malte Starostik <malte.starostik@t-online.de>

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

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <qfile.h>
#include <qpixmap.h>
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

static const char *gsargs[6] = {
    "gs",
    "-q",
    "-sDEVICE=png16m",
    "-sOutputFile=-",
    "-",
    0
};

bool GSCreator::create(const QString &path, int extent, QPixmap &pix)
{
    QFile psFile(path);
    if (!psFile.open(IO_ReadOnly))
        return false;
    
    int input[2];
    int output[2];
    QByteArray data(1024);
    bool ok = false;
    
    if (pipe(input) == -1)
        return false;
    if (pipe(output) != -1)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process, reopen stdin/stdout on the pipes and exec gs
            close(input[1]);
            close(output[0]);
            dup2(input[0], STDIN_FILENO);
            dup2(output[1], STDOUT_FILENO);
            close(STDERR_FILENO);
            execvp("gs", const_cast<char *const *>(gsargs));
            exit(1);
        }
        else if (pid != -1)
        {
            // Parent process, write prolog and actual ps file to gs
            // and read the png output
            close(input[0]);
            close(output[1]);

            int count = write(input[1], prolog, strlen(prolog));
            if (count  == static_cast<int>(strlen(prolog)))
            {
                while ((count = psFile.readBlock(data.data(), data.size())) > 0)
                {
                    if (write(input[1], data.data(), count) != count)
                        break;
                }
                if (count == 0)
                {
                    int offset = 0;
                    while ((count = read(output[0],
                        data.data() + offset, 1024)) > 0)
                    {
                        if (count == 1024)
                            data.resize(data.size() + 1024);
                        offset += count;
                    }
                    if (count == 0)
                        ok = true;
                    data.resize(offset);
                }
            }
            int status;
            if (waitpid(pid, &status, 0) != pid || status != 0)
                ok = false;
        }
        else
        {
            // fork() failed, child didn't close these
            close(input[0]);
            close(output[1]);
        }
        close(output[0]);
    }
    else // second pipe() failed
        close(input[0]);
    close(input[1]);

    if ( ok && pix.loadFromData( data ) )
    {
        int w = pix.width(), h = pix.height();
        // scale to pixie size
        if(w > extent || h > extent)
        {
            if(w > h)
            {
                h = (int)( (double)( h * extent ) / w );
                if ( h == 0 ) h = 1;
                w = extent;
                ASSERT( h <= extent );
            }
            else
            {
                w = (int)( (double)( w * extent ) / h );
                if ( w == 0 ) w = 1;
                h = extent;
                ASSERT( w <= extent );
            }
            QImage img(pix.convertToImage().smoothScale( w, h ));
            if ( img.width() != w || img.height() != h )
            {
                // Resizing failed. Aborting.
                return false;
            }
            pix.convertFromImage( img );
        }
        return true;
    }
    return false;
}

ThumbCreator::Flags GSCreator::flags() const
{
    return static_cast<Flags>(DrawFrame);
}
