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

#include "fish.h"

#include <config-runtime.h>
#include "config-fish.h"
#include <QFile>
#include <QDateTime>
#include <QRegExp>
#include <QCoreApplication>

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
#include <kcomponentdata.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kremoteencoding.h>
#include <kurl.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <kmimetype.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/resource.h>
#include <kdefakes.h>

#include "fishcode.h"

#ifndef NDEBUG
#define myDebug(x) kDebug(7127) << __LINE__ << ": " x
#define connected() do{myDebug( << "_______ emitting connected()" << endl); connected();}while(0)
#define dataReq() do{myDebug( << "_______ emitting dataReq()" << endl); dataReq();}while(0)
#define needSubURLData() do{myDebug( << "_______ emitting needSubURLData()" << endl); needSubURLData();}while(0)
#define slaveStatus(x,y) do{myDebug( << "_______ emitting slaveStatus(" << x << ", " << y << ")" << endl); slaveStatus(x,y);}while(0)
#define statEntry(x) do{myDebug( << "_______ emitting statEntry("<<x.count()<<")" << endl); statEntry(x);}while(0)
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

#ifdef Q_WS_WIN
#define ENDLINE "\r\n"
#else
#define ENDLINE '\n'
#endif

static char *sshPath = NULL;
static char *suPath = NULL;
// disabled: currently not needed. Didn't work reliably.
// static int isOpenSSH = 0;

/** the SSH process used to communicate with the remote end */
#ifndef Q_WS_WIN
static pid_t childPid;
#else
static KProcess *childPid = 0;
#endif

#define E(x) ((const char*)remoteEncoding()->encode(x).data())

