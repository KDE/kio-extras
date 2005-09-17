/***************************************************************************
                          fish.cpp  -  a FISH kioslave
                             -------------------
    begin                : Thu Oct  4 17:09:14 CEST 2001
    copyright            : (C) 2001-2003 by Jörg Walter
    email                : jwalt-kde@garni.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, version 2 of the License                *
 *                                                                         *
 ***************************************************************************/

/*
  This code contains fragments and ideas from the ftp kioslave
  done by David Faure <faure@kde.org>.

  Structure is a bit complicated, since I made the mistake to use
  KProcess... now there is a lightweight homebrew async IO system
  inside, but if signals/slots become available for ioslaves, switching
  back to KProcess should be easy.
*/

#include "config.h"

#include <q3cstring.h>
#include <qfile.h>
#include <q3socket.h>
#include <qdatetime.h>
#include <qbitarray.h>
#include <qregexp.h>

#include <stdlib.h>
#ifdef HAVE_PTY_H
#include <pty.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#ifdef HAVE_STROPTS
#include <stropts.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#endif
#ifdef HAVE_UTIL_H
#include <util.h>
#endif

#include <kdebug.h>
#include <kmessagebox.h>
#include <kinstance.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kremoteencoding.h>
#include <kurl.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <kmimetype.h>
#include <kmimemagic.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/resource.h>

#include "fish.h"
#include "fishcode.h"

#ifndef NDEBUG
#define myDebug(x) kdDebug(7127) << __LINE__ << ": " x
#define connected() do{myDebug( << "_______ emitting connected()" << endl); connected();}while(0)
#define dataReq() do{myDebug( << "_______ emitting dataReq()" << endl); dataReq();}while(0)
#define needSubURLData() do{myDebug( << "_______ emitting needSubURLData()" << endl); needSubURLData();}while(0)
#define slaveStatus(x,y) do{myDebug( << "_______ emitting slaveStatus(" << x << ", " << y << ")" << endl); slaveStatus(x,y);}while(0)
#define statEntry(x) do{myDebug( << "_______ emitting statEntry("<<x.size()<<")" << endl); statEntry(x);}while(0)
#define listEntries(x) do{myDebug( << "_______ emitting listEntries(...)" << endl); listEntries(x);}while(0)
#define canResume(x) do{myDebug( << "_______ emitting canResume("<<(int)x<<")" << endl); canResume(x);}while(0)
#define totalSize(x) do{myDebug( << "_______ emitting totalSize("<<(int)x<<")" << endl); totalSize(x);}while(0)
#define processedSize(x) do{myDebug( << "_______ emitting processedSize("<<x<<")" << endl); processedSize(x);}while(0)
#define speed(x) do{myDebug( << "_______ emitting speed("<<(int)x<<")" << endl); speed(x);}while(0)
#define redirection(x) do{myDebug( << "_______ emitting redirection("<<x<<")" << endl); redirection(x);}while(0)
#define errorPage() do{myDebug( << "_______ emitting errorPage()" << endl); errorPage();}while(0)
#define sendmimeType(x) do{myDebug( << "_______ emitting mimeType("<<x<<")" << endl); mimeType(x);}while(0)
#define warning(x) do{myDebug( << "_______ emitting warning("<<x<<")" << endl); warning(x);}while(0)
#define infoMessage(x) do{myDebug( << "_______ emitting infoMessage("<<x<<")" << endl); infoMessage(x);}while(0)
#else
#define myDebug(x)
#define sendmimeType(x) mimeType(x)
#endif

static char *sshPath = NULL;
static char *suPath = NULL;
// disabled: currently not needed. Didn't work reliably.
// static int isOpenSSH = 0;

#define E(x) ((const char*)remoteEncoding()->encode(x).data())

using namespace KIO;
extern "C" {

static void ripper(int)
{
    while (waitpid(-1,0,WNOHANG) > 0) {
      // do nothing, go on
    }
}

int KDE_EXPORT kdemain( int argc, char **argv )
{
    KLocale::setMainCatalogue("kio_fish");
    KInstance instance("fish");

    myDebug( << "*** Starting fish " << endl);
    if (argc != 4) {
        myDebug( << "Usage: fish  protocol domain-socket1 domain-socket2" << endl);
        exit(-1);
    }

    struct sigaction act;
    memset(&act,0,sizeof(act));
    act.sa_handler = ripper;
    act.sa_flags = 0
#ifdef SA_NOCLDSTOP
    | SA_NOCLDSTOP
#endif
#ifdef SA_RESTART
    | SA_RESTART
#endif
    ;
    sigaction(SIGCHLD,&act,NULL);

    fishProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    myDebug( << "*** fish Done" << endl);
    return 0;
}

}

