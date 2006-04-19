/***************************************************************************
                          ksshprocess.cpp  -  description
                             -------------------
    begin                : Tue Jul 31 2001
    copyright            : (C) 2001 by Lucas Fisher
    email                : ljfisher@purdue.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
 * See the KSshProcess header for examples on use.
 *
 * This class uses a hacked version of the PTYProcess
 * class.  This was needed because the kdelibs PTYProcess does not provide 
 * access to the pty file descriptor which we need, because ssh prints the
 * password prompt to the pty and reads the password from the pty.  I don't
 * feel I know enough about ptys to confidently modify the orignial
 * PTYProcess class.
 *
 * To start ssh we take the arguments the user gave us
 * in the SshOptList and build the ssh command arguments based on the version
 * of ssh we are using.  This command and its arguments are  passed to
 * PTYProcess for execution.  Once ssh is started we scan each line of input
 * from stdin, stderr, and the pty for recognizable strings.  The recognizable
 * strings are taken from several string tables.  Each table contains a string
 * for each specific version of ssh we support and a string for a generic
 * version of OpenSSH and commercial SSH incase we don't recognized the 
 * specific ssh version strings (as when a new SSH version is released after
 * a release of KSshProcess).  There are tables for ssh version strings,
 * password prompts, new host key errors, different host key errors,
 * messages than indicate a successful connect, authentication errors, etc.
 * If we find user interaction is necessary, for instance to provide a 
 * password or passphrase, we return a err code to the user who can send
 * a message to KSshProcess, using one of several methods, to correct
 * the error.
 *
 * Determining when the ssh connection has successfully authenticationed has
 * proved to be the most difficult challenge.  OpenSSH does not print a message
 * on successful authentication, thus the only way to know is to send data
 * and wait for a return.  The problem here is sometimes it can take a bit
 * to establish the connection (for example, do to DNS lookups).  This means
 * the user may be sitting there waiting for a connection that failed.
 * Instead, ssh is always started with the verbose flag.  Then we look for
 * a message that indicates auth succeeded.  This is hazardous because
 * debug messages are more likely to change between OpenSSH releases.
 * Thus, we could become incompatible with new OpenSSH releases.
 */

#include <config.h>

#include "ksshprocess.h"

#include <stdio.h>
#include <errno.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <kstandarddirs.h>
#include <klocale.h>
#include <qregexp.h>
//Added by qt3to4:
#include <Q3CString>

/*
 * The following are tables of string and regexps we match
 * against the output of ssh.  An entry in each array 
 * corresponds the the version of ssh found in versionStrs[].
 * 
 * The version strings must be ordered in the array from most
 * specific to least specific in cases where the beginning
 * of several version strings are the similar. For example,
 * consider the openssh version strings.  The generic "OpenSSH"
 * must be the last of the openssh version strings in the array
 * so that is matched last. We use these generic version strings
 * so we can do a best effor to  support unknown ssh versions.
 */
QRegExp KSshProcess::versionStrs[] = {
    QRegExp("OpenSSH_3\\.[6-9]|OpenSSH_[1-9]*[4-9]\\.[0-9]"),
    QRegExp("OpenSSH"),
    QRegExp("SSH Secure Shell")
};

const char * const KSshProcess::passwordPrompt[] = {
    "password:", // OpenSSH
    "password:", // OpenSSH
    "password:"  // SSH
};

const char * const KSshProcess::passphrasePrompt[] = {
    "Enter passphrase for key",
    "Enter passphrase for key",
    "Passphrase for key"
};

const char * const KSshProcess::authSuccessMsg[] = {
    "Authentication succeeded",
    "ssh-userauth2 successful",
    "Received SSH_CROSS_AUTHENTICATED packet"
};

const char* const KSshProcess::authFailedMsg[] = {
    "Permission denied (",
    "Permission denied (",
    "Authentication failed."
};

const char* const KSshProcess::tryAgainMsg[] = {
    "please try again",
    "please try again",
    "adjfhjsdhfdsjfsjdfhuefeufeuefe"
};

QRegExp KSshProcess::hostKeyMissingMsg[] = {
    QRegExp("The authenticity of host|No (DSA|RSA) host key is known for"),
    QRegExp("The authenticity of host|No (DSA|RSA) host key is known for"),
    QRegExp("Host key not found from database")
};

const char* const KSshProcess::continuePrompt[] = {
    "Are you sure you want to continue connecting (yes/no)?",
    "Are you sure you want to continue connecting (yes/no)?",
    "Are you sure you want to continue connecting (yes/no)?"
};

const char* const KSshProcess::hostKeyChangedMsg[] = {
    "WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!",
    "WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!",
    "WARNING: HOST IDENTIFICATION HAS CHANGED!"
};

QRegExp KSshProcess::keyFingerprintMsg[] = {
    QRegExp("..(:..){15}"),
    QRegExp("..(:..){15}"),
    QRegExp(".....(-.....){10}")
};

