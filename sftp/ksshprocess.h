/***************************************************************************
                          ksshprocess.h  -  description
                             -------------------
    begin                : Tue Jul 31 2001
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

#ifndef KSSHPROCESS_H
#define KSSHPROCESS_H

#define KSSHPROC 7116

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include <qvaluelist.h>

#include <kdebug.h>

#include "kssh.h"
#include "process.h"


/********************
 * Provides easy and version independent access to ssh.
 * @author Lucas Fisher
 */

class KSSHProcess {

public: 
	KSSHProcess();
    KSSHProcess(QString pathToSsh);
	~KSSHProcess();
    QString version();
    int error(QString& msg);
    int error() { return mError; }
    void kill(int signal = SIGTERM);
    void printArgs();
    bool setArgs(const KSSH::SshOptList& opts);
    bool connect(bool acceptHostKey = false);

private:
    QString mSshPath;
    int mVersion;
    QString mPassword;
    QString mUsername;
    QString mHost;
    bool mConnected;
    int mPort;
    int mError;
    QString mErrorMsg;
    MyPtyProcess ssh;
    QCStringList mArgs;
    void init();

    static const char * const versionStrs[];
    static const char * const passwdPrompt[];
    static const char * const authFailedPrompt[];
    static const char * const hostKeyMissing[];
    static const char * const hostKeyChanged[];
    static const char * const continuePrompt[];
};
#endif