const struct fishProtocol::fish_info fishProtocol::fishInfo[] = {
    { ("FISH"), 0,
      ("echo; /bin/sh -c start_fish_server > /dev/null 2>/dev/null; perl .fishsrv.pl " CHECKSUM " 2>/dev/null; perl -e '$|=1; print \"### 100 transfer fish server\\n\"; while(<STDIN>) { last if /^__END__/; $code.=$_; } exit(eval($code));' 2>/dev/null;"),
      1 },
    { ("VER 0.0.3 copy append lscount lslinks lsmime exec stat"), 0,
      ("echo 'VER 0.0.3 copy append lscount lslinks lsmime exec stat'"),
      1 },
    { ("PWD"), 0,
      ("pwd"),
      1 },
    { ("LIST"), 1,
      ("echo `ls -Lla %1 2> /dev/null | grep '^[-dsplcb]' | wc -l`; ls -Lla %1 2>/dev/null | grep '^[-dspl]' | ( while read -r p x u g s m d y n; do file -b -i $n 2>/dev/null | sed -e '\\,^[^/]*$,d;s/^/M/;s,/.*[ \t],/,'; FILE=%1; if [ -e %1\"/$n\" ]; then FILE=%1\"/$n\"; fi; if [ -L \"$FILE\" ]; then echo \":$n\"; ls -lad \"$FILE\" | sed -e 's/.* -> /L/'; else echo \":$n\" | sed -e 's/ -> /\\\nL/'; fi; echo \"P$p $u.$g\nS$s\nd$m $d $y\n\"; done; );"
                "ls -Lla %1 2>/dev/null | grep '^[cb]' | ( while read -r p x u g a i m d y n; do echo \"P$p $u.$g\nE$a$i\nd$m $d $y\n:$n\n\"; done; )"),
      0 },
    { ("STAT"), 1,
      ("echo `ls -dLla %1 2> /dev/null | grep '^[-dsplcb]' | wc -l`; ls -dLla %1 2>/dev/null | grep '^[-dspl]' | ( while read -r p x u g s m d y n; do file -b -i $n 2>/dev/null | sed -e '\\,^[^/]*$,d;s/^/M/;s,/.*[ \t],/,'; FILE=%1; if [ -e %1\"/$n\" ]; then FILE=%1\"/$n\"; fi; if [ -L \"$FILE\" ]; then echo \":$n\"; ls -lad \"$FILE\" | sed -e 's/.* -> /L/'; else echo \":$n\" | sed -e 's/ -> /\\\nL/'; fi; echo \"P$p $u.$g\nS$s\nd$m $d $y\n\"; done; );"
                "ls -dLla %1 2>/dev/null | grep '^[cb]' | ( while read -r p x u g a i m d y n; do echo \"P$p $u.$g\nE$a$i\nd$m $d $y\n:$n\n\"; done; )"),
      0 },
    { ("RETR"), 1,
      ("ls -l %1 2>&1 | ( read -r a b c d x e; echo $x ) 2>&1; echo '### 001'; cat %1"),
      1 },
    { ("STOR"), 2,
      ("> %2; echo '### 001'; ( [ \"`expr %1 / 4096`\" -gt 0 ] && dd bs=4096 count=`expr %1 / 4096` 2>/dev/null;"
              "[ \"`expr %1 % 4096`\" -gt 0 ] && dd bs=`expr %1 % 4096` count=1 2>/dev/null; ) | ( cat > %2 || echo Error $?; cat > /dev/null )"),
      0 },
    { ("CWD"), 1,
      ("cd %1"),
      0 },
    { ("CHMOD"), 2,
      ("chmod %1 %2"),
      0 },
    { ("DELE"), 1,
      ("rm -f %1"),
      0 },
    { ("MKD"), 1,
      ("mkdir %1"),
      0 },
    { ("RMD"), 1,
      ("rmdir %1"),
      0 },
    { ("RENAME"), 2,
      ("mv -f %1 %2"),
      0 },
    { ("LINK"), 2,
      ("ln -f %1 %2"),
      0 },
    { ("SYMLINK"), 2,
      ("ln -sf %1 %2"),
      0 },
    { ("CHOWN"), 2,
      ("chown %1 %2"),
      0 },
    { ("CHGRP"), 2,
      ("chgrp %1 %2"),
      0 },
    { ("READ"), 3,
      ("echo '### 100';cat %3 /dev/zero | ( [ \"`expr %1 / 4096`\" -gt 0 ] && dd bs=4096 count=`expr %1 / 4096` >/dev/null;"
              "[ \"`expr %1 % 4096`\" -gt 0 ] && dd bs=`expr %1 % 4096` count=1 >/dev/null;"
              "dd bs=%2 count=1; ) 2>/dev/null;"),
      0 },
    // Yes, this is "ibs=1", since dd "count" is input blocks.
    // On network connections, read() may not fill the buffer
    // completely (no more data immediately available), but dd
    // does ignore that fact by design. Sorry, writes are slow.
    // OTOH, WRITE is not used by the current ioslave methods,
    // we use APPEND.
    { ("WRITE"), 3,
      (">> %3; echo '### 001'; ( [ %2 -gt 0 ] && dd ibs=1 obs=%2 count=%2 2>/dev/null ) | "
              "( dd ibs=32768 obs=%1 seek=1 of=%3 2>/dev/null || echo Error $?; cat >/dev/null; )"),
      0 },
    { ("COPY"), 2,
      ("if [ -L %1 ]; then if cp -pdf %1 %2 2>/dev/null; then :; else LINK=\"`readlink %1`\"; ln -sf $LINK %2; fi; else cp -pf %1 %2; fi"),
      0 },
    { ("APPEND"), 2,
      (">> %2; echo '### 001'; ( [ %1 -gt 0 ] && dd ibs=1 obs=%1 count=%1 2> /dev/null; ) | ( cat >> %2 || echo Error $?; cat >/dev/null; )"),
      0 },
    { ("EXEC"), 2,
      ("UMASK=`umask`; umask 077; touch %2; umask $UMASK; eval %1 < /dev/null > %2 2>&1; echo \"###RESULT: $?\" >> %2"),
      0 }
};

fishProtocol::fishProtocol(const Q3CString &pool_socket, const Q3CString &app_socket)
  : SlaveBase("fish", pool_socket, app_socket), mimeBuffer(1024),
    mimeTypeSent(false)
{
    myDebug( << "fishProtocol::fishProtocol()" << endl);
    if (sshPath == NULL) {
        // disabled: currently not needed. Didn't work reliably.
        // isOpenSSH = !system("ssh -V 2>&1 | grep OpenSSH > /dev/null");
        sshPath = strdup(QFile::encodeName(KStandardDirs::findExe("ssh")));
    }
    if (suPath == NULL) {
        suPath = strdup(QFile::encodeName(KStandardDirs::findExe("su")));
    }
    childPid = 0;
    connectionPort = 0;
    isLoggedIn = false;
    writeReady = true;
    isRunning = false;
    firstLogin = true;
    errorCount = 0;
    rawRead = 0;
    rawWrite = -1;
    recvLen = -1;
    sendLen = -1;
    setMultipleAuthCaching( true );
    connectionAuth.keepPassword = true;
    connectionAuth.url.setProtocol("fish");
    outBufPos = -1;
    outBuf = NULL;
    outBufLen = 0;
    typeAtom.m_uds = UDS_FILE_TYPE;
    typeAtom.m_long = 0;
    mimeAtom.m_uds = UDS_MIME_TYPE;
    mimeAtom.m_long = 0;
    mimeAtom.m_str = QString::null;

    hasAppend = false;

    isStat = false; // FIXME: just a workaround for konq deficiencies
    redirectUser = ""; // FIXME: just a workaround for konq deficiencies
    redirectPass = ""; // FIXME: just a workaround for konq deficiencies
    fishCodeLen = strlen(fishCode);
}
/* ---------------------------------------------------------------------------------- */


fishProtocol::~fishProtocol()
{
    myDebug( << "fishProtocol::~fishProtocol()" << endl);
    shutdownConnection(true);
}

/* --------------------------------------------------------------------------- */

/**
Connects to a server and logs us in via SSH. Then starts FISH protocol.
*/
void fishProtocol::openConnection() {
    if (childPid) return;

    if (connectionHost.isEmpty())
    {
       error( KIO::ERR_UNKNOWN_HOST, QString::null );
       return;
    }

    infoMessage(i18n("Connecting..."));

    myDebug( << "connecting to: " << connectionUser << "@" << connectionHost << ":" << connectionPort << endl);
    sendCommand(FISH_FISH);
    sendCommand(FISH_VER);
    if (connectionStart()) {
        error(ERR_COULD_NOT_CONNECT,connectionHost);
        shutdownConnection();
        return;
    };
    myDebug( << "subprocess is running" << endl);
}