QRegExp KSshProcess::knownHostsFileMsg[] = {
    QRegExp("Add correct host key in (.*) to get rid of this message."),
    QRegExp("Add correct host key in (.*) to get rid of this message."),
    QRegExp("Add correct host key to \"(.*)\"")
};


// This prompt only applies to commerical ssh.
const char* const KSshProcess::changeHostKeyOnDiskPrompt[] = {
    "as;jf;sajkfdslkfjas;dfjdsa;fj;dsajfdsajf",
    "as;jf;sajkfdslkfjas;dfjdsa;fj;dsajfdsajf",
    "Do you want to change the host key on disk (yes/no)?"
};

// We need this in addition the authFailedMsg because when
// OpenSSH gets a changed host key it will fail to connect
// depending on the StrictHostKeyChecking option.  Depending
// how this option is set, it will print "Permission denied"
// and quit, or print "Host key verification failed." and
// quit.  The later if StrictHostKeyChecking is "no".
// The former if StrictHostKeyChecking is
// "yes" or explicitly set to "ask".
QRegExp KSshProcess::hostKeyVerifyFailedMsg[] = {
    QRegExp("Host key verification failed\\."),
    QRegExp("Host key verification failed\\."),
    QRegExp("Disconnected; key exchange or algorithm? negotiation failed \\(Key exchange failed\\.\\)\\.")
};

const char * const KSshProcess::connectionClosedMsg[] = {
    "Connection closed by remote host",
    "Connection closed by remote host",
    "Connection closed by remote host"
};


void KSshProcess::SIGCHLD_handler(int) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void KSshProcess::installSignalHandlers() {
    struct sigaction act;
    memset(&act,0,sizeof(act));
    act.sa_handler = SIGCHLD_handler;
    act.sa_flags = 0
#ifdef SA_NOCLDSTOP
	    | SA_NOCLDSTOP
#endif
#ifdef SA_RESTART
	    | SA_RESTART
#endif
	    ;
    sigaction(SIGCHLD,&act,NULL);
}
					
void KSshProcess::removeSignalHandlers() {
	struct sigaction act;
	memset(&act,0,sizeof(act));
	act.sa_handler = SIG_DFL;
	sigaction(SIGCHLD,&act,NULL);
}

KSshProcess::KSshProcess() 
            : mVersion(UNKNOWN_VER), mConnected(false), 
        mRunning(false), mConnectState(0) {
    mSshPath = KStandardDirs::findExe(QString::fromLatin1("ssh"));
    kDebug(KSSHPROC) << "KSshProcess::KSshProcess(): ssh path [" << 
		mSshPath << "]" << endl;
        
	installSignalHandlers();
}

KSshProcess::KSshProcess(QString pathToSsh)
            : mSshPath(pathToSsh), mVersion(UNKNOWN_VER), mConnected(false),
    mRunning(false), mConnectState(0)  {
	installSignalHandlers();
}