using namespace KIO;
extern "C" {

int KDE_EXPORT kdemain( int argc, char **argv )
{
    QCoreApplication app(argc, argv);
    KComponentData componentData("fish", "kio_fish");

    myDebug( << "*** Starting fish " << endl);
    if (argc != 4) {
        myDebug( << "Usage: fish  protocol domain-socket1 domain-socket2" << endl);
        exit(-1);
    }

    setenv("TZ", "UTC", true);

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

fishProtocol::fishProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
  : SlaveBase("fish", pool_socket, app_socket), mimeBuffer(1024, '\0'),
    mimeTypeSent(false)
{
    myDebug( << "fishProtocol::fishProtocol()" << endl);
    if (sshPath == NULL) {
        // disabled: currently not needed. Didn't work reliably.
        // isOpenSSH = !system("ssh -V 2>&1 | grep OpenSSH > /dev/null");
#ifdef Q_WS_WIN
        sshPath = strdup(QFile::encodeName(KStandardDirs::findExe("plink")));
#else
        sshPath = strdup(QFile::encodeName(KStandardDirs::findExe("ssh")));
#endif
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
    connectionAuth.keepPassword = true;
    connectionAuth.url.setProtocol("fish");
    outBufPos = -1;
    outBuf = NULL;
    outBufLen = 0;

    udsType = 0;

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
       error( KIO::ERR_UNKNOWN_HOST, QString() );
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

// XXX Use KPty! XXX
#ifndef Q_WS_WIN
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
#endif
/**
creates the subprocess
*/
bool fishProtocol::connectionStart() {
    int fd[2];
    int rc, flags;
    thisFn.clear();

#ifndef Q_WS_WIN
    rc = open_pty_pair(fd);
    if (rc == -1) {
        myDebug( << "socketpair failed, error: " << strerror(errno) << endl);
        return true;
    }
#endif

    if (!requestNetwork()) return true;
    myDebug( << "Exec: " << (local ? suPath : sshPath) << " Port: " << connectionPort << " User: " << connectionUser << endl);
#ifdef Q_WS_WIN
    childPid = new KProcess();
    childPid->setOutputChannelMode(KProcess::MergedChannels);
    QStringList common_args;
    common_args << "-l" << connectionUser.toLatin1().constData() << "-x" << connectionHost.toLatin1().constData();
    common_args << "echo;echo FISH:;exec /bin/sh -c \"if env true 2>/dev/null; then env PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; else PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; fi\"";

    childPid->setProgram(sshPath, common_args);
    childPid->start();
    
    QByteArray buf;
    int offset = 0;
    while (!isLoggedIn) {
        if (outBuf.size()) {
            rc = childPid->write(outBuf);
            outBuf.clear();
        }
        else rc = 0;

        if(rc < 0) {
            myDebug( << "write failed, rc: " << rc);
            outBufPos = -1;
            //return true;
        }

        if (childPid->waitForReadyRead(1000)) {
            QByteArray buf2 = childPid->readAll();
            buf += buf2;

            int noff = establishConnection(buf);
            if (noff < 0) return false;
            if (noff > 0) buf = buf.mid(/*offset+*/noff);
//             offset = noff;
        }
    }
#else
    childPid = fork();
    if (childPid == -1) {
        myDebug( << "fork failed, error: " << strerror(errno) << endl);
        ::close(fd[0]);
        ::close(fd[1]);
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
            if (i != fd[1]) ::close(i);

        dup2(fd[1],0);
        dup2(fd[1],1);
        dup2(fd[1],2);
        if (fd[1] > 2) ::close(fd[1]);

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
        if (dev) ::close(::open(dev, O_WRONLY, 0));
        setpgid(0,0);

        if (local) {
            execl(suPath, "su", "-", connectionUser.toLatin1().constData(), "-c", "cd ~;echo FISH:;exec /bin/sh -c \"if env true 2>/dev/null; then env PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; else PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; fi\"", (void *)0);
        } else {
            #define common_args "-l", connectionUser.toLatin1().constData(), "-x", "-e", "none", \
                    "-q", connectionHost.toLatin1().constData(),        \
                "echo FISH:;exec /bin/sh -c \"if env true 2>/dev/null; then env PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; else PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; fi\"", (void *)0
            // disabled: leave compression up to the client.
            // (isOpenSSH?"-C":"+C"),

            if (connectionPort)
                execl(sshPath, "ssh", "-p", qPrintable(QString::number(connectionPort)), common_args);
            else
                execl(sshPath, "ssh", common_args);
            #undef common_args
        }
        myDebug( << "could not exec! " << strerror(errno) << endl);
        ::exit(-1);
    }
    ::close(fd[1]);
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
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
        rc = select(childFd+1, &rfds, &wfds, NULL, &timeout);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            myDebug( << "select failed, rc: " << rc << ", error: " << strerror(errno) << endl);
            return true;
        }
        if (FD_ISSET(childFd, &wfds) && outBufPos >= 0) {
            if (outBuf)
                rc = ::write(childFd, outBuf + outBufPos, outBufLen - outBufPos);
            else
                rc = 0;

            if (rc >= 0)
                outBufPos += rc;
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
        } else if (FD_ISSET(childFd,&rfds)) {
            rc = ::read(childFd, buf + offset, sizeof(buf) - offset);
            if (rc > 0) {
                int noff = establishConnection(buf, rc + offset);
                if (noff < 0)
                    return false;
                if (noff > 0)
                    memmove(buf, buf + offset + rc - noff, noff);
                offset = noff;
            } else {
                if (errno == EINTR)
                    continue;
                myDebug( << "read failed, rc: " << rc << ", error: " << strerror(errno) << endl);
                return true;
            }
        }
    }
#endif
    return false;
}

/**
writes one chunk of data to stdin of child process
*/
#ifndef Q_WS_WIN
void fishProtocol::writeChild(const char *buf, KIO::fileoffset_t len) {
    if (outBufPos >= 0 && outBuf) {
#else
void fishProtocol::writeChild(const QByteArray &buf, KIO::fileoffset_t len) {
    if (outBufPos >= 0 && outBuf.size()) {
#endif
#if 0
        QString debug = QString::fromLatin1(outBuf,outBufLen);
        if (len > 0) myDebug( << "write request while old one is pending, throwing away input (" << outBufLen << "," << outBufPos << "," << debug.left(10) << "...)" << endl);
#endif
        return;
    }
    outBuf = buf;
    outBufPos = 0;
    outBufLen = len;
}

/**
manages initial communication setup including password queries
*/
#ifndef Q_WS_WIN
int fishProtocol::establishConnection(char *buffer, KIO::fileoffset_t len) {
    QString buf = QString::fromLatin1(buffer,len);
#else
int fishProtocol::establishConnection(const QByteArray &buffer) {
    QString buf = buffer;
#endif
    int pos=0;
    // Strip trailing whitespace
    while (buf.length() && (buf[buf.length()-1] == ' '))
       buf.truncate(buf.length()-1);

    myDebug( << "establishing: got " << buf << endl);
    while (childPid && ((pos = buf.indexOf('\n')) >= 0 ||
            buf.endsWith(':') || buf.endsWith('?'))) {
        pos++;
        QString str = buf.left(pos);
        buf = buf.mid(pos);
        if (str == "\n")
            continue;
        if (str == "FISH:\n") {
            thisFn.clear();
            infoMessage(i18n("Initiating protocol..."));
            if (!connectionAuth.password.isEmpty()) {
                connectionAuth.password = connectionAuth.password.left(connectionAuth.password.length()-1);
                cacheAuthentication(connectionAuth);
            }
            isLoggedIn = true;
            return 0;
        } else if (!str.isEmpty()) {
            thisFn += str;
        } else if (buf.endsWith(':')) {
            if (!redirectUser.isEmpty() && connectionUser != redirectUser) {
                KUrl dest = url;
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
                connectionAuth.password = connectionPassword+ENDLINE;
                connectionPassword.clear();
                // su does not like receiving a password directly after sending
                // the password prompt so we wait a while.
                if (local)
                    sleep(1);
                writeChild(connectionAuth.password.toLatin1(),connectionAuth.password.length());
            } else {
                myDebug( << "sending mpass" << endl);
                connectionAuth.prompt = thisFn+buf;
                if (local)
                    connectionAuth.caption = i18n("Local Login");
                else
                    connectionAuth.caption = i18n("SSH Authorization");
                if ((!firstLogin || !checkCachedAuthentication(connectionAuth))) {
                    connectionAuth.password.clear(); // don't prefill
                    if ( !openPasswordDialog(connectionAuth)) {
                        error(ERR_USER_CANCELED,connectionHost);
                        shutdownConnection();
                        return -1;
                    }
                }
                firstLogin = false;
                connectionAuth.password += ENDLINE;
                if (connectionAuth.username != connectionUser) {
                    KUrl dest = url;
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
                writeChild(connectionAuth.password.toLatin1(),connectionAuth.password.length());
            }
            thisFn.clear();
#ifdef Q_WS_WIN
            return buf.length();
        }
#else
            return 0;
        } else if (buf.endsWith('?')) {
            int rc = messageBox(QuestionYesNo,thisFn+buf);
            if (rc == KMessageBox::Yes) {
                writeChild("yes\n",4);
            } else {
                writeChild("no\n",3);
            }
            thisFn.clear();
            return 0;
        }
#endif
        else {
            myDebug( << "unmatched case in initial handling! should not happen!" << endl);
        }
#ifdef Q_WS_WIN
        if (buf.endsWith(QLatin1String("(y/n)"))) {
            int rc = messageBox(QuestionYesNo,thisFn+buf);
            if (rc == KMessageBox::Yes) {
                writeChild("y\n",2);
            } else {
                writeChild("n\n",2);
            }
            thisFn.clear();
            return 0;
        }
#endif
    }
    return buf.length();
}

void fishProtocol::setHostInternal(const KUrl & u){
  int port = u.port();
  if(port <= 0 ) // no port is -1 in QUrl, but in kde3 we used 0 and the kioslaves assume that.
     port = 0;
  setHost(u.host(),port,u.user(),u.pass());
}

/**
sets connection information for subsequent commands
*/
void fishProtocol::setHost(const QString & host, quint16 port, const QString & u, const QString & pass){
    QString user(u);

    local = (host == "localhost" && port == 0);
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
#ifdef Q_WS_WIN
        childPid->terminate();
#else
        int killStatus = kill(childPid,SIGTERM); // We may not have permission...
        if (killStatus == 0) waitpid(childPid, 0, 0);
#endif
        childPid = 0;
#ifndef Q_WS_WIN
        ::close(childFd); // ...in which case this should do the trick
        childFd = -1;
#endif
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
    myDebug( << "queuing: cmd="<< cmd << "['" << info.command << "'](" << info.params <<"), alt=['" << info.alt << "'], lines=" << info.lines << endl);

    va_list list;
    va_start(list, cmd);
    QString realCmd = info.command;
    QString realAlt = info.alt;
    static QRegExp rx("[][\\\\\n $`#!()*?{}~&<>;'\"%^@|\t]");
    for (int i = 0; i < info.params; i++) {
        QString arg(va_arg(list, const char *));
        int pos = -2;
        while ((pos = rx.indexIn(arg,pos+2)) >= 0) {
            arg.replace(pos,0,QString("\\"));
        }
        //myDebug( << "arg " << i << ": " << arg << endl);
        realCmd.append(" ").append(arg);
        realAlt.replace(QRegExp('%'+QString::number(i+1)),arg);
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
    if (str.startsWith(QLatin1String("### "))) {
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
    QDateTime dt(QDateTime::currentDateTime().toUTC());
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

    int pos = timeyearStr.indexOf(':');
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
    QDateTime dt;
    long pos, pos2, pos3;
    bool isOk = false;
    if (!rc) {
        switch (fishCommand) {
        case FISH_VER:
            if (line.startsWith(QLatin1String("VER 0.0.3"))) {
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
            myDebug( << "listReason: " << static_cast<int>(listReason) << endl);
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
                {
                    long long val = line.toLongLong(&isOk);
                    if (val > 0 && isOk) errorCount--;
                    if ((fishCommand == FISH_LIST) && (listReason == LIST))
                        totalSize(val);
                }
                break;

                case 'P':
                {
                    errorCount--;
                    if (line[1] == 'd') {
                        udsMime = "inode/directory";
                        udsType = S_IFDIR;
                    } else {
                        if (line[1] == '-') {
                            udsType = S_IFREG;
                        } else if (line[1] == 'l') {
                            udsType = S_IFLNK;
                        } else if (line[1] == 'c') {
                            udsType = S_IFCHR;
                        } else if (line[1] == 'b') {
                            udsType = S_IFBLK;
                        } else if (line[1] == 's') {
                            udsType = S_IFSOCK;
                        } else if (line[1] == 'p') {
                            udsType = S_IFIFO;
                        } else {
                            myDebug( << "unknown file type: " << line[1].cell() << endl);
                            errorCount++;
                            break;
                        }
                    }
                    //myDebug( << "file type: " << udsType << endl);

                    long long accessVal = 0;
                    if (line[2] == 'r') accessVal |= S_IRUSR;
                    if (line[3] == 'w') accessVal |= S_IWUSR;
                    if (line[4] == 'x' || line[4] == 's') accessVal |= S_IXUSR;
                    if (line[4] == 'S' || line[4] == 's') accessVal |= S_ISUID;
                    if (line[5] == 'r') accessVal |= S_IRGRP;
                    if (line[6] == 'w') accessVal |= S_IWGRP;
                    if (line[7] == 'x' || line[7] == 's') accessVal |= S_IXGRP;
                    if (line[7] == 'S' || line[7] == 's') accessVal |= S_ISGID;
                    if (line[8] == 'r') accessVal |= S_IROTH;
                    if (line[9] == 'w') accessVal |= S_IWOTH;
                    if (line[10] == 'x' || line[10] == 't') accessVal |= S_IXOTH;
                    if (line[10] == 'T' || line[10] == 't') accessVal |= S_ISVTX;
                    udsEntry.insert(KIO::UDSEntry::UDS_ACCESS, accessVal);

                    pos = line.indexOf(':',12);
                    if (pos < 0) {
                        errorCount++;
                        break;
                    }
                    udsEntry.insert(KIO::UDSEntry::UDS_USER, line.mid(12,pos-12));
                    udsEntry.insert(KIO::UDSEntry::UDS_GROUP, line.mid(pos+1));
                }
                break;

                case 'd':
                    pos = line.indexOf(' ');
                    pos2 = line.indexOf(' ',pos+1);
                    if (pos < 0 || pos2 < 0) break;
                    errorCount--;
                    udsEntry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME,
                                    makeTimeFromLs(line.mid(1,pos-1), line.mid(pos+1,pos2-pos), line.mid(pos2+1)));
                    break;

                case 'D':
                    pos = line.indexOf(' ');
                    pos2 = line.indexOf(' ',pos+1);
                    pos3 = line.indexOf(' ',pos2+1);
                    if (pos < 0 || pos2 < 0 || pos3 < 0) break;
                    dt.setDate(QDate(line.mid(1,pos-1).toInt(),line.mid(pos+1,pos2-pos-1).toInt(),line.mid(pos2+1,pos3-pos2-1).toInt()));
                    pos = pos3;
                    pos2 = line.indexOf(' ',pos+1);
                    pos3 = line.indexOf(' ',pos2+1);
                    if (pos < 0 || pos2 < 0 || pos3 < 0) break;
                    dt.setTime(QTime(line.mid(pos+1,pos2-pos-1).toInt(),line.mid(pos2+1,pos3-pos2-1).toInt(),line.mid(pos3+1).toInt()));
                    errorCount--;
                    udsEntry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, dt.toTime_t());
                    break;

                case 'S':
                {
                    long long sizeVal = line.mid(1).toLongLong(&isOk);
                    if (!isOk) break;
                    errorCount--;
                    udsEntry.insert(KIO::UDSEntry::UDS_SIZE, sizeVal);
                }
                break;

                case 'E':
                    errorCount--;
                    break;

                case ':':
                    pos = line.lastIndexOf('/');
                    thisFn = line.mid(pos < 0?1:pos+1);
                    if (fishCommand == FISH_LIST) {
                        udsEntry.insert(KIO::UDSEntry::UDS_NAME, thisFn);
                    }
                    // By default, the mimetype comes from the extension
                    // We'll use the file(1) result only as fallback [like the rest of KDE does]
                    {
                      KUrl kurl("fish://host/");
                      kurl.setFileName(thisFn);  // properly encode special chars
                      KMimeType::Ptr mime = KMimeType::findByUrl(kurl, udsType);
                      if ( mime->name() != KMimeType::defaultMimeType() )
                          udsMime = mime->name();
                    }
                    errorCount--;
                    break;

                case 'M':
                    // This is getting ugly. file(1) makes some uneducated
                    // guesses, so we must try to ignore them (#51274)
                    if (udsMime.isEmpty() && line.right(8) != "/unknown" &&
                            (thisFn.indexOf('.') < 0 || (line.left(8) != "Mtext/x-"
                                                  && line != "Mtext/plain"))) {
                        udsMime = line.mid(1);
                        if ( udsMime == "inode/directory" ) // a symlink to a dir is a dir
                          udsType = S_IFDIR;
                    }
                    errorCount--;
                    break;

                case 'L':
                    udsEntry.insert(KIO::UDSEntry::UDS_LINK_DEST, line.mid(1));
                    if (!udsType) udsType = S_IFLNK;
                    errorCount--;
                    break;
                }
            } else {
                if (!udsMime.isNull())
                    udsEntry.insert(KIO::UDSEntry::UDS_MIME_TYPE, udsMime);
                udsMime.clear();

                udsEntry.insert( KIO::UDSEntry::UDS_FILE_TYPE, udsType );
                udsType = 0;

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
                error(ERR_IS_DIRECTORY,url.prettyUrl());
                recvLen = 0;
                break;
            }
            recvLen = line.toLongLong(&isOk);
            if (!isOk) {
                error(ERR_COULD_NOT_READ,url.prettyUrl());
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
                error(ERR_COULD_NOT_READ,url.prettyUrl());
                shutdownConnection();
            } else {
                rawRead = recvLen;
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
            error(ERR_COULD_NOT_WRITE,url.prettyUrl());
            shutdownConnection();
            break;
        case FISH_RETR:
            error(ERR_COULD_NOT_READ,url.prettyUrl());
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
               error(ERR_COULD_NOT_READ,url.prettyUrl());
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
            error(ERR_CANNOT_ENTER_DIRECTORY,url.prettyUrl());
            break;
        case FISH_LIST:
            myDebug( << "list error. reason: " << static_cast<int>(listReason) << endl);
            if (listReason == LIST) error(ERR_CANNOT_ENTER_DIRECTORY,url.prettyUrl());
            else if (listReason == CHECK) {
                checkExist = false;
                finished();
            }
            break;
        case FISH_STAT:
            error(ERR_DOES_NOT_EXIST,url.prettyUrl());
            udsStatEntry.clear();
            break;
        case FISH_CHMOD:
            error(ERR_CANNOT_CHMOD,url.prettyUrl());
            break;
        case FISH_CHOWN:
        case FISH_CHGRP:
            error(ERR_ACCESS_DENIED,url.prettyUrl());
            break;
        case FISH_MKD:
            if ( rc == 501 )
                error(ERR_DIR_ALREADY_EXIST,url.prettyUrl());
            else
                error(ERR_COULD_NOT_MKDIR,url.prettyUrl());
            break;
        case FISH_RMD:
            error(ERR_COULD_NOT_RMDIR,url.prettyUrl());
            break;
        case FISH_DELE:
            error(ERR_CANNOT_DELETE,url.prettyUrl());
            break;
        case FISH_RENAME:
            error(ERR_CANNOT_RENAME,url.prettyUrl());
            break;
        case FISH_COPY:
        case FISH_LINK:
        case FISH_SYMLINK:
            error(ERR_COULD_NOT_WRITE,url.prettyUrl());
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
                    error(ERR_FILE_ALREADY_EXIST,url.prettyUrl());
                    return; // Don't call finished!
                }
            }
        } else if (fishCommand == FISH_STAT) {
            udsStatEntry.insert( KIO::UDSEntry::UDS_NAME, url.fileName() );
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
    qlist.append(line.toLatin1());

    if (writeReady) {
        writeReady = false;
        //myDebug( << "Writing: " << qlist.first().mid(0,qlist.first().indexOf('\n')) << endl);
        myDebug( << "Writing: " << qlist.first() << endl);
        myDebug( << "---------" << endl);
        writeChild((const char *)qlist.first(), qlist.first().length());
    }
}

void fishProtocol::sent()
{
    if (rawWrite > 0) {
        myDebug( << "writing raw: " << rawData.size() << "/" << rawWrite << endl);
        writeChild(rawData.data(),(rawWrite > rawData.size()?rawData.size():rawWrite));
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
    if (qlist.count() > 0) qlist.erase(qlist.begin());
    if (qlist.count() == 0) {
        writeReady = true;
    } else {
        //myDebug( << "Writing: " << qlist.first().mid(0,qlist.first().indexOf('\n')) << endl);
        myDebug( << "Writing: " << qlist.first() << endl);
        myDebug( << "---------" << endl);
        writeChild((const char *)qlist.first(),qlist.first().length());
    }
}

int fishProtocol::received(const char *buffer, KIO::fileoffset_t buflen)
{
    int pos = 0;
    do {
        if (buflen <= 0) break;

        if (rawRead > 0) {
            myDebug( << "processedSize " << dataRead << ", len " << buflen << "/" << rawRead << endl);
            int dataSize = (rawRead > buflen?buflen:rawRead);
            if (!mimeTypeSent)
            {
                int mimeSize = qMin(dataSize, (int)(mimeBuffer.size()-dataRead));
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
                sendmimeType(KMimeType::findByNameAndContent(url.path(), mimeBuffer)->name());
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

            QByteArray bdata(buffer,dataSize);
            data(bdata);

            dataRead += dataSize;
            rawRead -= dataSize;
            processedSize(dataRead);
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
           QString s = remoteEncoding()->decode(QByteArray(buffer,pos));

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
void fishProtocol::get(const KUrl& u){
    myDebug( << "@@@@@@@@@ get " << u << endl);
    setHostInternal(u);
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
void fishProtocol::put(const KUrl& u, int permissions, KIO::JobFlags flags) {
    myDebug( << "@@@@@@@@@ put " << u << " " << permissions << " " << (flags & KIO::Overwrite) << " " /* << resume */ << endl);
    setHostInternal(u);

    url = u;
    openConnection();
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        putPerm = permissions;

        checkOverwrite = flags & KIO::Overwrite;
        checkExist = false;
        putPos = 0;
        listReason = CHECK;
        sendCommand(FISH_LIST,E(url.path()));
        sendCommand(FISH_STOR,"0",E(url.path()));

        const QString mtimeStr = metaData( "modified" );
        if ( !mtimeStr.isEmpty() ) {
            QDateTime dt = QDateTime::fromString( mtimeStr, Qt::ISODate );
            // TODO set modification time on url.path() somehow
            // see FileProtocol::put if using utime() to do that.
        }
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
        //if (fishCommand != FISH_APPEND && fishCommand != FISH_WRITE) infoMessage("Sending "+(commandList.first().mid(1,commandList.first().indexOf("\n")-1))+"...");
        commandList.erase(commandList.begin());
        commandCodes.erase(commandCodes.begin());
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
void fishProtocol::run()
/* This function writes to childFd fish commands (like #STOR 0 /tmp/test ...) that are stored in outBuf
and reads from childFd the remote host's response. ChildFd is the fd to a process that communicates
with .fishsrv.pl typically running on another computer. */
{
    if (!isRunning) {
        int rc;
        isRunning = true;
        finished();
#ifndef Q_WS_WIN
        fd_set rfds, wfds;
        FD_ZERO(&rfds);
#endif
        char buf[32768];
        int offset = 0;
        while (isRunning) {
#ifndef Q_WS_WIN
            FD_SET(childFd,&rfds);
            FD_ZERO(&wfds);
            if (outBufPos >= 0) FD_SET(childFd,&wfds);
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 1000;
            rc = select(childFd+1, &rfds, &wfds, NULL, &timeout);
            if (rc < 0) {
                if (errno == EINTR)
                    continue;
                myDebug( << "select failed, rc: " << rc << ", error: " << strerror(errno) << endl);
                error(ERR_CONNECTION_BROKEN,connectionHost);
                shutdownConnection();
                return;
            }
	    // We first write the complete buffer, including all newlines.
	    // Do: send command and newlines, expect response then
	    // Do not: send commands, expect response, send newlines, expect response on newlines
	    // Newlines do not trigger a response.
            if (FD_ISSET(childFd,&wfds) && outBufPos >= 0) {
                if (outBufLen-outBufPos > 0)
                    rc = ::write(childFd, outBuf + outBufPos, outBufLen - outBufPos);
#else
            if (outBufPos >= 0) {
                if (outBufLen-outBufPos > 0) {
                    rc = childPid->write(outBuf);
                }
#endif
                else
                    rc = 0;

                if (rc >= 0)
                    outBufPos += rc;
                else {
#ifndef Q_WS_WIN
                    if (errno == EINTR)
                        continue;
                    myDebug( << "write failed, rc: " << rc << ", error: " << strerror(errno) << endl);
#else
                    myDebug( << "write failed, rc: " << rc);
#endif
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
#ifndef Q_WS_WIN
            else if (FD_ISSET(childFd,&rfds)) {
                rc = ::read(childFd, buf + offset, sizeof(buf) - offset);
#else
            else if (childPid->waitForReadyRead(1000)) {
                rc = childPid->read(buf + offset, sizeof(buf) - offset);
#endif
                //myDebug( << "read " << rc << " bytes" << endl);
                if (rc > 0) {
                    int noff = received(buf, rc + offset);
                    if (noff > 0) memmove(buf, buf + offset + rc - noff, noff);
                    //myDebug( << "left " << noff << " bytes: " << QString::fromLatin1(buf,offset) << endl);
                    offset = noff;
                } else {
#ifndef Q_WS_WIN
                    if (errno == EINTR)
                        continue;
                    myDebug( << "read failed, rc: " << rc << ", error: " << strerror(errno) << endl);
#else
                    myDebug( << "read failed, rc: " << rc );
#endif
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
void fishProtocol::stat(const KUrl& u){
    myDebug( << "@@@@@@@@@ stat " << u << endl);
    setHostInternal(u);
    url = u;
    isStat = true; // FIXME: just a workaround for konq deficiencies
    openConnection();
    isStat = false; // FIXME: just a workaround for konq deficiencies
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        sendCommand(FISH_STAT,E(url.path(KUrl::RemoveTrailingSlash)));
    }
    run();
}
/** find mimetype for a file */
void fishProtocol::mimetype(const KUrl& u){
    myDebug( << "@@@@@@@@@ mimetype " << u << endl);
    setHostInternal(u);
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
void fishProtocol::listDir(const KUrl& u){
    myDebug( << "@@@@@@@@@ listDir " << u << endl);
    setHostInternal(u);
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
void fishProtocol::mkdir(const KUrl& u, int permissions) {
    myDebug( << "@@@@@@@@@ mkdir " << u << " " << permissions << endl);
    setHostInternal(u);
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
void fishProtocol::rename(const KUrl& s, const KUrl& d, KIO::JobFlags flags) {
    myDebug( << "@@@@@@@@@ rename " << s << " " << d << " " << (flags & KIO::Overwrite) << endl);
    if (s.host() != d.host() || s.port() != d.port() || s.user() != d.user()) {
        error(ERR_UNSUPPORTED_ACTION,s.prettyUrl());
        return;
    }
    setHostInternal(s);
    url = d;
    openConnection();
    if (!isLoggedIn) return;
    KUrl src = s;
    url.cleanPath();
    src.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        if (!(flags & KIO::Overwrite)) {
            listReason = CHECK;
            checkOverwrite = false;
            sendCommand(FISH_LIST,E(url.path()));
        }
        sendCommand(FISH_RENAME,E(src.path()),E(url.path()));
    }
    run();
}
/** create a symlink */
void fishProtocol::symlink(const QString& target, const KUrl& u, KIO::JobFlags flags) {
    myDebug( << "@@@@@@@@@ symlink " << target << " " << u << " " << (flags & KIO::Overwrite) << endl);
    setHostInternal(u);
    url = u;
    openConnection();
    if (!isLoggedIn) return;
    url.cleanPath();
    if (!url.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        if (!(flags & KIO::Overwrite)) {
            listReason = CHECK;
            checkOverwrite = false;
            sendCommand(FISH_LIST,E(url.path()));
        }
        sendCommand(FISH_SYMLINK,E(target),E(url.path()));
    }
    run();
}
/** change file permissions */
void fishProtocol::chmod(const KUrl& u, int permissions){
    myDebug( << "@@@@@@@@@ chmod " << u << " " << permissions << endl);
    setHostInternal(u);
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
void fishProtocol::copy(const KUrl &s, const KUrl &d, int permissions, KIO::JobFlags flags) {
    myDebug( << "@@@@@@@@@ copy " << s << " " << d << " " << permissions << " " << (flags & KIO::Overwrite) << endl);
    if (s.host() != d.host() || s.port() != d.port() || s.user() != d.user()) {
        error(ERR_UNSUPPORTED_ACTION,s.prettyUrl());
        return;
    }
    //myDebug( << s << endl << d << endl);
    setHostInternal(s);
    url = d;
    openConnection();
    if (!isLoggedIn) return;
    KUrl src = s;
    url.cleanPath();
    src.cleanPath();
    if (!src.hasPath()) {
        sendCommand(FISH_PWD);
    } else {
        if (!(flags & KIO::Overwrite)) {
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
void fishProtocol::del(const KUrl &u, bool isFile){
    myDebug( << "@@@@@@@@@ del " << u << " " << isFile << endl);
    setHostInternal(u);
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
            KUrl u;
            QString command;
            QString tempfile;
            stream >> u;
            stream >> command;
            myDebug( << "@@@@@@@@@ exec " << u << " " << command << endl);
            setHostInternal(u);
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
        slaveStatus(QString(),false);
}