static int open_pty_pair(int fd[2])
{
#if defined(HAVE_TERMIOS_H) && defined(HAVE_GRANTPT) && !defined(HAVE_OPENPTY)
/** with kind regards to The GNU C Library
Reference Manual for Version 2.2.x of the GNU C Library */
    int master, slave;
    char *name;
    struct ::termios ti;
    memset(&ti,0,sizeof(ti));

    ti.c_cflag = CLOCAL|CREAD|CS8;
    ti.c_cc[VMIN] = 1;

#ifdef HAVE_GETPT
    master = getpt();
#else
    master = open("/dev/ptmx", O_RDWR);
#endif
    if (master < 0) return 0;

    if (grantpt(master) < 0 || unlockpt(master) < 0) goto close_master;

    name = ptsname(master);
    if (name == NULL) goto close_master;

    slave = open(name, O_RDWR);
    if (slave == -1) goto close_master;

#if (defined(HAVE_ISASTREAM) || defined(isastream)) && defined(I_PUSH)
    if (isastream(slave) &&
        (ioctl(slave, I_PUSH, "ptem") < 0 ||
         ioctl(slave, I_PUSH, "ldterm") < 0))
            goto close_slave;
#endif

    tcsetattr(slave, TCSANOW, &ti);
    fd[0] = master;
    fd[1] = slave;
    return 0;

#if (defined(HAVE_ISASTREAM) || defined(isastream)) && defined(I_PUSH)
close_slave:
#endif
    close(slave);

close_master:
    close(master);
    return -1;
#else
#ifdef HAVE_OPENPTY
    struct ::termios ti;
    memset(&ti,0,sizeof(ti));

    ti.c_cflag = CLOCAL|CREAD|CS8;
    ti.c_cc[VMIN] = 1;

    return openpty(fd,fd+1,NULL,&ti,NULL);
#else
#ifdef __GNUC__
#warning "No tty support available. Password dialog won't work."
#endif
    return socketpair(PF_UNIX,SOCK_STREAM,0,fd);
#endif
#endif
}
/**
creates the subprocess
*/
bool fishProtocol::connectionStart() {
    int fd[2];
    int rc, flags;
    thisFn = QString::null;

    rc = open_pty_pair(fd);
    if (rc == -1) {
        myDebug( << "socketpair failed, error: " << strerror(errno) << endl);
        return true;
    }

    if (!requestNetwork()) return true;
    myDebug( << "Exec: " << (local ? suPath : sshPath) << " Port: " << connectionPort << " User: " << connectionUser << endl);
    childPid = fork();
    if (childPid == -1) {
        myDebug( << "fork failed, error: " << strerror(errno) << endl);
        close(fd[0]);
        close(fd[1]);
        childPid = 0;
        dropNetwork();
        return true;
    }
    if (childPid == 0) {
        // taken from konsole, see TEPty.C for details
        // note: if we're running on socket pairs,
        // this will fail, but thats what we expect

        for (int sig = 1; sig < NSIG; sig++) signal(sig,SIG_DFL);

        struct rlimit rlp;
        getrlimit(RLIMIT_NOFILE, &rlp);
        for (int i = 0; i < (int)rlp.rlim_cur; i++)
            if (i != fd[1]) close(i);

        dup2(fd[1],0);
        dup2(fd[1],1);
        dup2(fd[1],2);
        if (fd[1] > 2) close(fd[1]);

        setsid();

#if defined(TIOCSCTTY)
        ioctl(0, TIOCSCTTY, 0);
#endif

        int pgrp = getpid();
#if defined( _AIX) || defined( __hpux)
        tcsetpgrp(0, pgrp);
#else
        ioctl(0, TIOCSPGRP, (char *)&pgrp);
#endif

        const char *dev = ttyname(0);
        setpgid(0,0);
        if (dev) close(open(dev, O_WRONLY, 0));
        setpgid(0,0);

        if (local) {
            execl(suPath, "su", "-", connectionUser.latin1(), "-c", "cd ~;echo FISH:;exec /bin/sh -c \"if env true 2>/dev/null; then env PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; else PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; fi\"", (void *)0);
        } else {
            #define common_args "-l", connectionUser.latin1(), "-x", "-e", "none", \
                "-q", connectionHost.latin1(), \
                "echo FISH:;exec /bin/sh -c \"if env true 2>/dev/null; then env PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; else PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; fi\"", (void *)0
            // disabled: leave compression up to the client.
            // (isOpenSSH?"-C":"+C"),

            if (connectionPort)
                execl(sshPath, "ssh", "-p", QString::number(connectionPort).latin1(), common_args);
            else
                execl(sshPath, "ssh", common_args);
            #undef common_args
        }
        myDebug( << "could not exec! " << strerror(errno) << endl);
        ::exit(-1);
    }
    close(fd[1]);
    rc = fcntl(fd[0],F_GETFL,&flags);
    rc = fcntl(fd[0],F_SETFL,flags|O_NONBLOCK);
    childFd = fd[0];

    fd_set rfds, wfds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    char buf[32768];
    int offset = 0;
    while (!isLoggedIn) {
        FD_SET(childFd,&rfds);
        FD_ZERO(&wfds);
        if (outBufPos >= 0) FD_SET(childFd,&wfds);
        rc = select(childFd+1, &rfds, &wfds, NULL, NULL);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            myDebug( << "select failed, rc: " << rc << ", error: " << strerror(errno) << endl);
            return true;
        }
        if (FD_ISSET(childFd,&wfds) && outBufPos >= 0) {
            if (outBuf > 0) rc = write(childFd,outBuf+outBufPos,outBufLen-outBufPos);
            else rc = 0;
            if (rc >= 0) outBufPos += rc;
            else {
                if (errno == EINTR)
                    continue;
                myDebug( << "write failed, rc: " << rc << ", error: " << strerror(errno) << endl);
                outBufPos = -1;
                //return true;
            }
            if (outBufPos >= outBufLen) {
                outBufPos = -1;
                outBuf = NULL;
                outBufLen = 0;
            }
        }
        if (FD_ISSET(childFd,&rfds)) {
            rc = read(childFd,buf+offset,32768-offset);
            if (rc > 0) {
                int noff = establishConnection(buf,rc+offset);
                if (noff < 0) return false;
                if (noff > 0) memmove(buf,buf+offset+rc-noff,noff);
                offset = noff;
            } else {
                if (errno == EINTR)
                    continue;
                myDebug( << "read failed, rc: " << rc << ", error: " << strerror(errno) << endl);
                return true;
            }
        }
    }
    return false;
}

/**
writes one chunk of data to stdin of child process
*/
void fishProtocol::writeChild(const char *buf, int len) {
    if (outBufPos >= 0 && outBuf) {
        QString debug;
        debug.setLatin1(outBuf,outBufLen);
        if (len > 0) myDebug( << "write request while old one is pending, throwing away input (" << outBufLen << "," << outBufPos << "," << debug.left(10) << "...)" << endl);
        return;
    }
    outBuf = buf;
    outBufPos = 0;
    outBufLen = len;
}