KSshProcess::~KSshProcess(){
    disconnect();
    removeSignalHandlers();
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

bool KSshProcess::setSshPath(QString pathToSsh) {
    mSshPath = pathToSsh;
    version();
    if( mVersion == UNKNOWN_VER )
        return false;

    return true;
}

KSshProcess::SshVersion KSshProcess::version() {
    QString cmd;
    cmd = mSshPath+" -V 2>&1";

    // Get version string from ssh client.
    FILE *p;
    if( (p = popen(cmd.toLatin1(), "r")) == NULL ) {
        kDebug(KSSHPROC) << "KSshProcess::version(): "
            "failed to start ssh: " << strerror(errno) << endl;
        return UNKNOWN_VER;
    }

    // Determine of the version from the version string. 
    size_t len;
    char buf[128];
    if( (len = fread(buf, sizeof(char), sizeof(buf)-1, p)) == 0 ) {
        kDebug(KSSHPROC) << "KSshProcess::version(): "
            "Read of ssh version string failed " << 
             strerror(ferror(p)) << endl;
        return UNKNOWN_VER;
    }
    if( pclose(p) == -1 ) {
        kError(KSSHPROC) << "KSshProcess::version(): pclose failed." << endl;
    }
    buf[len] = '\0';
    QString ver;
    ver = buf;
    kDebug(KSSHPROC) << "KSshProcess::version(): "
        "got version string [" << ver << "]" << endl;

    mVersion = UNKNOWN_VER;
    for(int i = 0; i < SSH_VER_MAX; i++) {
        if( ver.indexOf(versionStrs[i]) != -1 ) {
             mVersion = (SshVersion)i;
             break;
        }
    }

    kDebug(KSSHPROC) << "KSshPRocess::version(): version number = "
	    << mVersion << endl;
    
    if( mVersion == UNKNOWN_VER ) {
        kDebug(KSSHPROC) << "KSshProcess::version(): "
            "Sorry, I don't know about this version of ssh" << endl;
        mError = ERR_UNKNOWN_VERSION;
        return UNKNOWN_VER;
    }

    return mVersion;
}
/*
QString KSshProcess::versionStr() {
    if( mVersion == UNKNOWN_VER ) {
        version();
        if( mVersion == UNKNOWN_VER )
            return QString();
    }

    return QString::fromLatin1(versionStrs[mVersion]);
}
*/

bool KSshProcess::setOptions(const SshOptList& opts) {
    kDebug(KSSHPROC) << "KSshProcess::setOptions()" << endl;
    mArgs.clear();
    SshOptListConstIterator it;
    QString cmd, subsystem;
    mPassword = mUsername = mHost = QString();
    Q3CString tmp;
    for(it = opts.begin(); it != opts.end(); ++it) {
        //kDebug(KSSHPROC) << "opt.opt = " << (*it).opt << endl;
        //kDebug(KSSHPROC) << "opt.str = " << (*it).str << endl;
        //kDebug(KSSHPROC) << "opt.num = " << (*it).num << endl;
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
            mArgs.append((*it).str.toLatin1());
            mUsername = (*it).str;
            break;

        case SSH_PASSWD:
            mPassword = (*it).str;
            break;

        case SSH_PROTOCOL:
            if( mVersion <= OPENSSH ) {
                tmp = "Protocol=";
                tmp += QString::number((*it).num).toLatin1();
                mArgs.append("-o");
                mArgs.append(tmp);
            }
            else if( mVersion <= SSH ) {
                if( (*it).num == 1 ) {
                    mArgs.append("-1");
                }
                // else uses version 2 by default
            }
            break;

        case SSH_FORWARDX11:
            tmp = "ForwardX11=";
            tmp += (*it).boolean ? "yes" : "no";
            mArgs.append("-o");
            mArgs.append(tmp);
            break;

        case SSH_FORWARDAGENT:
            tmp = "ForwardAgent=";
            tmp += (*it).boolean ? "yes" : "no";
            mArgs.append("-o");
            mArgs.append(tmp);
            break;

        case SSH_ESCAPE_CHAR:
            if( (*it).num == -1 )
                tmp = "none";
            else
                tmp = (char)((*it).num);
            mArgs.append("-e");
            mArgs.append(tmp);
            break;

        case SSH_OPTION:
            // don't allow NumberOfPasswordPrompts or StrictHostKeyChecking
            // since KSshProcess depends on specific setting of these for 
            // preforming authentication correctly.
            tmp = (*it).str.toLatin1();
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
            kDebug(KSSHPROC) << "KSshProcess::setOptions(): "
                "unrecognized ssh opt " << (*it).opt << endl;
        }
    }

    if( !subsystem.isEmpty() && !cmd.isEmpty() ) {
        kDebug(KSSHPROC) << "KSshProcess::setOptions(): "
            "cannot use a subsystem and command at the same time" << endl;
        mError = ERR_CMD_SUBSYS_CONFLICT;
        mErrorMsg = i18n("Cannot specify a subsystem and command at the same time.");
        return false;
    }

    // These options govern the behavior of ssh and 
    // cannot be defined by the user
    //mArgs.append("-o");
    //mArgs.append("StrictHostKeyChecking=ask");
    mArgs.append("-v"); // So we get a message that the 
                        // connection was successful
    if( mVersion <= OPENSSH ) {
        // nothing
    }
    else if( mVersion <= SSH ) {
        mArgs.append("-o"); // So we can check if the connection was successful
        mArgs.append("AuthenticationSuccessMsg=yes");
    }

    if( mHost.isEmpty() ) {
        kDebug(KSSHPROC) << "KSshProcess::setOptions(): "
            "a host name must be supplied" << endl;
        return false;
    }
    else {
        mArgs.append(mHost.toLatin1());
    }

    if( !subsystem.isEmpty() ) {
        mArgs.append("-s");
        mArgs.append(subsystem.toLatin1());
    }

    if( !cmd.isEmpty() ) {
        mArgs.append(cmd.toLatin1());
    }

    return true;
}

void KSshProcess::printArgs() {
    QList<Q3CString>::Iterator it;
    for( it = mArgs.begin(); it != mArgs.end(); ++it) {
        kDebug(KSSHPROC) << "arg: " << *it << endl;
    }
}


int KSshProcess::error(QString& msg) {
    kDebug(KSSHPROC) << "KSshProcess::error()" << endl;
    kDebug() << mErrorMsg << endl;
    msg = mErrorMsg;
    return mError;
}

void KSshProcess::kill(int signal) {
    int pid = ssh.pid();
    
    kDebug(KSSHPROC) << "KSshProcess::kill(signal:" << signal 
                      << "): ssh pid is " << pid << endl;
    kDebug(KSSHPROC) << "KSshPRocess::kill(): we are " 
                      << (mConnected ? "" : "not ") << "connected" << endl;
    kDebug(KSSHPROC) << "KSshProcess::kill(): we are " 
                      << (mRunning ? "" : "not ") << "running a ssh process" << endl;

    if( mRunning && pid > 1 ) {
            // Kill the child process...
            if ( ::kill(pid, signal) == 0 ) {
                // clean up if we tried to kill the process
                if( signal == SIGTERM || signal == SIGKILL ) {
                    while(waitpid(-1, NULL, WNOHANG) > 0);
                    mConnected = false;
                    mRunning = false;
                }
            }
            else
                kDebug(KSSHPROC) << "KSshProcess::kill(): kill failed" << endl;
    }
    else
        kDebug(KSSHPROC) << "KSshProcess::kill(): "
            "Refusing to kill ssh process" << endl;
}



