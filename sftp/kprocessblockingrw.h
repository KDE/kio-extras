/***************************************************************************
                          kprocessblockingrw.h  -  description
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

#ifndef KPROCESSBLOCKINGRW_H
#define KPROCESSBLOCKINGRW_H

#include <kprocess.h>

/**Implements blocking read and writing
calls for KProcess.
  *@author Lucas Fisher
  */

class KProcessBlockingRW : public KProcess  {
public: 
	KProcessBlockingRW();
	~KProcessBlockingRW();

  /** A blocking write call to stdin the the process.  If blocking is true and waitAll is true,
     buflen bytes of buf be written unless an error other than EINTR or EAGAIN is detected. */
  int writeStdin(char* buf, int buflen, bool blocking = false, bool waitAll = false);

  /** Blocking call to read the process's stdout.  If waitAll is true, will read buflen bytes
      unless an error other than EINTR or EAGAIN is detected. */
  int readStdout(char* buf, int buflen, bool waitAll = false);
};

#endif