/**
manages initial communication setup including password queries
*/
int fishProtocol::establishConnection(char *buffer, int len) {
    QString buf;
    buf.setLatin1(buffer,len);
    int pos=0;
    // Strip trailing whitespace
    while (buf.length() && (buf[buf.length()-1] == ' '))
       buf.truncate(buf.length()-1);

    myDebug( << "establishing: got " << buf << endl);
    while (childPid && ((pos = buf.find('\n')) >= 0 ||
            buf.endsWith(":") || buf.endsWith("?"))) {
        pos++;
        QString str = buf.left(pos);
        buf = buf.mid(pos);
        if (str == "\n")
            continue;
        if (str == "FISH:\n") {
            thisFn = QString::null;
            infoMessage(i18n("Initiating protocol..."));
            if (!connectionAuth.password.isEmpty()) {
                connectionAuth.password = connectionAuth.password.left(connectionAuth.password.length()-1);
                cacheAuthentication(connectionAuth);
            }
            isLoggedIn = true;
            return 0;
        } else if (!str.isEmpty()) {
            thisFn += str;
        } else if (buf.endsWith(":")) {
            if (!redirectUser.isEmpty() && connectionUser != redirectUser) {
                KURL dest = url;
                dest.setUser(redirectUser);
                dest.setPass(redirectPass);
                redirection(dest);
                commandList.clear();
                commandCodes.clear();
                finished();
                redirectUser = "";
                redirectPass = "";
                return -1;
            } else if (!connectionPassword.isEmpty()) {
                myDebug( << "sending cpass" << endl);
                connectionAuth.password = connectionPassword+"\n";
                connectionPassword = QString::null;
                // su does not like receiving a password directly after sending
                // the password prompt so we wait a while.
                if (local)
                    sleep(1);
                writeChild(connectionAuth.password.latin1(),connectionAuth.password.length());
            } else {
                myDebug( << "sending mpass" << endl);
                connectionAuth.prompt = thisFn+buf;
                if (local)
                    connectionAuth.caption = i18n("Local Login");
                else
                    connectionAuth.caption = i18n("SSH Authorization");
                if ((!firstLogin || !checkCachedAuthentication(connectionAuth))) {
                    connectionAuth.password = QString::null; // don't prefill
                    if ( !openPassDlg(connectionAuth)) {
                        error(ERR_USER_CANCELED,connectionHost);
                        shutdownConnection();
                        return -1;
                    }
                }
                firstLogin = false;
                connectionAuth.password += "\n";
                if (connectionAuth.username != connectionUser) {
                    KURL dest = url;
                    dest.setUser(connectionAuth.username);
                    dest.setPass(connectionAuth.password);
                    redirection(dest);
                    if (isStat) { // FIXME: just a workaround for konq deficiencies
                        redirectUser = connectionAuth.username;
                        redirectPass = connectionAuth.password;
                    }
                    commandList.clear();
                    commandCodes.clear();
                    finished();
                    return -1;
                }
                myDebug( << "sending pass" << endl);
                if (local)
                    sleep(1);
                writeChild(connectionAuth.password.latin1(),connectionAuth.password.length());
            }
            thisFn = QString::null;
            return 0;
        } else if (buf.endsWith("?")) {
            int rc = messageBox(QuestionYesNo,thisFn+buf);
            if (rc == KMessageBox::Yes) {
                writeChild("yes\n",4);
            } else {
                writeChild("no\n",3);
            }
            thisFn = QString::null;
            return 0;
        } else {
            myDebug( << "unmatched case in initial handling! shouldn't happen!" << endl);
          }
    }
    return buf.length();
}
/**
sets connection information for subsequent commands
*/
void fishProtocol::setHost(const QString & host, int port, const QString & u, const QString & pass){
    QString user(u);

    local = (host == "localhost" && port == 0);
    if (port <= 0) port = 0;
    if (user.isEmpty()) user = getenv("LOGNAME");

    if (host == connectionHost && port == connectionPort && user == connectionUser)
        return;
    myDebug( << "setHost " << u << "@" << host << endl);

    if (childPid) shutdownConnection();

    connectionHost = host;
    connectionAuth.url.setHost(host);

    connectionUser = user;
    connectionAuth.username = user;
    connectionAuth.url.setUser(user);

    connectionPort = port;
    connectionPassword = pass;
    firstLogin = true;
}

/**
Forced close of the connection

This function gets called from the application side of the universe,
it shouldn't send any response.
 */
void fishProtocol::closeConnection(){
    myDebug( << "closeConnection()" << endl);
    shutdownConnection(true);
}

/**
Closes the connection
 */
void fishProtocol::shutdownConnection(bool forced){
    if (childPid) {
        kill(childPid,SIGTERM); // We may not have permission...
        childPid = 0;
        close(childFd); // ...in which case this should do the trick
        childFd = -1;
        if (!forced)
        {
           dropNetwork();
           infoMessage(i18n("Disconnected."));
        }
    }
    outBufPos = -1;
    outBuf = NULL;
    outBufLen = 0;
    qlist.clear();
    commandList.clear();
    commandCodes.clear();
    isLoggedIn = false;
    writeReady = true;
    isRunning = false;
    rawRead = 0;
    rawWrite = -1;
    recvLen = -1;
    sendLen = -1;
}
/**
builds each FISH request and sets the error counter
*/
bool fishProtocol::sendCommand(fish_command_type cmd, ...) {
    const fish_info &info = fishInfo[cmd];
    myDebug( << "queueing: cmd="<< cmd << "['" << info.command << "'](" << info.params <<"), alt=['" << info.alt << "'], lines=" << info.lines << endl);

    va_list list;
    va_start(list, cmd);
    QString realCmd = info.command;
    QString realAlt = info.alt;
    static QRegExp rx("[][\\\\\n $`#!()*?{}~&<>;'\"%^@|\t]");
    for (int i = 0; i < info.params; i++) {
        QString arg(va_arg(list, const char *));
        int pos = -2;
        while ((pos = rx.search(arg,pos+2)) >= 0) {
            arg.replace(pos,0,QString("\\"));
        }
        //myDebug( << "arg " << i << ": " << arg << endl);
        realCmd.append(" ").append(arg);
        realAlt.replace(QRegExp("%"+QString::number(i+1)),arg);
    }
    QString s("#");
    s.append(realCmd).append("\n ").append(realAlt).append(" 2>&1;echo '### 000'\n");
    if (realCmd == "FISH")
        s.prepend(" ");
    commandList.append(s);
    commandCodes.append(cmd);
    return true;
}

/**
checks response string for result code, converting 000 and 001 appropriately
*/
int fishProtocol::handleResponse(const QString &str){
    myDebug( << "handling: " << str << endl);
    if (str.startsWith("### ")) {
        bool isOk = false;
        int result = str.mid(4,3).toInt(&isOk);
        if (!isOk) result = 500;
        if (result == 0) result = (errorCount != 0?500:200);
        if (result == 1) result = (errorCount != 0?500:100);
        myDebug( << "result: " << result << ", errorCount: " << errorCount << endl);
        return result;
    } else {
        errorCount++;
        return 0;
    }
}

