/***************************************************************************
                          kprocessblockingrw.cpp  -  description
                             -------------------
    begin                : Sun Jul 8 2001
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

#include "kprocessblockingrw.h"
#include "atomicio.h"
#include <unistd.h>

KProcessBlockingRW::KProcessBlockingRW() : KProcess() {
}
KProcessBlockingRW::~KProcessBlockingRW() {
}
/** A blocking write call to stdin the the process.  If waitAll is true, buflen bytes of buf 
be written unless an error other than EINTR or EAGAIN is detected. */
int KProcessBlockingRW::writeStdin(char* buf, int buflen, bool blocking, bool waitAll){
    int ret;
    if( !blocking )
        ret = KProcess::writeStdin(buf, buflen);
    else {
        if( runs && (communication & Stdin) ) {
            if( waitAll )
                ret = atomicio(in[1], buf, buflen, false);
            else
                ret = ::write(in[1], buf, buflen);
            innot->setEnabled(true);
        }
        else
            ret = 0;
    }
    return ret;
}

/** Blocking call to read the process's stdout.  If waitAll is true, will read buflen bytes
unless an error other than EINTR or EAGAIN is detected. */
int KProcessBlockingRW::readStdout(char* buf, int buflen, bool waitAll){
    if( waitAll )
        return atomicio(out[0], buf, buflen);
    else
        return ::read(out[0], buf, buflen);
}

