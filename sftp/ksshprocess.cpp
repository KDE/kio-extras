/***************************************************************************
                          ksshprocess.cpp  -  description
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

#include "ksshprocess.h"

#include <stdio.h>
#include <errno.h>

#include <kstddirs.h>

const char * const KSSHProcess::versionStrs[] = {
    "OpenSSH_2.9p1",
    "OpenSSH_2.9p2",
    "OpenSSH_2.9",
    "OpenSSH",
    "SSH Secure Shell 3.0.0",
    "SSH Secure Shell"
};

const char * const KSSHProcess::passwdPrompt[] = {
    "password:", // OpenSSH_2.9p1
    "password:", // OpenSSH_2.9p2
    "password:", // OpenSSH_2.9
    "password:", // OpenSSH
    "password:", // SSH 3.0.0
    "password:"  // SSH
};

const char* const KSSHProcess::authFailedPrompt[] = {
    "Permission denied",
    "",
    "",
    "",
    "Authentication failed.",
    "Authentication failed."
};

const char* const KSSHProcess::hostKeyMissing[] = {
    "The authenticity of host",
    "The authenticity of host",
    "The authenticity of host",
    "The authenticity of host",
    "Host key not found from database",
    "Host key not found from database"
};

const char* const KSSHProcess::continuePrompt[] = {
    "Are you sure you want to continue connecting (yes/no)?",
    "Are you sure you want to continue connecting (yes/no)?",
    "Are you sure you want to continue connecting (yes/no)?",
    "Are you sure you want to continue connecting (yes/no)?",
    "Are you sure you want to continue connecting (yes/no)?",
    "Are you sure you want to continue connecting (yes/no)?"
};

const char* const KSSHProcess::hostKeyChanged[] = {
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
};

KSSHProcess::KSSHProcess() {
    mSshPath = KStandardDirs::findExe(QString::fromLatin1("ssh"));
    if( mSshPath.isEmpty() ) {
        kdDebug(KSSHPROC) << "KSSHProcess::KSSHProcess(): ssh path not found" << endl;
    }
    init();
}

KSSHProcess::KSSHProcess(QString pathToSsh) {
    mSshPath = pathToSsh;
    init();
}

KSSHProcess::~KSSHProcess(){
}

void KSSHProcess::init() {
    mVersion = -1;
    mConnected = false;
}

QString KSSHProcess::version() {
    QString cmd;
    cmd = mSshPath+" -V 2>&1";

    FILE *p;
    if( (p = popen(cmd.latin1(), "r")) == NULL ) {
        kdDebug(KSSHPROC) << "KSSHProcess::version(): failed to start ssh: " << strerror(errno) << endl;
        return QString::null;
    }

    size_t len;
    char buf[128];
    if( (len = fread(buf, sizeof(char), sizeof(buf)-1, p)) == 0 ) {
        kdDebug(KSSHPROC) << "KSSHProcess::version(): Read of ssh version string failed " << 
                             strerror(ferror(p)) << endl;
        return QString::null;
    }
    pclose(p);
    buf[len] = '\0';
    QString ver;
    ver = buf;
    kdDebug(KSSHPROC) << "KSSHProcess::version(): got version string [" << ver << "]" << endl;

    mVersion = -1;
    for(int i = 0; i < SSH_VER_MAX; i++) {
        if( ver.contains(versionStrs[i]) ) {
             mVersion = i;
             break;
        }
    }

    if( mVersion == -1 ) {
        kdDebug(KSSHPROC) << "KSSHProcess::version(): Sorry, I don't know about this version of ssh" << endl;
        return QString::null;
    }

    ver = versionStrs[mVersion];
    return ver;
}


bool KSSHProcess::setArgs(const SshOptList& opts) {
    kdDebug(KSSHPROC) << "KSSHProcess::setArgs()" << endl;
    mArgs.clear();
    SshOptListConstIterator it;
    QString cmd, subsystem;
    mPassword = mUsername = mHost = QString::null;
    QCString tmp;
    for(it = opts.begin(); it != opts.end(); ++it) {
        switch( (*it).opt ) {
        case SSH_VERBOSE:
            mArgs.append("-v");
            break;

        case SSH_SUBSYSTEM:
            subsystem = (*it).str;
            break;

        case SSH_PORT:
            mArgs.append("-p");
            tmp.setNum((*it).num);
            mArgs.append(tmp);
            mPort = (*it).num;
            break;

        case SSH_HOST:
            mHost = (*it).str;
            break;

        case SSH_USERNAME:
            mArgs.append("-l");
            mArgs.append((*it).str.latin1());
            mUsername = (*it).str;
            break;

        case SSH_PASSWD:
            mPassword = (*it).str;
            break;

        case SSH_PROTOCOL:
            if( mVersion <= OPENSSH ) {
                tmp = "Protocol";
                tmp += QString::number((*it).num).latin1();
                mArgs.append("-o");
                mArgs.append(tmp);
            }
            else if( mVersion <= SSH ) {
                if( (*it).num == 1 ) {
                    mArgs.append("-1i");
                }
                // else uses version 2 by default
            }
            break;

        case SSH_FORWARDX11:
            tmp = "ForwardX11 ";
            tmp += (*it).boolean ? "yes" : "no";
            mArgs.append("-o");
            mArgs.append(tmp);
            break;

        case SSH_FORWARDAGENT:
            tmp = "ForwardAgent ";
            tmp += (*it).boolean ? "yes" : "no";
            mArgs.append("-o");
            mArgs.append(tmp);
            break;

        case SSH_ESCAPE_CHAR:
            if( (*it).num == 0 )
                tmp = "none";
            else
                tmp = (char)((*it).num);
            mArgs.append("-e");
            mArgs.append(tmp);
            break;

        case SSH_OPTION:
            // don't allow NumberOfPasswordPrompts or StrictHostKeyChecking
            tmp = (*it).str;
            if( tmp.contains("NumberOfPasswordPrompts") ||
                tmp.contains("StrictHostKeyChecking") ) {
                mError = ERR_INVALID_OPT;
                return false;
            }
            else {
                mArgs.append("-o");
                mArgs.append(tmp);
            }
            break;
            
        case SSH_COMMAND:
            cmd = (*it).str;
            break;

        default:
            kdDebug(KSSHPROC) << "KSSHProcess::getArgs(): unrecognized ssh opt " << (*it).opt << endl;
        }
    }

    if( !subsystem.isEmpty() && !cmd.isEmpty() ) {
        kdDebug(KSSHPROC) << "KSSHProcess::getArgs(): cannot use a subsystem and command at the same time" << endl;
        mError = ERR_CMD_SUBSYS_CONFLICT;
        return false;
    }

    if( mPassword.isEmpty() ) {
        kdDebug(KSSHPROC) << "KSSHProcess::getArgs(): a password must be supplied" << endl;
        mError = ERR_NEED_PASSWD;
        return false;
    }

    if( mUsername.isEmpty() ) {
        kdDebug(KSSHPROC) << "KSSHProcess::getArgs(): a username must be supplied" << endl;
        mError = ERR_NEED_USERNAME;
        return false;
    }

    // These options govern the behavior of ssh and cannot be defined by the user
    mArgs.append("-o");
    mArgs.append("NumberOfPasswordPrompts 1");
    mArgs.append("-o");
    mArgs.append("StrictHostKeyChecking ask");
    if( mVersion <= OPENSSH ) {
        // nothing
    }
    else if( mVersion <= SSH ) {
        mArgs.append("-o");
        mArgs.append("AuthenticationSuccessMsg no");
    }

    if( mHost.isEmpty() ) {
        kdDebug(KSSHPROC) << "KSSHProcess::getArgs(): a host name must be supplied" << endl;
        return false;
    }
    else {
        mArgs.append(mHost.latin1());
    }

    if( !subsystem.isEmpty() ) {
        mArgs.append("-s");
        mArgs.append(subsystem.latin1());
    }

    if( !cmd.isEmpty() ) {
        mArgs.append(cmd.latin1());
    }

    return true;
}

void KSSHProcess::printArgs() {
    QValueListIterator<QCString> it;
    for( it = mArgs.begin(); it != mArgs.end(); ++it) {
        kdDebug(KSSHPROC) << "arg: " << *it << endl;
    }
}


int KSSHProcess::error(QString& msg) {
    msg = mErrorMsg;
    return mError;
}

void KSSHProcess::kill(int signal) {
    // expects the signal to kill the process. we should change this later
    ::kill(ssh.pid(), signal);
    ::waitpid(ssh.pid(), NULL, 0);
    mConnected = false;
}

bool KSSHProcess::connect(bool acceptHostKey) {
    if( mVersion == -1 ) {
        // we don't know the ssh version yet, so find out
        version();
    }

    if( mArgs.isEmpty() ) {
        kdDebug(KSSHPROC) << "KSSHProcess::connect(): ssh options need to be set first using setArgs()" << endl;
        mError = ERR_NO_OPTIONS;
        return false;
    }

    if( ssh.exec(mSshPath.latin1(), mArgs) ) {
        kdDebug(KSSHPROC) << "KSSHProcess::connect(): ssh exec failed" << endl;
        mError = ERR_CANNOT_LAUNCH;
        return false;
    }

    int ptyfd = ssh.fd();
    int errfd = ssh.stderr();
    int stdiofd = ssh.stdio();
    fd_set rfds;
    struct timeval tv;
    
    int maxfd = ptyfd > errfd ? ptyfd : errfd;  // find max file descriptor
    
    int tries = 100;
    bool lookForContinuePrompt = false;
    QString message; // save any messages from SSH here so we can give them to the user
    do {
        FD_ZERO(&rfds);
        FD_SET(ptyfd, &rfds);           // Add pty file descriptor
        FD_SET(errfd, &rfds);           // Add std error file descriptor
        tv.tv_sec = 60; tv.tv_usec = 0; // 60 second timeout
        
        int ret = select(maxfd+1, &rfds, NULL, NULL, &tv);
        kdDebug(KSSHPROC) << "KSSHProcess::connect(): Select returned " << ret << endl;
        
        if( ret > 0 ) {
            if( FD_ISSET(ptyfd, &rfds) ) {
                QCString line = ssh.readLine();
                kdDebug(KSSHPROC) << "KSSHProcess::connect(): got line from pty [" << line << "]" << endl;
                if( line.contains(passwdPrompt[mVersion]) ) {
                    kdDebug(KSSHPROC) << "KSSHProcess::connect(): found password prompt" << endl;
                    ssh.WaitSlave();
                    ssh.writeLine(mPassword.latin1());
                    ssh.readLine();
                    break;
                }
                else {
                  //  kdDebug(KSSHPROC) << "KSSHProcess::connect(): unrecognized message [" << line << "]" << endl;
                }
            }
            if( FD_ISSET(errfd, &rfds) ) {
                QCString line = ssh.readLineFromStderr(false);
                kdDebug(KSSHPROC) << "KSSHProcess::connect(): got from stderr [" << line << "]" << endl;
                    
                if( line.contains(hostKeyMissing[mVersion]) ) {
                    kdDebug(KSSHPROC) << "KSSHProcess::connect(): found 'no host key' message" << endl;
                    if( !acceptHostKey ) {
                        kdDebug(KSSHPROC) << "KSSHProcess::connect(): Not accepting host key" << endl;
                        mError = ERR_NEW_HOST_KEY;
                        mErrorMsg = message;
//                        kill();
//                        return false;
                    }
                    lookForContinuePrompt = true;
                }
                else if( line.contains(hostKeyChanged[mVersion]) ) {
                    kdDebug(KSSHPROC) << "KSSHProcess::connect(): found 'changed host key' message" << endl;
                    if( !acceptHostKey ) {
                        kdDebug(KSSHPROC) << "KSSHProcess::connect(): Not accepting host key" << endl;
                        mError = ERR_DIFF_HOST_KEY;
                        mErrorMsg = message;
//                        kill();
//                        return false;
                    }
                    lookForContinuePrompt = true;
                }
                else if( lookForContinuePrompt && line.contains(continuePrompt[mVersion]) ) {
                    if( acceptHostKey ) {
                        kdDebug(KSSHPROC) << "KSSHProcess::connect(): Accepting host key" << endl;
                        ::write(stdiofd, "yes\n", 4);
                    }
                    else {
                        kdDebug(KSSHPROC) << "KSSHProcess::connect(): error we should never " <<
                                             "get to continue prompt and not want to continue" << endl;
                        ::write(stdiofd, "no\n", 3);
                        kill();
                        mError = ERR_INTERNAL;
                        return false;
                    }
                }
//                else if( line.contains("Pseudo-terminal") ) {
//                    int a = ::read(ptyfd, line.data(), line.size()-1);
//                    line[a] = '\0';
//                    kdDebug(KSSHPROC) << " got line from terminal: " << line << endl;
//                }
                else {
                   kdDebug(KSSHPROC) << "KSSHProcess::connect(): unrecognized message" << endl;
                }
                
                if( lookForContinuePrompt ) { message.append(line); }
            }
        }
        else if( ret == 0 ) {
            kdDebug(KSSHPROC) << "KSSHProcess::connect(): timed out waiting for a response" << endl;
            kill();
            mError = ERR_TIMED_OUT;
            return false;
        }
        else {
            kdDebug(KSSHPROC) << "KSSHProcess::connect(): select error: " << strerror(errno) << endl;
            mError = ERR_INTERNAL;
            return false;
        }
    } while(tries--);

    if( !tries ) {
        kdDebug(KSSHPROC) << "KSSHProcess::connect(): tries exceeded. connect failed" << endl;
        mError = ERR_DISCONNECTED;
        return false;
    }
    
    // Check if ssh is still running. It will die if passwd auth fails since
    // we specified only one password prompt.

    // First make sure ssh isn't hanging around as a zombie if it has exited.
    ::waitpid(ssh.pid(), NULL, WNOHANG);

    // Send signal 0. If it succeeds or fails with error EPERM the process
    // is still running. See the Unix Programming FAQ Q1.9 for an explanation
    int ret = ::kill(ssh.pid(), 0);
    if( ret == 0 || (ret == -1 && errno == EPERM) ) {
        // ssh process probably still exists so we
        // must have successfully connected.
        kdDebug(KSSHPROC) << "KSSHProcess::connect(): ssh is still running -> auth successful!" << endl;
        return true;
    }
    kdDebug(KSSHPROC) << "KSSHProcess::connect(): ssh process no longer running -> auth failed" << endl;
    return false;
}