int fishProtocol::makeTimeFromLs(const QString &monthStr, const QString &dayStr, const QString &timeyearStr)
{
    QDateTime dt(QDate::currentDate(Qt::UTC));
    int year = dt.date().year();
    int month = dt.date().month();
    int currentMonth = month;
    int day = dayStr.toInt();

    static const char * const monthNames[12] = {
          "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    for (int i=0; i < 12; i++) if (monthStr.startsWith(monthNames[i])) {
        month = i+1;
        break;
    }

    int pos = timeyearStr.find(':');
    if (timeyearStr.length() == 4 && pos == -1) {
        year = timeyearStr.toInt();
    } else if (pos == -1) {
        return 0;
    } else {
        if (month > currentMonth + 1) year--;
        dt.time().setHMS(timeyearStr.left(pos).toInt(),timeyearStr.mid(pos+1).toInt(),0);
    }
    dt.date().setYMD(year,month,day);

    return dt.toTime_t();
}

/**
parses response from server and acts accordingly
*/
void fishProtocol::manageConnection(const QString &l) {
    QString line(l);
    int rc = handleResponse(line);
    UDSAtom atom;
    QDateTime dt;
    KIO::filesize_t pos, pos2, pos3;
    bool isOk = false;
    if (!rc) {
        switch (fishCommand) {
        case FISH_VER:
            if (line.startsWith("VER 0.0.3")) {
                line.append(" ");
                hasAppend = line.contains(" append ");
            } else {
                error(ERR_UNSUPPORTED_PROTOCOL,line);
                shutdownConnection();
            }
            break;
        case FISH_PWD:
            url.setPath(line);
            redirection(url);
            break;
        case FISH_LIST:
            myDebug( << "listReason: " << listReason << endl);
            /* Fall through */
        case FISH_STAT:
            if (line.length() > 0) {
                switch (line[0].cell()) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    pos = line.toULongLong(&isOk);
                    if (pos > 0 && isOk) errorCount--;
                    if ((fishCommand == FISH_LIST) && (listReason == LIST))
                        totalSize(pos);
                    break;

                case 'P':
                    errorCount--;
                    if (line[1] == 'd') {
                        mimeAtom.m_str = "inode/directory";
                        typeAtom.m_long = S_IFDIR;
                    } else {
                        if (line[1] == '-') {
                            typeAtom.m_long = S_IFREG;
                        } else if (line[1] == 'l') {
                            typeAtom.m_long = S_IFLNK;
                        } else if (line[1] == 'c') {
                            typeAtom.m_long = S_IFCHR;
                        } else if (line[1] == 'b') {
                            typeAtom.m_long = S_IFBLK;
                        } else if (line[1] == 's') {
                            typeAtom.m_long = S_IFSOCK;
                        } else if (line[1] == 'p') {
                            typeAtom.m_long = S_IFIFO;
                        } else {
                            myDebug( << "unknown file type: " << line[1].cell() << endl);
                            errorCount++;
                            break;
                        }
                    }
                    //myDebug( << "file type: " << atom.m_long << endl);
                    //udsEntry.append(atom);

                    atom.m_uds = UDS_ACCESS;
                    atom.m_long = 0;
                    if (line[2] == 'r') atom.m_long |= S_IRUSR;
                    if (line[3] == 'w') atom.m_long |= S_IWUSR;
                    if (line[4] == 'x' || line[4] == 's') atom.m_long |= S_IXUSR;
                    if (line[4] == 'S' || line[4] == 's') atom.m_long |= S_ISUID;
                    if (line[5] == 'r') atom.m_long |= S_IRGRP;
                    if (line[6] == 'w') atom.m_long |= S_IWGRP;
                    if (line[7] == 'x' || line[7] == 's') atom.m_long |= S_IXGRP;
                    if (line[7] == 'S' || line[7] == 's') atom.m_long |= S_ISGID;
                    if (line[8] == 'r') atom.m_long |= S_IROTH;
                    if (line[9] == 'w') atom.m_long |= S_IWOTH;
                    if (line[10] == 'x' || line[10] == 't') atom.m_long |= S_IXOTH;
                    if (line[10] == 'T' || line[10] == 't') atom.m_long |= S_ISVTX;
                    udsEntry.append(atom);

                    atom.m_uds = UDS_USER;
                    atom.m_long = 0;
                    pos = line.find('.',12);
                    if (pos < 0) {
                        errorCount++;
                        break;
                    }
                    atom.m_str = line.mid(12,pos-12);
                    udsEntry.append(atom);

                    atom.m_uds = UDS_GROUP;
                    atom.m_long = 0;
                    atom.m_str = line.mid(pos+1);
                    udsEntry.append(atom);
                    break;

                case 'd':
                    atom.m_uds = UDS_MODIFICATION_TIME;
                    pos = line.find(' ');
                    pos2 = line.find(' ',pos+1);
                    if (pos < 0 || pos2 < 0) break;
                    errorCount--;
                    atom.m_long = makeTimeFromLs(line.mid(1,pos-1), line.mid(pos+1,pos2-pos), line.mid(pos2+1));
                    udsEntry.append(atom);
                    break;

                case 'D':
                    atom.m_uds = UDS_MODIFICATION_TIME;
                    pos = line.find(' ');
                    pos2 = line.find(' ',pos+1);
                    pos3 = line.find(' ',pos2+1);
                    if (pos < 0 || pos2 < 0 || pos3 < 0) break;
                    dt.setDate(QDate(line.mid(1,pos-1).toInt(),line.mid(pos+1,pos2-pos-1).toInt(),line.mid(pos2+1,pos3-pos2-1).toInt()));
                    pos = pos3;
                    pos2 = line.find(' ',pos+1);
                    pos3 = line.find(' ',pos2+1);
                    if (pos < 0 || pos2 < 0 || pos3 < 0) break;
                    dt.setTime(QTime(line.mid(pos+1,pos2-pos-1).toInt(),line.mid(pos2+1,pos3-pos2-1).toInt(),line.mid(pos3+1).toInt()));
                    errorCount--;
                    atom.m_long = dt.toTime_t();
                    udsEntry.append(atom);
                    break;

                case 'S':
                    atom.m_uds = UDS_SIZE;
                    atom.m_long = line.mid(1).toULongLong(&isOk);
                    if (!isOk) break;
                    errorCount--;
                    udsEntry.append(atom);
                    break;

                case 'E':
                    errorCount--;
                    break;

                case ':':
                    atom.m_uds = UDS_NAME;
                    atom.m_long = 0;
                    pos = line.findRev('/');
                    atom.m_str = thisFn = line.mid(pos < 0?1:pos+1);
                    if (fishCommand == FISH_LIST)
                        udsEntry.append(atom);
                    // By default, the mimetype comes from the extension
                    // We'll use the file(1) result only as fallback [like the rest of KDE does]
                    {
                      KMimeType::Ptr mime = KMimeType::findByURL( KURL("fish://host/" + thisFn) );
                      if ( mime->name() != KMimeType::defaultMimeType() )
                          mimeAtom.m_str = mime->name();
                    }
                    errorCount--;
                    break;

                case 'M':
                    // This is getting ugly. file(1) makes some uneducated
                    // guesses, so we must try to ignore them (#51274)
                    if (mimeAtom.m_str.isEmpty() && line.right(8) != "/unknown" &&
                            (thisFn.find('.') < 0 || (line.left(8) != "Mtext/x-"
                                                  && line != "Mtext/plain"))) {
                        mimeAtom.m_str = line.mid(1);
                        if ( mimeAtom.m_str == "inode/directory" ) // a symlink to a dir is a dir
                          typeAtom.m_long = S_IFDIR;
                    }
                    errorCount--;
                    break;

                case 'L':
                    atom.m_uds = UDS_LINK_DEST;
                    atom.m_long = 0;
                    atom.m_str = line.mid(1);
                    udsEntry.append(atom);
                    if (!typeAtom.m_long) typeAtom.m_long = S_IFLNK;
                    errorCount--;
                    break;
                }
            } else {
                if (!mimeAtom.m_str.isNull())
                    udsEntry.append(mimeAtom);
                mimeAtom.m_str = QString::null;

                udsEntry.append(typeAtom);
                typeAtom.m_long = 0;

                if (fishCommand == FISH_STAT)
                    udsStatEntry = udsEntry;
                else if (listReason == LIST) {
                    listEntry(udsEntry, false); //1
                } else if (listReason == CHECK) checkExist = true; //0
                errorCount--;
                udsEntry.clear();
            }
            break;

        case FISH_RETR:
            if (line.length() == 0) {
                error(ERR_IS_DIRECTORY,url.prettyURL());
                recvLen = 0;
                break;
            }
            recvLen = line.toInt(&isOk);
            if (!isOk) {
                error(ERR_COULD_NOT_READ,url.prettyURL());
                shutdownConnection();
                break;
            }
            break;
        default : break;
        }

    } else if (rc == 100) {
        switch (fishCommand) {
        case FISH_FISH:
            writeChild(fishCode, fishCodeLen);
            break;
        case FISH_READ:
            recvLen = 1024;
            /* fall through */
        case FISH_RETR:
            myDebug( << "reading " << recvLen << endl);
            if (recvLen == -1) {
                error(ERR_COULD_NOT_READ,url.prettyURL());
                shutdownConnection();
            } else {
                rawRead = recvLen;
                t_start = t_last = time(NULL);
                dataRead = 0;
                mimeTypeSent = false;
                if (recvLen == 0)
                {
                    mimeType("application/x-zerosize");
                    mimeTypeSent = true;
                }
            }
            break;
        case FISH_STOR:
        case FISH_WRITE:
        case FISH_APPEND:
            rawWrite = sendLen;
            //myDebug( << "sending " << sendLen << endl);
            writeChild(NULL,0);
            break;
        default : break;
        }
    } else if (rc/100 != 2) {
        switch (fishCommand) {
        case FISH_STOR:
        case FISH_WRITE:
        case FISH_APPEND:
            error(ERR_COULD_NOT_WRITE,url.prettyURL());
            shutdownConnection();
            break;
        case FISH_RETR:
            error(ERR_COULD_NOT_READ,url.prettyURL());
            shutdownConnection();
            break;
        case FISH_READ:
            if ( rc == 501 )
            {
               mimeType("inode/directory");
               mimeTypeSent = true;
               recvLen = 0;
               finished();
            }
            else
            {
               error(ERR_COULD_NOT_READ,url.prettyURL());
               shutdownConnection();
            }
            break;
        case FISH_FISH:
        case FISH_VER:
            error(ERR_SLAVE_DEFINED,line);
            shutdownConnection();
            break;
        case FISH_PWD:
        case FISH_CWD:
            error(ERR_CANNOT_ENTER_DIRECTORY,url.prettyURL());
            break;
        case FISH_LIST:
            myDebug( << "list error. reason: " << listReason << endl);
            if (listReason == LIST) error(ERR_CANNOT_ENTER_DIRECTORY,url.prettyURL());
            else if (listReason == CHECK) {
                checkExist = false;
                finished();
            }
            break;
        case FISH_STAT:
            error(ERR_DOES_NOT_EXIST,url.prettyURL());
            udsStatEntry.clear();
            break;
        case FISH_CHMOD:
            error(ERR_CANNOT_CHMOD,url.prettyURL());
            break;
        case FISH_CHOWN:
        case FISH_CHGRP:
            error(ERR_ACCESS_DENIED,url.prettyURL());
            break;
        case FISH_MKD:
            if ( rc == 501 )
                error(ERR_DIR_ALREADY_EXIST,url.prettyURL());
            else
                error(ERR_COULD_NOT_MKDIR,url.prettyURL());
            break;
        case FISH_RMD:
            error(ERR_COULD_NOT_RMDIR,url.prettyURL());
            break;
        case FISH_DELE:
            error(ERR_CANNOT_DELETE,url.prettyURL());
            break;
        case FISH_RENAME:
            error(ERR_CANNOT_RENAME,url.prettyURL());
            break;
        case FISH_COPY:
        case FISH_LINK:
        case FISH_SYMLINK:
            error(ERR_COULD_NOT_WRITE,url.prettyURL());
            break;
        default : break;
        }
    } else {
        if (fishCommand == FISH_STOR) fishCommand = (hasAppend?FISH_APPEND:FISH_WRITE);
        if (fishCommand == FISH_FISH) {
            connected();
        } else if (fishCommand == FISH_LIST) {
            if (listReason == LIST) {
                listEntry(UDSEntry(),true);
            } else if (listReason == CHECK) {
                if (!checkOverwrite && checkExist)
                {
                    error(ERR_FILE_ALREADY_EXIST,url.prettyURL());
                    return; // Don't call finished!
                }
            }
        } else if (fishCommand == FISH_STAT) {
            UDSAtom atom;

            atom.m_uds = KIO::UDS_NAME;
            atom.m_str = url.fileName();
            udsStatEntry.append( atom );
            statEntry(udsStatEntry);
        } else if (fishCommand == FISH_APPEND) {
            dataReq();
            if (readData(rawData) > 0) sendCommand(FISH_APPEND,E(QString::number(rawData.size())),E(url.path()));
            else if (!checkExist && putPerm > -1) sendCommand(FISH_CHMOD,E(QString::number(putPerm,8)),E(url.path()));
            sendLen = rawData.size();
        } else if (fishCommand == FISH_WRITE) {
            dataReq();
            if (readData(rawData) > 0) sendCommand(FISH_WRITE,E(QString::number(putPos)),E(QString::number(rawData.size())),E(url.path()));
            else if (!checkExist && putPerm > -1) sendCommand(FISH_CHMOD,E(QString::number(putPerm,8)),E(url.path()));
            putPos += rawData.size();
            sendLen = rawData.size();
        } else if (fishCommand == FISH_RETR) {
            data(QByteArray());
        }
        finished();
    }
}