/**
 * Try to open an ssh connection.
 * SSH prints certain messages to certain file descriptiors:
 *    passwordPrompt - pty
 *    passphrasePrompt - pty
 *    authSuccessMsg - stderr (OpenSSH), 
 *    authFailedMsg - stderr
 *    hostKeyMissing - stderr
 *    hostKeyChanged - stderr
 *    continuePrompt - stderr
 * 
 * We will use a select to wait for a line on each descriptor. Then get
 * each line that available and take action based on it.  The type
 * of messages we are looking for and the action we take on each
 * message are:
 *   passwordPrompt - Return false, set error to ERR_NEED_PASSWD.
 *                    On the next call to connect() we expect a password
 *                    to be available.
 *                    
 *   passpharsePrompt - Return false, set error to ERR_NEED_PASSPHRASE.
 *                      On the next call to connect() we expect a
 *                      passphrase to be available.
 *   
 *   authSuccessMsg - Return true, as we have successfully established a
 *                    ssh connection.
 *   
 *   authFailedMsg - Return false, set error to ERR_AUTH_FAILED. We
 *                   were unable to authenticate the connection given
 *                   the available authentication information.
 *   
 *   hostKeyMissing - Return false, set error to ERR_NEW_HOST_KEY. Caller
 *                    must call KSshProcess.acceptHostKey(bool) to accept
 *                    or reject the key before calling connect() again.
 *   
 *   hostKeyChanged - Return false, set error to ERR_DIFF_HOST_KEY. Caller
 *                    must call KSshProcess.acceptHostKey(bool) to accept
 *                    or reject the key before calling connect() again.
 *   
 *   continuePrompt - Send 'yes' or 'no' to accept or reject a key,
 *                    respectively.
 *
 */


void KSshProcess::acceptHostKey(bool accept) {
    kDebug(KSSHPROC) << "KSshProcess::acceptHostKey(accept:"
        << accept << ")" << endl;
    mAcceptHostKey = accept;
}

void KSshProcess::setPassword(QString password) {
    kDebug(KSSHPROC) << "KSshProcess::setPassword(password:xxxxxxxx)" << endl;
    mPassword = password;
}

QString KSshProcess::getLine() {
    static QStringList buffer;
    QString line = QString();
    Q3CString ptyLine, errLine;

    if( buffer.empty() ) {
        // PtyProcess buffers lines.  First check that there
        // isn't something on the PtyProces buffer or that there
        // is not data ready to be read from the pty or stderr.
        ptyLine = ssh.readLineFromPty(false);
        errLine = ssh.readLineFromStderr(false);

        // If PtyProcess did have something for us, get it and
        // place it in our line buffer.
        if( ! ptyLine.isEmpty() ) {
            buffer.prepend(QString(ptyLine));
        }

        if( ! errLine.isEmpty() ) {
            buffer.prepend(QString(errLine));
        }

        // If we still don't have anything in our buffer so there must
        // not be anything on the pty or stderr. Setup a select()
        // to wait for some data from SSH.
        if( buffer.empty() ) {
            //kDebug(KSSHPROC) << "KSshProcess::getLine(): " <<
            //    "Line buffer empty, calling select() to wait for data." << endl;
            int errfd = ssh.stderrFd();
            int ptyfd = ssh.fd();
            fd_set rfds;
            fd_set efds;
            struct timeval tv;
            
            // find max file descriptor
            int maxfd = ptyfd > errfd ? ptyfd : errfd;          
            
            FD_ZERO(&rfds);
            FD_SET(ptyfd, &rfds);      // Add pty file descriptor
            FD_SET(errfd, &rfds);      // Add std error file descriptor
    
            FD_ZERO(&efds);
            FD_SET(ptyfd, &efds);
            FD_SET(errfd, &efds);
   
            tv.tv_sec = 60; tv.tv_usec = 0; // 60 second timeout
    
            // Wait for a message from ssh on stderr or the pty.
            int ret = -1;
            do 
              ret = ::select(maxfd+1, &rfds, NULL, &efds, &tv);
            while( ret == -1 && errno == EINTR );
    
            // Handle any errors from select
            if( ret == 0 ) {
                kDebug(KSSHPROC) << "KSshProcess::connect(): " <<
                    "timed out waiting for a response" << endl;
                mError = ERR_TIMED_OUT;
                return QString();
            }
            else if( ret == -1 ) {
                kDebug(KSSHPROC) << "KSshProcess::connect(): "
                    << "select error: " << strerror(errno) << endl;
                mError = ERR_INTERNAL;
                return QString();
            }
    
            // We are not respecting any type of order in which the
            // lines were received. Who knows whether pty or stderr
            // had data on it first.
            if( FD_ISSET(ptyfd, &rfds) ) {
                ptyLine = ssh.readLineFromPty(false);
                buffer.prepend(QString(ptyLine));
                //kDebug(KSSHPROC) << "KSshProcess::getLine(): "
                //    "line from pty -" << ptyLine  << endl;
            }
            
            if( FD_ISSET(errfd, &rfds) ) {
                errLine = ssh.readLineFromStderr(false);
                buffer.prepend(QString(errLine));
                //kDebug(KSSHPROC) << "KSshProcess::getLine(): "
                //    "line from err -" << errLine << endl;
            }

            if( FD_ISSET(ptyfd, &efds) ) {
                kDebug(KSSHPROC) << "KSshProcess::getLine(): "
                    "Exception on pty file descriptor." << endl;
            }

            if( FD_ISSET(errfd, &efds) ) {
                kDebug(KSSHPROC) << "KSshProcess::getLine(): "
                    "Exception on std err file descriptor." << endl;
            }
            
        }
    }
   
    // We should have something in our buffer now.
    // Return the last line.
    //it = buffer.end();
    //line = *it;
    //buffer.remove(it);

    line = buffer.last();
    buffer.pop_back();

    if( line.isNull() && buffer.count() > 0 ) {
        line = buffer.last();
        buffer.pop_back();
    }
    
//    kDebug(KSSHPROC) << "KSshProcess::getLine(): " << 
//        buffer.count() << " lines in buffer" << endl;
    kDebug(KSSHPROC) << "KSshProcess::getLine(): "
        "ssh: " << line << endl;
    

    return line;
}

