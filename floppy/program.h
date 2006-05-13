/*  This file is part of the KDE project
    Copyright (C) 2000-2002 Alexander Neundorf <neundorf@kde.org>

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

#ifndef PROGRAM_H
#define PROGRAM_H

#include <QStringList>

/**
 * start programs and write to thieir stdin, stderr,
 * and read from stdout
 **/
class Program
{
public:
	Program(const QStringList &args);
	~Program();
	bool start();
	bool isRunning();

   int stdinFD() {return mStdin[1];};
   int stdoutFD() {return mStdout[0];};
   int stderrFD() {return mStderr[0];};
   int pid()      {return m_pid;};
   int kill();
   int select(int secs, int usecs, bool& stdoutReceived, bool& stderrReceived/*, bool& stdinWaiting*/);
protected:
	int mStdout[2];
	int mStdin[2];
	int mStderr[2];
   int m_pid;
	QStringList mArgs;
	bool mStarted;
};

#endif