void fishProtocol::writeStdin(const QString &line)
{
    qlist.append(line);

    if (writeReady) {
        writeReady = false;
        //myDebug( << "Writing: " << qlist.first().mid(0,qlist.first().find('\n')) << endl);
        myDebug( << "Writing: " << qlist.first() << endl);
        myDebug( << "---------" << endl);
        writeChild((const char *)qlist.first().latin1(), qlist.first().length());
    }
}

void fishProtocol::sent()
{
    if (rawWrite > 0) {
        myDebug( << "writing raw: " << rawData.size() << "/" << rawWrite << endl);
        writeChild(rawData.data(),((int)rawWrite > rawData.size()?rawData.size():rawWrite));
        rawWrite -= rawData.size();
        if (rawWrite > 0) {
            dataReq();
            if (readData(rawData) <= 0) {
                shutdownConnection();
            }
        }
        return;
    } else if (rawWrite == 0) {
        // workaround: some dd's insist in reading multiples of
        // 8 bytes, swallowing up to seven bytes. Sending
        // newlines is safe even when a sane dd is used
        writeChild("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",15);
        rawWrite = -1;
        return;
    }
    if (qlist.count() > 0) qlist.remove(qlist.begin());
    if (qlist.count() == 0) {
        writeReady = true;
    } else {
        //myDebug( << "Writing: " << qlist.first().mid(0,qlist.first().find('\n')) << endl);
        myDebug( << "Writing: " << qlist.first() << endl);
        myDebug( << "---------" << endl);
        writeChild((const char *)qlist.first().latin1(),qlist.first().length());
    }
}