// All the different states we could go through while trying to connect.
enum sshConnectState {
    STATE_START, STATE_TRY_PASSWD, STATE_WAIT_PROMPT, STATE_NEW_KEY_CONTINUE,
    STATE_DIFF_KEY_CONTINUE, STATE_FATAL, STATE_WAIT_CONTINUE_PROMPT,
    STATE_SEND_CONTINUE, STATE_AUTH_FAILED, STATE_NEW_KEY_WAIT_CONTINUE,
    STATE_DIFF_KEY_WAIT_CONTINUE, STATE_TRY_PASSPHRASE
};

// Print the state as a string. Good for debugging
const char* stateStr(int state) {
    switch(state) {
        case STATE_START:
            return "STATE_START";
        case STATE_TRY_PASSWD:
            return "STATE_TRY_PASSWD";
        case STATE_WAIT_PROMPT:
            return "STATE_WAIT_PROMPT";
        case STATE_NEW_KEY_CONTINUE:
            return "STATE_NEW_KEY_CONTINUE";
        case STATE_DIFF_KEY_CONTINUE:
            return "STATE_DIFF_KEY_CONTINUE";
        case STATE_FATAL:
            return "STATE_FATAL";
        case STATE_WAIT_CONTINUE_PROMPT:
            return "STATE_WAIT_CONTINUE_PROMPT";
        case STATE_SEND_CONTINUE:
            return "STATE_SEND_CONTINE";
        case STATE_AUTH_FAILED:
            return "STATE_AUTH_FAILED";
        case STATE_NEW_KEY_WAIT_CONTINUE:
            return "STATE_NEW_KEY_WAIT_CONTINUE";
        case STATE_DIFF_KEY_WAIT_CONTINUE:
            return "STATE_DIFF_KEY_WAIT_CONTINUE";
        case STATE_TRY_PASSPHRASE:
            return "STATE_TRY_PASSPHRASE";
    }
    return "UNKNOWN";
}