int fishProtocol::received(const char *buffer, int buflen)
{
    int pos = 0;
    do {
        if (buflen <= 0) break;

        if (rawRead > 0) {
            myDebug( << "processedSize " << dataRead << ", len " << buflen << "/" << rawRead << endl);
            int dataSize = (rawRead > buflen?buflen:rawRead);
            if (!mimeTypeSent)
            {
                int mimeSize = QMIN(dataSize, (int)mimeBuffer.size()-dataRead);
                memcpy(mimeBuffer.data()+dataRead,buffer,mimeSize);
                dataRead += mimeSize;
                rawRead -= mimeSize;
                buffer += mimeSize;
                buflen -= mimeSize;
                if (rawRead == 0) // End of data
                    mimeBuffer.resize(dataRead);
                if (dataRead < (int)mimeBuffer.size())
                {
                    myDebug( << "wait for more" << endl);
                    break;
                }
                sendmimeType(KMimeMagic::self()->findBufferFileType(mimeBuffer,url.path())->mimeType());
                mimeTypeSent = true;
                if (fishCommand != FISH_READ) {
                    totalSize(dataRead + rawRead);
                    data(mimeBuffer);
                    processedSize(dataRead);
                }
                mimeBuffer.resize(1024);
                pos = 0;
                continue; // Process rest of buffer/buflen
            }

            QByteArray bdata;
            bdata.duplicate(buffer,dataSize);
            data(bdata);

            dataRead += dataSize;
            rawRead -= dataSize;
            time_t t = time(NULL);
            if (t-t_last >= 1) {
                processedSize(dataRead);
                //speed(dataRead/(t-t_start));
                t_last = t;
            }
            if (rawRead <= 0) {
                buffer += dataSize;
                buflen -= dataSize;
            } else {
                return 0;
            }
        }

        if (buflen <= 0) break;

        pos = 0;
        // Find newline
        while((pos < buflen) && (buffer[pos] != '\n'))
            ++pos;

        if (pos < buflen)
        {
           QString s = remoteEncoding()->decode(Q3CString(buffer,pos+1));

           buffer += pos+1;
           buflen -= pos+1;

           manageConnection(s);

           pos = 0;
           // Find next newline
           while((pos < buflen) && (buffer[pos] != '\n'))
               ++pos;
        }
    } while (childPid && buflen && (rawRead > 0 || pos < buflen));
    return buflen;
}
/** get a file */
void fishProtocol::get(const KURL& u){
    myDebug( << "@@@@@@@@@ get " << u << endl);
    setHost(u.host(),u.port(),u.user(),u.pass());
    url = u;
    openConnection();
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        recvLen = -1;
        sendCommand(FISH_RETR,E(url.path()));
    }
    run();
}

/** put a file */
void fishProtocol::put(const KURL& u, int permissions, bool overwrite, bool /*resume*/){
    myDebug( << "@@@@@@@@@ put " << u << " " << permissions << " " << overwrite << " " /* << resume */ << endl);
    setHost(u.host(),u.port(),u.user(),u.pass());
    url = u;
    openConnection();
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        putPerm = permissions;
        checkOverwrite = overwrite;
        checkExist = false;
        putPos = 0;
        listReason = CHECK;
        sendCommand(FISH_LIST,E(url.path()));
        sendCommand(FISH_STOR,"0",E(url.path()));
    }
    run();
}
/** executes next command in sequence or calls finished() if all is done */
void fishProtocol::finished() {
    if (commandList.count() > 0) {
        fishCommand = (fish_command_type)commandCodes.first();
        errorCount = -fishInfo[fishCommand].lines;
        rawRead = 0;
        rawWrite = -1;
        udsEntry.clear();
        udsStatEntry.clear();
        writeStdin(commandList.first());
        //if (fishCommand != FISH_APPEND && fishCommand != FISH_WRITE) infoMessage("Sending "+(commandList.first().mid(1,commandList.first().find("\n")-1))+"...");
        commandList.remove(commandList.begin());
        commandCodes.remove(commandCodes.begin());
    } else {
        myDebug( << "_______ emitting finished()" << endl);
        SlaveBase::finished();
        isRunning = false;
    }
}
/** aborts command sequence and calls error() */
void fishProtocol::error(int type, const QString &detail) {
    commandList.clear();
    commandCodes.clear();
    myDebug( << "ERROR: " << type << " - " << detail << endl);
    SlaveBase::error(type,detail);
    isRunning = false;
}
/** executes a chain of commands */
void fishProtocol::run() {
    if (!isRunning) {
        int rc;
        isRunning = true;
        finished();
        fd_set rfds, wfds;
        FD_ZERO(&rfds);
        char buf[32768];
        int offset = 0;
        while (isRunning) {
            FD_SET(childFd,&rfds);
            FD_ZERO(&wfds);
            if (outBufPos >= 0) FD_SET(childFd,&wfds);
            rc = select(childFd+1, &rfds, &wfds, NULL, NULL);
            if (rc < 0) {
                if (errno == EINTR)
                    continue;
                myDebug( << "select failed, rc: " << rc << ", error: " << strerror(errno) << endl);
                error(ERR_CONNECTION_BROKEN,connectionHost);
                shutdownConnection();
                return;
            }
            if (FD_ISSET(childFd,&wfds) && outBufPos >= 0) {
                QString debug;
                debug.setLatin1(outBuf+outBufPos,outBufLen-outBufPos);
                myDebug( << "now writing " << (outBufLen-outBufPos) << " " << debug.left(40) << "..." << endl);
                if (outBufLen-outBufPos > 0) rc = write(childFd,outBuf+outBufPos,outBufLen-outBufPos);
                else rc = 0;
                if (rc >= 0) outBufPos += rc;
                else {
                    if (errno == EINTR)
                        continue;
                    myDebug( << "write failed, rc: " << rc << ", error: " << strerror(errno) << endl);
                    error(ERR_CONNECTION_BROKEN,connectionHost);
                    shutdownConnection();
                    return;
                }
                if (outBufPos >= outBufLen) {
                    outBufPos = -1;
                    outBuf = NULL;
                    sent();
                }
            }
            if (FD_ISSET(childFd,&rfds)) {
                rc = read(childFd,buf+offset,32768-offset);
                //myDebug( << "read " << rc << " bytes" << endl);
                if (rc > 0) {
                    int noff = received(buf,rc+offset);
                    if (noff > 0) memmove(buf,buf+offset+rc-noff,noff);
                    //myDebug( << "left " << noff << " bytes: " << QString::fromLatin1(buf,offset) << endl);
                    offset = noff;
                } else {
                    if (errno == EINTR)
                        continue;
                    myDebug( << "read failed, rc: " << rc << ", error: " << strerror(errno) << endl);
                    error(ERR_CONNECTION_BROKEN,connectionHost);
                    shutdownConnection();
                    return;
                }
            }
            if (wasKilled())
                return;
        }
    }
}
/** stat a file */
void fishProtocol::stat(const KURL& u){
    myDebug( << "@@@@@@@@@ stat " << u << endl);
    setHost(u.host(),u.port(),u.user(),u.pass());
    url = u;
    isStat = true; // FIXME: just a workaround for konq deficiencies
    openConnection();
    isStat = false; // FIXME: just a workaround for konq deficiencies
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        sendCommand(FISH_STAT,E(url.path(-1)));
    }
    run();
}
/** find mimetype for a file */
void fishProtocol::mimetype(const KURL& u){
    myDebug( << "@@@@@@@@@ mimetype " << u << endl);
    setHost(u.host(),u.port(),u.user(),u.pass());
    url = u;
    openConnection();
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        recvLen = 1024;
        sendCommand(FISH_READ,"0","1024",E(url.path()));
    }
    run();
}
/** list a directory */
void fishProtocol::listDir(const KURL& u){
    myDebug( << "@@@@@@@@@ listDir " << u << endl);
    setHost(u.host(),u.port(),u.user(),u.pass());
    url = u;
    openConnection();
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        listReason = LIST;
        sendCommand(FISH_LIST,E(url.path()));
    }
    run();
}
/** create a directory */
void fishProtocol::mkdir(const KURL& u, int permissions) {
    myDebug( << "@@@@@@@@@ mkdir " << u << " " << permissions << endl);
    setHost(u.host(),u.port(),u.user(),u.pass());
    url = u;
    openConnection();
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        sendCommand(FISH_MKD,E(url.path()));
        if (permissions > -1) sendCommand(FISH_CHMOD,E(QString::number(permissions,8)),E(url.path()));
    }
    run();
}
/** rename a file */
void fishProtocol::rename(const KURL& s, const KURL& d, bool overwrite) {
    myDebug( << "@@@@@@@@@ rename " << s << " " << d << " " << overwrite << endl);
    if (s.host() != d.host() || s.port() != d.port() || s.user() != d.user()) {
        error(ERR_UNSUPPORTED_ACTION,s.prettyURL());
        return;
    }
    setHost(s.host(),s.port(),s.user(),s.pass());
    url = d;
    openConnection();
    if (!isLoggedIn) return;
    KURL src = s;
    url.cleanPath();
    src.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        if (!overwrite) {
            listReason = CHECK;
            checkOverwrite = false;
            sendCommand(FISH_LIST,E(url.path()));
        }
        sendCommand(FISH_RENAME,E(src.path()),E(url.path()));
    }
    run();
}
/** create a symlink */
void fishProtocol::symlink(const QString& target, const KURL& u, bool overwrite) {
    myDebug( << "@@@@@@@@@ symlink " << target << " " << u << " " << overwrite << endl);
    setHost(u.host(),u.port(),u.user(),u.pass());
    url = u;
    openConnection();
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        if (!overwrite) {
            listReason = CHECK;
            checkOverwrite = false;
            sendCommand(FISH_LIST,E(url.path()));
        }
        sendCommand(FISH_SYMLINK,E(target),E(url.path()));
    }
    run();
}
/** change file permissions */
void fishProtocol::chmod(const KURL& u, int permissions){
    myDebug( << "@@@@@@@@@ chmod " << u << " " << permissions << endl);
    setHost(u.host(),u.port(),u.user(),u.pass());
    url = u;
    openConnection();
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        if (permissions > -1) sendCommand(FISH_CHMOD,E(QString::number(permissions,8)),E(url.path()));
    }
    run();
}
/** copies a file */
void fishProtocol::copy(const KURL &s, const KURL &d, int permissions, bool overwrite) {
    myDebug( << "@@@@@@@@@ copy " << s << " " << d << " " << permissions << " " << overwrite << endl);
    if (s.host() != d.host() || s.port() != d.port() || s.user() != d.user()) {
        error(ERR_UNSUPPORTED_ACTION,s.prettyURL());
        return;
    }
    //myDebug( << s << endl << d << endl);
    setHost(s.host(),s.port(),s.user(),s.pass());
    url = d;
    openConnection();
    if (!isLoggedIn) return;
    KURL src = s;
    url.cleanPath();
    src.cleanPath();
    if (!src.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        if (!overwrite) {
            listReason = CHECK;
            checkOverwrite = false;
            sendCommand(FISH_LIST,E(url.path()));
        }
        sendCommand(FISH_COPY,E(src.path()),E(url.path()));
        if (permissions > -1) sendCommand(FISH_CHMOD,E(QString::number(permissions,8)),E(url.path()));
    }
    run();
}
/** removes a file or directory */
void fishProtocol::del(const KURL &u, bool isFile){
    myDebug( << "@@@@@@@@@ del " << u << " " << isFile << endl);
    setHost(u.host(),u.port(),u.user(),u.pass());
    url = u;
    openConnection();
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        sendCommand((isFile?FISH_DELE:FISH_RMD),E(url.path()));
    }
    run();
}
/** special like background execute */
void fishProtocol::special( const QByteArray &data ){
    int tmp;

    QDataStream stream(data);

    stream >> tmp;
    switch (tmp) {
        case FISH_EXEC_CMD: // SSH EXEC
        {
            KURL u;
            QString command;
            QString tempfile;
            stream >> u;
            stream >> command;
            myDebug( << "@@@@@@@@@ exec " << u << " " << command << endl);
            setHost(u.host(),u.port(),u.user(),u.pass());
            url = u;
            openConnection();
            if (!isLoggedIn) return;
            sendCommand(FISH_EXEC,E(command),E(url.path()));
            run();
            break;
        }
        default:
            // Some command we don't understand.
            error(ERR_UNSUPPORTED_ACTION,QString().setNum(tmp));
            break;
    }
}
/** report status */
void fishProtocol::slave_status() {
    myDebug( << "@@@@@@@@@ slave_status" << endl);
    if (childPid > 0)
        slaveStatus(connectionHost,isLoggedIn);
    else
        slaveStatus(QString::null,false);
}