bool KSshProcess::connect() {
    if( mVersion == UNKNOWN_VER ) {
        // we don't know the ssh version yet, so find out
        version();
        if( mVersion == UNKNOWN_VER ) {
            return false;
        }
    }

    // We'll put a limit on the number of state transitions
    // to ensure we don't go out of control.
    int transitionLimit = 500;

    while(--transitionLimit) {
        kDebug(KSSHPROC) << "KSshProcess::connect(): "
            << "Connect state " << stateStr(mConnectState) << endl;
        
        QString line;      // a line from ssh
        QString msgBuf;    // buffer for important messages from ssh
                           // which are to be returned to the user
        
        switch(mConnectState) {
        // STATE_START:
        // Executes the ssh binary with the options provided.  If no options
        // have been specified, sets error and returns false. Continue to
        // state 1 if execution is successful, otherwise set error and 
        // return false.
        case STATE_START:
            // reset some key values to safe values
            mAcceptHostKey = false;
            mKeyFingerprint.clear();
            mKnownHostsFile.clear();
            
            if( mArgs.isEmpty() ) {
                kDebug(KSSHPROC) << "KSshProcess::connect(): ssh options "
                    "need to be set first using setArgs()" << endl;
                mError = ERR_NO_OPTIONS;
                mErrorMsg = i18n("No options provided for ssh execution.");
                return false;
            }

            if( ssh.exec(mSshPath.toLatin1(), mArgs) ) {
                kDebug(KSSHPROC) << 
                    "KSshProcess::connect(): ssh exec failed" << endl;
                mError = ERR_CANNOT_LAUNCH;
                mErrorMsg = i18n("Failed to execute ssh process.");
                return false;
            }
           
            kDebug(KSSHPROC) << "KSshPRocess::connect(): ssh pid = " << ssh.pid() << endl;
	    
            // set flag to indicate what have started a ssh process
            mRunning = true;
            mConnectState = STATE_WAIT_PROMPT;
            break;
        
        // STATE_WAIT_PROMPT:
        // Get a line of input from the ssh process. Check the contents 
        // of the line to determine the next state. Ignore the line
        // if we don't recognize its contents.  If the line contains
        // the continue prompt, we have an error since we should never
        // get that line in this state.  Set ERR_INVALID_STATE error
        // and return false.
        case STATE_WAIT_PROMPT:
            line = getLine();
            if( line.isNull() ) {
                kDebug(KSSHPROC) << "KSshProcess::connect(): "
                    "Got null line in STATE_WAIT_PROMPT." << endl;
                mError = ERR_INTERACT;
                mErrorMsg =
                    i18n("Error encountered while talking to ssh.");
                mConnectState = STATE_FATAL;
            }
            else if( line.indexOf(QString::fromLatin1(passwordPrompt[mVersion]), 0, Qt::CaseInsensitive) != -1 ) {
                mConnectState = STATE_TRY_PASSWD;
            }
            else if( line.indexOf(passphrasePrompt[mVersion]) != -1 ) {
                mConnectState = STATE_TRY_PASSPHRASE;
            }
            else if( line.indexOf(authSuccessMsg[mVersion]) != -1 ) {
                return true;
            }
            else if( (line.indexOf(authFailedMsg[mVersion]) != -1)
                    && (line.indexOf(tryAgainMsg[mVersion]) == -1) ) {
                mConnectState = STATE_AUTH_FAILED;
            }
            else if( line.indexOf(hostKeyMissingMsg[mVersion]) != -1 ) {
                mConnectState = STATE_NEW_KEY_WAIT_CONTINUE;
            }
            else if( line.indexOf(hostKeyChangedMsg[mVersion]) != -1 ) {
                mConnectState = STATE_DIFF_KEY_WAIT_CONTINUE;
            }
            else if( line.indexOf(continuePrompt[mVersion]) != -1 ) {
                //mConnectState = STATE_SEND_CONTINUE;
                kDebug(KSSHPROC) << "KSshProcess:connect(): "
                    "Got continue prompt where we shouldn't (STATE_WAIT_PROMPT)"
                    << endl;
                mError = ERR_INTERACT;
                mErrorMsg =
                    i18n("Error encountered while talking to ssh.");
            }
            else if( line.indexOf(connectionClosedMsg[mVersion]) != -1 ) {
                mConnectState = STATE_FATAL;
                mError = ERR_CLOSED_BY_REMOTE_HOST;
                mErrorMsg = i18n("Connection closed by remote host.");
            }
            else if( line.indexOf(changeHostKeyOnDiskPrompt[mVersion]) != -1 ) {
                // always say yes to this.  It always comes after commerical ssh
                // prints a "continue to connect prompt". We assume that if the
                // user choose to continue, then they also want to save the
                // host key to disk.
                ssh.writeLine("yes");
            }
            else {
                // ignore line
            }
            break; 

        // STATE_TRY_PASSWD:
        // If we have password send it to the ssh process, else
        // set error ERR_NEED_PASSWD and return false to the caller.
        // The caller then must then call KSshProcess::setPassword(QString)
        // before calling KSshProcess::connect() again.
        //
        // Almost exactly liek STATE_TRY_PASSPHRASE.  Check there if you
        // make changes here.
        case STATE_TRY_PASSWD:
            // We have a password prompt waiting for us to supply
            // a password.  Send that password to ssh.  If the caller
            // did not supply a password like we asked, then ask
            // again.
            if( !mPassword.isEmpty() ) {
//                ssh.WaitSlave();
                ssh.writeLine(mPassword.toLatin1());
                
                // Overwrite the password so it isn't in memory.
                mPassword.fill(QChar('X'));
                
                // Set the password to null so we will request another
                // password if this one fails.
                mPassword.clear();
                
                mConnectState = STATE_WAIT_PROMPT;
            }
            else {
                kDebug(KSSHPROC) << "KSshProcess::connect() "
                    "Need password from caller." << endl;
                // The caller needs to supply a password before
                // connecting can continue.
                mError = ERR_NEED_PASSWD;
                mErrorMsg = i18n("Please supply a password.");
                mConnectState = STATE_TRY_PASSWD;
                return false;
            }
            break;

        // STATE_TRY_KEY_PASSPHRASE:
        // If we have passphrase send it to the ssh process, else
        // set error ERR_NEED_PASSPHRASE and return false to the caller.
        // The caller then must then call KSshProcess::setPassword(QString)
        // before calling KSshProcess::connect() again.
        //
        // Almost exactly like STATE_TRY_PASSWD. The only difference is 
        // the error we set if we don't have a passphrase.  We duplicate
        // this code to keep in the spirit of the state machine.
        case STATE_TRY_PASSPHRASE:
            // We have a passphrase prompt waiting for us to supply
            // a passphrase.  Send that passphrase to ssh.  If the caller
            // did not supply a passphrase like we asked, then ask
            // again.
            if( !mPassword.isEmpty() ) {
//                ssh.WaitSlave();
                ssh.writeLine(mPassword.toLatin1());
                
                // Overwrite the password so it isn't in memory.
                mPassword.fill(QChar('X'));
                
                // Set the password to null so we will request another
                // password if this one fails.
                mPassword.clear();
                
                mConnectState = STATE_WAIT_PROMPT;
            }
            else {
                kDebug(KSSHPROC) << "KSshProcess::connect() "
                    "Need passphrase from caller." << endl;
                // The caller needs to supply a passphrase before
                // connecting can continue.
                mError = ERR_NEED_PASSPHRASE;
                mErrorMsg = i18n("Please supply the passphrase for " 
                    "your SSH private key.");
                mConnectState = STATE_TRY_PASSPHRASE;
                return false;
            }
            break;

        // STATE_AUTH_FAILED:
        // Authentication has failed.  Tell the caller by setting the
        // ERR_AUTH_FAILED error and returning false. If
        // auth has failed then ssh should have exited, but
        // we will kill it to make sure. 
        case STATE_AUTH_FAILED:
            mError = ERR_AUTH_FAILED;
            mErrorMsg = i18n("Authentication to %1 failed", mHost);
            mConnectState = STATE_FATAL;
            break; 
        
        // STATE_NEW_KEY_WAIT_CONTINUE:
        // Grab lines from ssh until we get a continue prompt or a auth
        // denied.  We will get the later if StrictHostKeyChecking is set
        // to yes.  Go to STATE_NEW_KEY_CONTINUE if we get a continue prompt.
        case STATE_NEW_KEY_WAIT_CONTINUE:
            line = getLine();
            if( line.isNull() ) {
                kDebug(KSSHPROC) << "KSshProcess::connect(): "
                    "Got null line in STATE_NEW_KEY_WAIT_CONTINUE." << endl;
                mError = ERR_INTERACT;
                mErrorMsg =
                    i18n("Error encountered while talking to ssh.");
                mConnectState = STATE_FATAL;
            }
            else if( ((line.indexOf(authFailedMsg[mVersion]) != -1)
                           && (line.indexOf(tryAgainMsg[mVersion]) == -1))
                    || (line.indexOf(hostKeyVerifyFailedMsg[mVersion]) != -1) ) {
                mError = ERR_AUTH_FAILED_NEW_KEY;
                mErrorMsg = i18n(
                    "The identity of the remote host '%1' could not be verified "
                    "because the host's key is not in the \"known hosts\" file."
                , mHost);
                
                if( mKnownHostsFile.isEmpty() ) {
                    mErrorMsg += i18n(
                        " Manually, add the host's key to the \"known hosts\" "
                        "file or contact your administrator."
                    );
                }
                else {
                    mErrorMsg += i18n(
                        " Manually, add the host's key to %1 "
                        "or contact your administrator."
                    , mKnownHostsFile);
                }

                mConnectState = STATE_FATAL;
            }
            else if( line.indexOf(continuePrompt[mVersion]) != -1 ) {
                mConnectState = STATE_NEW_KEY_CONTINUE;
            }
            else if( line.indexOf(connectionClosedMsg[mVersion]) != -1 ) {
                mConnectState = STATE_FATAL;
                mError = ERR_CLOSED_BY_REMOTE_HOST;
                mErrorMsg = i18n("Connection closed by remote host.");
            }
            else if( line.indexOf(keyFingerprintMsg[mVersion]) != -1 ) {
                mKeyFingerprint = keyFingerprintMsg[mVersion].cap();
                kDebug(KSSHPROC) << "Found key fingerprint: " << mKeyFingerprint << endl;
                mConnectState = STATE_NEW_KEY_WAIT_CONTINUE;
            }
            else {
                // ignore line
            }
            break; 
            
        
        // STATE_NEW_KEY_CONTINUE:
        // We got a continue prompt for the new key message.  Set the error 
        // message to reflect this, return false and hope for caller response.
        case STATE_NEW_KEY_CONTINUE:
            mError = ERR_NEW_HOST_KEY;
            mErrorMsg = i18n(
                "The identity of the remote host '%1' could not be "
                "verified. The host's key fingerprint is:\n%2\nYou should "
                "verify the fingerprint with the host's administrator before "
                "connecting.\n\n"
                "Would you like to accept the host's key and connect anyway? "
            , mHost, mKeyFingerprint);
            mConnectState = STATE_SEND_CONTINUE;
            return false;

        // STATE_DIFF_KEY_WAIT_CONTINUE:
        // Grab lines from ssh until we get a continue prompt or a auth
        // denied.  We will get the later if StrictHostKeyChecking is set
        // to yes.  Go to STATE_DIFF_KEY_CONTINUE if we get a continue prompt.
        case STATE_DIFF_KEY_WAIT_CONTINUE:
            line = getLine();
            if( line.isNull() ) {
                kDebug(KSSHPROC) << "KSshProcess::connect(): "
                    "Got null line in STATE_DIFF_KEY_WAIT_CONTINUE." << endl;
                mError = ERR_INTERACT;
                mErrorMsg =
                    i18n("Error encountered while talking to ssh.");
                mConnectState = STATE_FATAL;
            }
            else if( ((line.indexOf(authFailedMsg[mVersion]) != -1)
                           && (line.indexOf(tryAgainMsg[mVersion]) == -1))
                    || (line.indexOf(hostKeyVerifyFailedMsg[mVersion]) != -1) ) {
                mError = ERR_AUTH_FAILED_DIFF_KEY;
                mErrorMsg = i18n(
                    "WARNING: The identity of the remote host '%1' has changed!\n\n"
                    "Someone could be eavesdropping on your connection, or the "
                    "administrator may have just changed the host's key. "
                    "Either way, you should verify the host's key fingerprint with the host's "
                    "administrator. The key fingerprint is:\n%2\n"
                    "Add the correct host key to \"%3\" to "
                    "get rid of this message."
                , mHost, mKeyFingerprint, mKnownHostsFile);
                mConnectState = STATE_FATAL;
            }
            else if( line.indexOf(continuePrompt[mVersion]) != -1 ) {
                mConnectState = STATE_DIFF_KEY_CONTINUE;
            }
            else if( line.indexOf(keyFingerprintMsg[mVersion]) != -1 ) {
                mKeyFingerprint = keyFingerprintMsg[mVersion].cap();
                kDebug(KSSHPROC) << "Found key fingerprint: " << mKeyFingerprint << endl;
                mConnectState = STATE_DIFF_KEY_WAIT_CONTINUE;
            }
            else if( line.indexOf(knownHostsFileMsg[mVersion]) != -1 ) {
                mKnownHostsFile = (knownHostsFileMsg[mVersion]).cap(1);
                kDebug(KSSHPROC) << "Found known hosts file name: " << mKnownHostsFile << endl;
                mConnectState = STATE_DIFF_KEY_WAIT_CONTINUE;
            }
            else {
                // ignore line
            }
            break; 
            
        // STATE_DIFF_KEY_CONTINUE:
        // We got a continue prompt for the different key message. 
        // Set ERR_DIFF_HOST_KEY error
        // and return false to signal need to caller action.
        case STATE_DIFF_KEY_CONTINUE:
            mError = ERR_DIFF_HOST_KEY;
            mErrorMsg = i18n(
                "WARNING: The identity of the remote host '%1' has changed!\n\n"
                "Someone could be eavesdropping on your connection, or the "
                "administrator may have just changed the host's key. "
                "Either way, you should verify the host's key fingerprint with the host's "
                "administrator before connecting. The key fingerprint is:\n%2\n\n"
                "Would you like to accept the host's new key and connect anyway?"
            , mHost, mKeyFingerprint);
            mConnectState = STATE_SEND_CONTINUE;
            return false;

        // STATE_SEND_CONTINUE:
        // We found a continue prompt.  Send our answer.
        case STATE_SEND_CONTINUE:
            if( mAcceptHostKey ) {
                kDebug(KSSHPROC) << "KSshProcess::connect(): "
                    "host key accepted" << endl;
                ssh.writeLine("yes");
                mConnectState = STATE_WAIT_PROMPT;
            }
            else {
                kDebug(KSSHPROC) << "KSshProcess::connect(): "
                    "host key rejected" << endl;
                ssh.writeLine("no");
                mError = ERR_HOST_KEY_REJECTED;
                mErrorMsg = i18n("Host key was rejected.");
                mConnectState = STATE_FATAL;
            }
            break;

        // STATE_FATAL:
        // Something bad happened that we cannot recover from.
        // Kill the ssh process and set flags to show we have
        // ended the connection and killed ssh.
        // 
        // mError and mErrorMsg should be set by the immediately
        // previous state.
        case STATE_FATAL:
            kill();
            mConnected = false;
            mRunning = false;
            mConnectState = STATE_START;
            // mError, mErroMsg set by last state
            return false;
        
        default:
            kDebug(KSSHPROC) << "KSshProcess::connect(): "
                "Invalid state number - " << mConnectState << endl;
            mError = ERR_INVALID_STATE;
            mConnectState = STATE_FATAL;
        }
    }

    // we should never get here
    kDebug(KSSHPROC) << "KSshProcess::connect(): " <<
        "After switch(). We shouldn't be here." << endl;
    mError = ERR_INTERNAL;
    return false;
}
            
void KSshProcess::disconnect() {
    kill();
    mConnected = false;
    mRunning = false;
    mConnectState = STATE_START;
}

