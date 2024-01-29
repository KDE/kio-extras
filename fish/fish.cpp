/*
    fish.cpp  -  a FISH KIO worker
    SPDX-FileCopyrightText: 2001 Jörg Walter <trouble@garni.ch>
    SPDX-License-Identifier: GPL-2.0-only
*/

/*
  This code contains fragments and ideas from the ftp KIO worker
  done by David Faure <faure@kde.org>.

  Structure is a bit complicated, since I made the mistake to use
  KProcess... now there is a lightweight homebrew async IO system
  inside, but if signals/slots become available for KIO workers, switching
  back to KProcess should be easy.
*/

#include "fish.h"

#include <config-fish.h>
#include <config-runtime.h>

#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QRegularExpression>
#include <QStandardPaths>

#include <stdlib.h>
#include <sys/resource.h>
#ifdef HAVE_PTY_H
#include <pty.h>
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
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

#ifndef Q_OS_WIN
#include <errno.h>
#endif

#include <KLocalizedString>
#include <KRemoteEncoding>

#include "fishcode.h"
#include "loggingcategory.h"

#ifndef NDEBUG
#define myDebug(x) qCDebug(KIO_FISH_DEBUG) << __LINE__ << ": " x
#define dataReq()                                                                                                                                              \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting dataReq()");                                                                                                              \
        dataReq();                                                                                                                                             \
    } while (0)
#define needSubURLData()                                                                                                                                       \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting needSubURLData()");                                                                                                       \
        needSubURLData();                                                                                                                                      \
    } while (0)
#define workerStatus(x, y)                                                                                                                                     \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting workerStatus(" << x << ", " << y << ")");                                                                                 \
        workerStatus(x, y);                                                                                                                                    \
    } while (0)
#define statEntry(x)                                                                                                                                           \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting statEntry(" << x.count() << ")");                                                                                         \
        statEntry(x);                                                                                                                                          \
    } while (0)
#define listEntries(x)                                                                                                                                         \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting listEntries(...)");                                                                                                       \
        listEntries(x);                                                                                                                                        \
    } while (0)
#define canResume(x)                                                                                                                                           \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting canResume(" << (int)x << ")");                                                                                            \
        canResume(x);                                                                                                                                          \
    } while (0)
#define totalSize(x)                                                                                                                                           \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting totalSize(" << (int)x << ")");                                                                                            \
        totalSize(x);                                                                                                                                          \
    } while (0)
#define processedSize(x)                                                                                                                                       \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting processedSize(" << x << ")");                                                                                             \
        processedSize(x);                                                                                                                                      \
    } while (0)
#define speed(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting speed(" << (int)x << ")");                                                                                                \
        speed(x);                                                                                                                                              \
    } while (0)
#define redirection(x)                                                                                                                                         \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting redirection(" << x << ")");                                                                                               \
        redirection(x);                                                                                                                                        \
    } while (0)
#define errorPage()                                                                                                                                            \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting errorPage()");                                                                                                            \
        errorPage();                                                                                                                                           \
    } while (0)
#define sendmimeType(x)                                                                                                                                        \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting mimeType(" << x << ")");                                                                                                  \
        mimeType(x);                                                                                                                                           \
    } while (0)
#define warning(x)                                                                                                                                             \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting warning(" << x << ")");                                                                                                   \
        warning(x);                                                                                                                                            \
    } while (0)
#define infoMessage(x)                                                                                                                                         \
    do {                                                                                                                                                       \
        myDebug(<< "_______ emitting infoMessage(" << x << ")");                                                                                               \
        infoMessage(x);                                                                                                                                        \
    } while (0)
#else
#define myDebug(x)
#define sendmimeType(x) mimeType(x)
#endif

#ifdef Q_OS_WIN
#define ENDLINE "\r\n"
#else
#define ENDLINE '\n'
#endif

static char *sshPath = nullptr;
static char *suPath = nullptr;
// disabled: currently not needed. Didn't work reliably.
// static int isOpenSSH = 0;

/** the SSH process used to communicate with the remote end */
#ifndef Q_OS_WIN
static pid_t childPid;
#else
static KProcess *childPid = 0;
#endif

#define E(x) ((const char *)remoteEncoding()->encode(x).data())

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.fish" FILE "fish.json")
};

using namespace KIO;
extern "C" {

int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("kio_fish");

    myDebug(<< "*** Starting fish ");
    if (argc != 4) {
        myDebug(<< "Usage: kio_fish protocol domain-socket1 domain-socket2");
        exit(-1);
    }

    setenv("TZ", "UTC", true);

    fishProtocol worker(argv[2], argv[3]);
    worker.dispatchLoop();

    myDebug(<< "*** fish Done");
    return 0;
}
}

const struct fishProtocol::fish_info fishProtocol::fishInfo[] = {
    {("FISH"),
     0,
     ("echo; /bin/sh -c start_fish_server > /dev/null 2>/dev/null; perl .fishsrv.pl " CHECKSUM
      " 2>/dev/null; perl -e '$|=1; print \"### 100 transfer fish server\\n\"; while(<STDIN>) { last if /^__END__/; $code.=$_; } exit(eval($code));' "
      "2>/dev/null;"),
     1},
    {("VER 0.0.3 copy append lscount lslinks lsmime exec stat"), 0, ("echo 'VER 0.0.3 copy append lscount lslinks lsmime exec stat'"), 1},
    {("PWD"), 0, ("pwd"), 1},
    {("LIST"),
     1,
     ("echo `ls -Lla %1 2> /dev/null | grep '^[-dsplcb]' | wc -l`;"
      "echo '### 100';"
      "ls -Lla %1 2>/dev/null | grep '^[-dspl]' | ( while read -r p x u g s m d y n; do file -b -i $n 2>/dev/null | sed -e '\\,^[^/]*$,d;s/^/M/;s,/.*[ "
      "\t],/,'; FILE=%1; if [ -e %1\"/$n\" ]; then FILE=%1\"/$n\"; fi; if [ -L \"$FILE\" ]; then echo \":$n\"; ls -lad \"$FILE\" | sed -e 's/.* -> /L/'; else "
      "echo \":$n\" | sed -e 's/ -> /\\\nL/'; fi; echo \"P$p $u.$g\nS$s\nd$m $d $y\n\"; done; );"
      "ls -Lla %1 2>/dev/null | grep '^[cb]' | ( while read -r p x u g a i m d y n; do echo \"P$p $u.$g\nE$a$i\nd$m $d $y\n:$n\n\"; done; )"),
     0},
    {("STAT"),
     1,
     ("echo `ls -dLla %1 2> /dev/null | grep '^[-dsplcb]' | wc -l`;"
      "echo '### 100';"
      "ls -dLla %1 2>/dev/null | grep '^[-dspl]' | ( while read -r p x u g s m d y n; do file -b -i $n 2>/dev/null | sed -e '\\,^[^/]*$,d;s/^/M/;s,/.*[ "
      "\t],/,'; FILE=%1; if [ -e %1\"/$n\" ]; then FILE=%1\"/$n\"; fi; if [ -L \"$FILE\" ]; then echo \":$n\"; ls -lad \"$FILE\" | sed -e 's/.* -> /L/'; else "
      "echo \":$n\" | sed -e 's/ -> /\\\nL/'; fi; echo \"P$p $u.$g\nS$s\nd$m $d $y\n\"; done; );"
      "ls -dLla %1 2>/dev/null | grep '^[cb]' | ( while read -r p x u g a i m d y n; do echo \"P$p $u.$g\nE$a$i\nd$m $d $y\n:$n\n\"; done; )"),
     0},
    {("RETR"), 1, ("ls -l %1 2>&1 | ( read -r a b c d x e; echo $x ) 2>&1; echo '### 001'; cat %1"), 1},
    {("STOR"),
     2,
     ("> %2; echo '### 001'; ( [ \"`expr %1 / 4096`\" -gt 0 ] && dd bs=4096 count=`expr %1 / 4096` 2>/dev/null;"
      "[ \"`expr %1 % 4096`\" -gt 0 ] && dd bs=`expr %1 % 4096` count=1 2>/dev/null; ) | ( cat > %2 || echo Error $?; cat > /dev/null )"),
     0},
    {("CWD"), 1, ("cd %1"), 0},
    {("CHMOD"), 2, ("chmod %1 %2"), 0},
    {("DELE"), 1, ("rm -f %1"), 0},
    {("MKD"), 1, ("mkdir %1"), 0},
    {("RMD"), 1, ("rmdir %1"), 0},
    {("RENAME"), 2, ("mv -f %1 %2"), 0},
    {("LINK"), 2, ("ln -f %1 %2"), 0},
    {("SYMLINK"), 2, ("ln -sf %1 %2"), 0},
    {("CHOWN"), 2, ("chown %1 %2"), 0},
    {("CHGRP"), 2, ("chgrp %1 %2"), 0},
    {("READ"),
     3,
     ("echo '### 100';cat %3 /dev/zero | ( [ \"`expr %1 / 4096`\" -gt 0 ] && dd bs=4096 count=`expr %1 / 4096` >/dev/null;"
      "[ \"`expr %1 % 4096`\" -gt 0 ] && dd bs=`expr %1 % 4096` count=1 >/dev/null;"
      "dd bs=%2 count=1; ) 2>/dev/null;"),
     0},
    // Yes, this is "ibs=1", since dd "count" is input blocks.
    // On network connections, read() may not fill the buffer
    // completely (no more data immediately available), but dd
    // does ignore that fact by design. Sorry, writes are slow.
    // OTOH, WRITE is not used by the current KIO worker methods,
    // we use APPEND.
    {("WRITE"),
     3,
     (">> %3; echo '### 001'; ( [ %2 -gt 0 ] && dd ibs=1 obs=%2 count=%2 2>/dev/null ) | "
      "( dd ibs=32768 obs=%1 seek=1 of=%3 2>/dev/null || echo Error $?; cat >/dev/null; )"),
     0},
    {("COPY"), 2, ("if [ -L %1 ]; then if cp -pdf %1 %2 2>/dev/null; then :; else LINK=\"`readlink %1`\"; ln -sf $LINK %2; fi; else cp -pf %1 %2; fi"), 0},
    {("APPEND"), 2, (">> %2; echo '### 001'; ( [ %1 -gt 0 ] && dd ibs=1 obs=%1 count=%1 2> /dev/null; ) | ( cat >> %2 || echo Error $?; cat >/dev/null; )"), 0},
    {("EXEC"), 2, ("UMASK=`umask`; umask 077; touch %2; umask $UMASK; eval %1 < /dev/null > %2 2>&1; echo \"###RESULT: $?\" >> %2"), 0}};

fishProtocol::fishProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
    : WorkerBase("fish", pool_socket, app_socket)
    , mimeBuffer(1024, '\0')
    , mimeTypeSent(false)
{
    myDebug(<< "fishProtocol::fishProtocol()");
    if (sshPath == nullptr) {
        // disabled: currently not needed. Didn't work reliably.
        // isOpenSSH = !system("ssh -V 2>&1 | grep OpenSSH > /dev/null");
#ifdef Q_OS_WIN
        sshPath = strdup(QFile::encodeName(QStandardPaths::findExecutable("plink")).constData());
#else
        sshPath = strdup(QFile::encodeName(QStandardPaths::findExecutable("ssh")).constData());
#endif
    }
    if (suPath == nullptr) {
        suPath = strdup(QFile::encodeName(QStandardPaths::findExecutable("su")).constData());
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
    connectionAuth.url.setScheme("fish");
    outBufPos = -1;
    outBuf = QByteArray();

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
    myDebug(<< "fishProtocol::~fishProtocol()");
    shutdownConnection(true);
}

/* --------------------------------------------------------------------------- */

/**
Connects to a server and logs us in via SSH. Then starts FISH protocol.
*/
KIO::WorkerResult fishProtocol::openConnection()
{
    if (childPid)
        return KIO::WorkerResult::pass();

    if (connectionHost.isEmpty()) {
        return error(KIO::ERR_UNKNOWN_HOST, QString());
    }

    infoMessage(i18n("Connecting..."));

    myDebug(<< "connecting to: " << connectionUser << "@" << connectionHost << ":" << connectionPort);
    sendCommand(FISH_FISH);
    sendCommand(FISH_VER);
    const auto result = connectionStart();
    if (!result.success()) {
        shutdownConnection();
        return result;
    };
    myDebug(<< "subprocess is running");
    return result;
}

// XXX Use KPty! XXX
#ifndef Q_OS_WIN
static int open_pty_pair(int fd[2])
{
#if defined(HAVE_TERMIOS_H) && defined(HAVE_GRANTPT) && !defined(HAVE_OPENPTY)
    /** with kind regards to The GNU C Library
    Reference Manual for Version 2.2.x of the GNU C Library */
    int master, slave;
    char *name;
    struct ::termios ti;
    memset(&ti, 0, sizeof(ti));

    ti.c_cflag = CLOCAL | CREAD | CS8;
    ti.c_cc[VMIN] = 1;

#ifdef HAVE_GETPT
    master = getpt();
#else
    master = open("/dev/ptmx", O_RDWR);
#endif
    if (master < 0)
        return 0;

    if (grantpt(master) < 0 || unlockpt(master) < 0)
        goto close_master;

    name = ptsname(master);
    if (name == NULL)
        goto close_master;

    slave = open(name, O_RDWR);
    if (slave == -1)
        goto close_master;

#if (defined(HAVE_ISASTREAM) || defined(isastream)) && defined(I_PUSH)
    if (isastream(slave) && (ioctl(slave, I_PUSH, "ptem") < 0 || ioctl(slave, I_PUSH, "ldterm") < 0))
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
    memset(&ti, 0, sizeof(ti));

    ti.c_cflag = CLOCAL | CREAD | CS8;
    ti.c_cc[VMIN] = 1;

    return openpty(fd, fd + 1, nullptr, &ti, nullptr);
#else
#ifdef __GNUC__
#warning "No tty support available. Password dialog won't work."
#endif
    return socketpair(PF_UNIX, SOCK_STREAM, 0, fd);
#endif
#endif
}
#endif
/**
creates the subprocess
*/
KIO::WorkerResult fishProtocol::connectionStart()
{
    int fd[2];
    int rc, flags;
    thisFn.clear();

#ifndef Q_OS_WIN
    rc = open_pty_pair(fd);
    if (rc == -1) {
        myDebug(<< "socketpair failed, error: " << strerror(errno));
        return error(ERR_CANNOT_CONNECT, connectionHost);
        ;
    }
#endif

    myDebug(<< "Exec: " << (local ? suPath : sshPath) << " Port: " << connectionPort << " User: " << connectionUser);
#ifdef Q_OS_WIN
    childPid = new KProcess();
    childPid->setOutputChannelMode(KProcess::MergedChannels);
    QStringList common_args;
    common_args << "-l" << connectionUser.toLatin1().constData() << "-x" << connectionHost.toLatin1().constData();
    common_args << "echo;echo FISH:;exec /bin/sh -c \"if env true 2>/dev/null; then env PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; else PS1= PS2= "
                   "TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; fi\"";

    childPid->setProgram(sshPath, common_args);
    childPid->start();

    QByteArray buf;
    int offset = 0;
    while (!isLoggedIn) {
        if (outBuf.size()) {
            rc = childPid->write(outBuf);
            outBuf.clear();
        } else
            rc = 0;

        if (rc < 0) {
            myDebug(<< "write failed, rc: " << rc);
            outBufPos = -1;
            // return true;
        }

        if (childPid->waitForReadyRead(1000)) {
            QByteArray buf2 = childPid->readAll();
            buf += buf2;

            const auto establishConnectionResult = establishConnection(buf, rc + offset);
            if (!establishConnectionResult.result.success()) {
                return establishConnectionResult.result;
            }
            const int noff = establishConnectionResult.remainingBufferSize;
            if (noff > 0)
                buf = buf.mid(/*offset+*/ noff);
            //             offset = noff;
        }
    }
#else
    childPid = fork();
    if (childPid == -1) {
        myDebug(<< "fork failed, error: " << strerror(errno));
        ::close(fd[0]);
        ::close(fd[1]);
        childPid = 0;
        return error(ERR_CANNOT_CONNECT, connectionHost);
        ;
    }
    if (childPid == 0) {
        // taken from konsole, see TEPty.C for details
        // note: if we're running on socket pairs,
        // this will fail, but thats what we expect

        for (int sig = 1; sig < NSIG; sig++)
            signal(sig, SIG_DFL);

        struct rlimit rlp;
        getrlimit(RLIMIT_NOFILE, &rlp);
        for (int i = 0; i < (int)rlp.rlim_cur; i++)
            if (i != fd[1])
                ::close(i);

        dup2(fd[1], 0);
        dup2(fd[1], 1);
        dup2(fd[1], 2);
        if (fd[1] > 2)
            ::close(fd[1]);

        setsid();

#if defined(TIOCSCTTY)
        ioctl(0, TIOCSCTTY, 0);
#endif

        int pgrp = getpid();
#if defined(_AIX) || defined(__hpux)
        tcsetpgrp(0, pgrp);
#else
        ioctl(0, TIOCSPGRP, (char *)&pgrp);
#endif

        const char *dev = ttyname(0);
        setpgid(0, 0);
        if (dev)
            ::close(::open(dev, O_WRONLY, 0));
        setpgid(0, 0);

        if (local) {
            execl(suPath,
                  "su",
                  "-",
                  connectionUser.toLatin1().constData(),
                  "-c",
                  "cd ~;echo FISH:;exec /bin/sh -c \"if env true 2>/dev/null; then env PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; else PS1= PS2= "
                  "TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; fi\"",
                  (void *)nullptr);
        } else {
#define common_args                                                                                                                                            \
    "-l", connectionUser.toLatin1().constData(), "-x", "-e", "none", "-q", connectionHost.toLatin1().constData(),                                              \
        "echo FISH:;exec /bin/sh -c \"if env true 2>/dev/null; then env PS1= PS2= TZ=UTC LANG=C LC_ALL=C LOCALE=C /bin/sh; else PS1= PS2= TZ=UTC LANG=C "      \
        "LC_ALL=C LOCALE=C /bin/sh; fi\"",                                                                                                                     \
        (void *)nullptr
            // disabled: leave compression up to the client.
            // (isOpenSSH?"-C":"+C"),

            if (connectionPort)
                execl(sshPath, "ssh", "-p", qPrintable(QString::number(connectionPort)), common_args);
            else
                execl(sshPath, "ssh", common_args);
#undef common_args
        }
        myDebug(<< "could not exec! " << strerror(errno));
        ::exit(-1);
    }
    ::close(fd[1]);
    rc = fcntl(fd[0], F_GETFL, &flags);
    rc = fcntl(fd[0], F_SETFL, flags | O_NONBLOCK);
    childFd = fd[0];

    fd_set rfds, wfds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    char buf[32768];
    int offset = 0;
    while (!isLoggedIn) {
        FD_SET(childFd, &rfds);
        FD_ZERO(&wfds);
        if (outBufPos >= 0)
            FD_SET(childFd, &wfds);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
        rc = select(childFd + 1, &rfds, &wfds, nullptr, &timeout);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            myDebug(<< "select failed, rc: " << rc << ", error: " << strerror(errno));
            return error(ERR_CANNOT_CONNECT, connectionHost);
            ;
        }
        if (FD_ISSET(childFd, &wfds) && outBufPos >= 0) {
            if (!outBuf.isEmpty())
                rc = ::write(childFd, outBuf.constData() + outBufPos, outBuf.length() - outBufPos);
            else
                rc = 0;

            if (rc >= 0)
                outBufPos += rc;
            else {
                if (errno == EINTR)
                    continue;
                myDebug(<< "write failed, rc: " << rc << ", error: " << strerror(errno));
                outBufPos = -1;
                // return true;
            }
            if (outBufPos >= outBuf.length()) {
                outBufPos = -1;
                outBuf = QByteArray();
            }
        } else if (FD_ISSET(childFd, &rfds)) {
            rc = ::read(childFd, buf + offset, sizeof(buf) - offset);
            if (rc > 0) {
                const auto establishConnectionResult = establishConnection(buf, rc + offset);
                if (!establishConnectionResult.result.success()) {
                    return establishConnectionResult.result;
                }
                const int noff = establishConnectionResult.remainingBufferSize;
                if (noff > 0)
                    memmove(buf, buf + offset + rc - noff, noff);
                offset = noff;
            } else {
                if (errno == EINTR)
                    continue;
                myDebug(<< "read failed, rc: " << rc << ", error: " << strerror(errno));
                return error(ERR_CANNOT_CONNECT, connectionHost);
                ;
            }
        }
    }
#endif
    return KIO::WorkerResult::pass();
}

/**
writes one chunk of data to stdin of child process
*/
void fishProtocol::writeChild(const char *buf, KIO::fileoffset_t len)
{
    if (outBufPos >= 0 && !outBuf.isEmpty()) {
        return;
    }
    outBuf = QByteArray(buf, len);
    outBufPos = 0;
}

/**
manages initial communication setup including password queries
*/
#ifndef Q_OS_WIN
ConnectedResult fishProtocol::establishConnection(char *buffer, KIO::fileoffset_t len)
{
    QString buf = QString::fromLatin1(buffer, len);
#else
ConnectedResult fishProtocol::establishConnection(const QByteArray &buffer)
{
    QString buf = buffer;
#endif
    int pos = 0;
    // Strip trailing whitespace
    while (buf.length() && (buf[buf.length() - 1] == ' '))
        buf.truncate(buf.length() - 1);

    myDebug(<< "establishing: got " << buf);
    while (childPid && ((pos = buf.indexOf('\n')) >= 0 || buf.endsWith(':') || buf.endsWith('?'))) {
        pos++;
        QString str = buf.left(pos);
        buf = buf.mid(pos);
        if (str == "\n")
            continue;
        if (str == "FISH:\n") {
            thisFn.clear();
            infoMessage(i18n("Initiating protocol..."));
            if (!connectionAuth.password.isEmpty()) {
                connectionAuth.password = connectionAuth.password.left(connectionAuth.password.length() - 1);
                if (connectionAuth.keepPassword) {
                    cacheAuthentication(connectionAuth);
                }
            }
            isLoggedIn = true;
            return {0, KIO::WorkerResult::pass()};
        } else if (!str.isEmpty()) {
            thisFn += str;
        } else if (buf.endsWith(':')) {
            if (!redirectUser.isEmpty() && connectionUser != redirectUser) {
                QUrl dest = url;
                dest.setUserName(redirectUser);
                dest.setPassword(redirectPass);
                redirection(dest);
                commandList.clear();
                commandCodes.clear();
                isRunning = false;
                redirectUser = "";
                redirectPass = "";
                return {-1, KIO::WorkerResult::fail()};
            }
            if (!connectionPassword.isEmpty()) {
                myDebug(<< "sending cpass");
                connectionAuth.password = connectionPassword + ENDLINE;
                connectionPassword.clear();
                // su does not like receiving a password directly after sending
                // the password prompt so we wait a while.
                if (local)
                    sleep(1);
                writeChild(connectionAuth.password.toLatin1().constData(), connectionAuth.password.length());
            } else {
                myDebug(<< "sending mpass");
                connectionAuth.prompt = thisFn + buf;
                if (local)
                    connectionAuth.caption = i18n("Local Login");
                else
                    connectionAuth.caption = i18n("SSH Authentication");
                if ((!firstLogin || !checkCachedAuthentication(connectionAuth))) {
                    connectionAuth.password.clear(); // don't prefill
                    int errorCode = openPasswordDialog(connectionAuth);
                    if (errorCode != 0) {
                        shutdownConnection();
                        return {-1, error(errorCode, connectionHost)};
                    }
                }
                firstLogin = false;
                connectionAuth.password += ENDLINE;
                if (connectionAuth.username != connectionUser) {
                    QUrl dest = url;
                    dest.setUserName(connectionAuth.username);
                    dest.setPassword(connectionAuth.password);
                    redirection(dest);
                    if (isStat) { // FIXME: just a workaround for konq deficiencies
                        redirectUser = connectionAuth.username;
                        redirectPass = connectionAuth.password;
                    }
                    commandList.clear();
                    commandCodes.clear();
                    isRunning = false;
                    return {-1, KIO::WorkerResult::fail()};
                }
                myDebug(<< "sending pass");
                if (local)
                    sleep(1);
                writeChild(connectionAuth.password.toLatin1().constData(), connectionAuth.password.length());
            }
            thisFn.clear();
#ifdef Q_OS_WIN
            return {buf.length(), KIO::WorkerResult::pass()};
        }
#else
            return {0, KIO::WorkerResult::pass()};
        } else if (buf.endsWith('?')) {
            int rc = messageBox(QuestionTwoActions, thisFn + buf, QString(), i18nc("@action:button", "Yes"), i18nc("@action:button", "No"));
            if (rc == KIO::WorkerBase::PrimaryAction) {
                writeChild("yes\n", 4);
            } else {
                writeChild("no\n", 3);
            }
            thisFn.clear();
            return {0, KIO::WorkerResult::pass()};
        }
#endif
        else {
            myDebug(<< "unmatched case in initial handling! should not happen!");
        }
#ifdef Q_OS_WIN
        if (buf.endsWith(QLatin1String("(y/n)"))) {
            int rc = messageBox(QuestionTwoActions, thisFn + buf, QString(), i18nc("@action:button", "Yes"), i18nc("@action:button", "No"));
            if (rc == KIO::WorkerBase::PrimaryAction) {
                writeChild("y\n", 2);
            } else {
                writeChild("n\n", 2);
            }
            thisFn.clear();
            return {0, KIO::WorkerResult::pass()};
        }
#endif
    }
    return {buf.length(), KIO::WorkerResult::pass()};
}

void fishProtocol::setHostInternal(const QUrl &u)
{
    int port = u.port();
    if (port <= 0) // no port is -1 in QUrl, but in kde3 we used 0 and the KIO worker assume that.
        port = 0;
    setHost(u.host(), port, u.userName(), u.password());
}

/**
sets connection information for subsequent commands
*/
void fishProtocol::setHost(const QString &host, quint16 port, const QString &u, const QString &pass)
{
    QString user(u);

    local = (host == "localhost" && port == 0);
    if (user.isEmpty())
        user = getenv("LOGNAME");

    if (host == connectionHost && port == connectionPort && user == connectionUser)
        return;
    myDebug(<< "setHost " << u << "@" << host);

    if (childPid)
        shutdownConnection();

    connectionHost = host;
    connectionAuth.url.setHost(host);

    connectionUser = user;
    connectionAuth.username = user;
    connectionAuth.url.setUserName(user);

    connectionPort = port;
    connectionPassword = pass;
    firstLogin = true;
}

/**
Forced close of the connection

This function gets called from the application side of the universe,
it shouldn't send any response.
 */
void fishProtocol::closeConnection()
{
    myDebug(<< "closeConnection()");
    shutdownConnection(true);
}

/**
Closes the connection
 */
void fishProtocol::shutdownConnection(bool forced)
{
    if (childPid) {
#ifdef Q_OS_WIN
        childPid->terminate();
#else
        int killStatus = kill(childPid, SIGTERM); // We may not have permission...
        if (killStatus == 0)
            waitpid(childPid, nullptr, 0);
#endif
        childPid = 0;
#ifndef Q_OS_WIN
        ::close(childFd); // ...in which case this should do the trick
        childFd = -1;
#endif
        if (!forced) {
            infoMessage(i18n("Disconnected."));
        }
    }
    outBufPos = -1;
    outBuf = QByteArray();
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
bool fishProtocol::sendCommand(fish_command_type cmd, ...)
{
    const fish_info &info = fishInfo[cmd];
    myDebug(<< "queuing: cmd=" << cmd << "['" << info.command << "'](" << info.params << "), alt=['" << info.alt << "'], lines=" << info.lines);

    va_list list;
    va_start(list, cmd);
    QString realCmd = info.command;
    QString realAlt = info.alt;
    static const QRegularExpression rx("[][\\\\\n $`#!()*?{}~&<>;'\"%^@|\t]");
    for (int i = 0; i < info.params; i++) {
        QString arg(va_arg(list, const char *));
        int pos = -2;
        while ((pos = arg.indexOf(rx, pos + 2)) >= 0) {
            arg.replace(pos, 0, QString("\\"));
        }
        // myDebug( << "arg " << i << ": " << arg);
        realCmd.append(" ").append(arg);
        realAlt.replace(QRegularExpression(QLatin1Char('%') + QString::number(i + 1)), arg);
    }
    QString s("#");
    s.append(realCmd).append("\n ").append(realAlt).append(" 2>&1;echo '### 200'\n");
    if (realCmd == "FISH")
        s.prepend(" ");
    commandList.append(s);
    commandCodes.append(cmd);
    va_end(list);
    return true;
}

/**
checks response string for result code, converting 000 and 001 appropriately
*/
int fishProtocol::handleResponse(const QString &str)
{
    myDebug(<< "handling: " << str);
    if (str.startsWith(QLatin1String("### "))) {
        bool isOk = false;
        int result = str.mid(4, 3).toInt(&isOk);
        if (!isOk)
            result = 500;
        if (result == 0)
            result = (errorCount != 0 ? 500 : 200);
        if (result == 1)
            result = (errorCount != 0 ? 500 : 100);
        myDebug(<< "result: " << result << ", errorCount: " << errorCount);
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

    static const char *const monthNames[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    for (int i = 0; i < 12; i++)
        if (monthStr.startsWith(monthNames[i])) {
            month = i + 1;
            break;
        }

    int pos = timeyearStr.indexOf(':');
    if (timeyearStr.length() == 4 && pos == -1) {
        year = timeyearStr.toInt();
    } else if (pos == -1) {
        return 0;
    } else {
        if (month > currentMonth + 1)
            year--;
        dt.setTime(QTime(timeyearStr.left(pos).toInt(), timeyearStr.mid(pos + 1).toInt(), 0));
    }
    dt.setDate(QDate(year, month, day));

    return dt.toSecsSinceEpoch();
}

/**
parses response from server and acts accordingly
*/
KIO::WorkerResult fishProtocol::manageConnection(const QString &l)
{
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
                shutdownConnection();
                return error(ERR_UNSUPPORTED_PROTOCOL, line);
            }
            break;
        case FISH_PWD:
            url.setPath(line);
            redirection(url);
            break;
        case FISH_LIST:
            myDebug(<< "listReason: " << static_cast<int>(listReason));
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
                case '9': {
                    long long val = line.toLongLong(&isOk);
                    if (val > 0 && isOk)
                        errorCount--;
                    if ((fishCommand == FISH_LIST) && (listReason == LIST))
                        totalSize(val);
                } break;

                case 'P': {
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
                            myDebug(<< "unknown file type: " << line[1].cell());
                            errorCount++;
                            break;
                        }
                    }
                    // myDebug( << "file type: " << udsType);

                    long long accessVal = 0;
                    if (line[2] == 'r')
                        accessVal |= S_IRUSR;
                    if (line[3] == 'w')
                        accessVal |= S_IWUSR;
                    if (line[4] == 'x' || line[4] == 's')
                        accessVal |= S_IXUSR;
                    if (line[4] == 'S' || line[4] == 's')
                        accessVal |= S_ISUID;
                    if (line[5] == 'r')
                        accessVal |= S_IRGRP;
                    if (line[6] == 'w')
                        accessVal |= S_IWGRP;
                    if (line[7] == 'x' || line[7] == 's')
                        accessVal |= S_IXGRP;
                    if (line[7] == 'S' || line[7] == 's')
                        accessVal |= S_ISGID;
                    if (line[8] == 'r')
                        accessVal |= S_IROTH;
                    if (line[9] == 'w')
                        accessVal |= S_IWOTH;
                    if (line[10] == 'x' || line[10] == 't')
                        accessVal |= S_IXOTH;
                    if (line[10] == 'T' || line[10] == 't')
                        accessVal |= S_ISVTX;
                    udsEntry.replace(KIO::UDSEntry::UDS_ACCESS, accessVal);

                    pos = line.indexOf(':', 12);
                    if (pos < 0) {
                        errorCount++;
                        break;
                    }
                    udsEntry.replace(KIO::UDSEntry::UDS_USER, line.mid(12, pos - 12));
                    udsEntry.replace(KIO::UDSEntry::UDS_GROUP, line.mid(pos + 1));
                } break;

                case 'd':
                    pos = line.indexOf(' ');
                    pos2 = line.indexOf(' ', pos + 1);
                    if (pos < 0 || pos2 < 0)
                        break;
                    errorCount--;
                    udsEntry.replace(KIO::UDSEntry::UDS_MODIFICATION_TIME,
                                     makeTimeFromLs(line.mid(1, pos - 1), line.mid(pos + 1, pos2 - pos), line.mid(pos2 + 1)));
                    break;

                case 'D':
                    pos = line.indexOf(' ');
                    pos2 = line.indexOf(' ', pos + 1);
                    pos3 = line.indexOf(' ', pos2 + 1);
                    if (pos < 0 || pos2 < 0 || pos3 < 0)
                        break;
                    dt.setDate(QDate(line.mid(1, pos - 1).toInt(), line.mid(pos + 1, pos2 - pos - 1).toInt(), line.mid(pos2 + 1, pos3 - pos2 - 1).toInt()));
                    pos = pos3;
                    pos2 = line.indexOf(' ', pos + 1);
                    pos3 = line.indexOf(' ', pos2 + 1);
                    if (pos < 0 || pos2 < 0 || pos3 < 0)
                        break;
                    dt.setTime(QTime(line.mid(pos + 1, pos2 - pos - 1).toInt(), line.mid(pos2 + 1, pos3 - pos2 - 1).toInt(), line.mid(pos3 + 1).toInt()));
                    errorCount--;
                    udsEntry.replace(KIO::UDSEntry::UDS_MODIFICATION_TIME, dt.toSecsSinceEpoch());
                    break;

                case 'S': {
                    long long sizeVal = line.mid(1).toLongLong(&isOk);
                    if (!isOk)
                        break;
                    errorCount--;
                    udsEntry.replace(KIO::UDSEntry::UDS_SIZE, sizeVal);
                } break;

                case 'E':
                    errorCount--;
                    break;

                case ':':
                    pos = line.lastIndexOf('/');
                    thisFn = line.mid(pos < 0 ? 1 : pos + 1);
                    if (fishCommand == FISH_LIST) {
                        udsEntry.replace(KIO::UDSEntry::UDS_NAME, thisFn);
                    }
                    // By default, the mimetype comes from the extension
                    // We'll use the file(1) result only as fallback [like the rest of KDE does]
                    if (udsMime != "inode/directory") {
                        QUrl kurl("fish://host/" + thisFn);
                        QMimeDatabase db;
                        QMimeType mime = db.mimeTypeForUrl(kurl);
                        if (!mime.isDefault())
                            udsMime = mime.name();
                    }
                    errorCount--;
                    break;

                case 'M':
                    // This is getting ugly. file(1) makes some uneducated
                    // guesses, so we must try to ignore them (#51274)
                    if (udsMime.isEmpty() && line.right(8) != "/unknown"
                        && (thisFn.indexOf('.') < 0 || (line.left(8) != "Mtext/x-" && line != "Mtext/plain"))) {
                        udsMime = line.mid(1);
                        if (udsMime == "inode/directory") // a symlink to a dir is a dir
                            udsType = S_IFDIR;
                    }
                    errorCount--;
                    break;

                case 'L':
                    udsEntry.replace(KIO::UDSEntry::UDS_LINK_DEST, line.mid(1));
                    if (!udsType)
                        udsType = S_IFLNK;
                    errorCount--;
                    break;
                }
            } else {
                if (!udsMime.isNull())
                    udsEntry.replace(KIO::UDSEntry::UDS_MIME_TYPE, udsMime);
                udsMime.clear();

                udsEntry.replace(KIO::UDSEntry::UDS_FILE_TYPE, udsType);
                udsType = 0;

                if (fishCommand == FISH_STAT)
                    udsStatEntry = udsEntry;
                else if (listReason == LIST) {
                    listEntry(udsEntry); // 1
                } else if (listReason == CHECK)
                    checkExist = true; // 0
                errorCount--;
                udsEntry.clear();
            }
            break;

        case FISH_RETR:
            if (line.length() == 0) {
                recvLen = 0;
                return error(ERR_IS_DIRECTORY, url.toDisplayString());
            }
            recvLen = line.toLongLong(&isOk);
            if (!isOk) {
                shutdownConnection();
                return error(ERR_CANNOT_READ, url.toDisplayString());
            }
            break;
        default:
            break;
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
            myDebug(<< "reading " << recvLen);
            if (recvLen == -1) {
                shutdownConnection();
                return error(ERR_CANNOT_READ, url.toDisplayString());
            }

            rawRead = recvLen;
            dataRead = 0;
            mimeTypeSent = false;
            if (recvLen == 0) {
                mimeType("application/x-zerosize");
                mimeTypeSent = true;
            }
            break;
        case FISH_STOR:
        case FISH_WRITE:
        case FISH_APPEND:
            rawWrite = sendLen;
            // myDebug( << "sending " << sendLen);
            writeChild(nullptr, 0);
            break;
        default:
            break;
        }
    } else if (rc / 100 != 2) {
        switch (fishCommand) {
        case FISH_STOR:
        case FISH_WRITE:
        case FISH_APPEND:
            shutdownConnection();
            return error(ERR_CANNOT_WRITE, url.toDisplayString());
        case FISH_RETR:
            shutdownConnection();
            return error(ERR_CANNOT_READ, url.toDisplayString());
        case FISH_READ:
            if (rc == 501) {
                mimeType("inode/directory");
                mimeTypeSent = true;
                recvLen = 0;
                finished();
            } else {
                shutdownConnection();
                return error(ERR_CANNOT_READ, url.toDisplayString());
            }
            break;
        case FISH_FISH:
        case FISH_VER:
            shutdownConnection();
            return error(ERR_WORKER_DEFINED, line);
        case FISH_PWD:
        case FISH_CWD:
            return error(ERR_CANNOT_ENTER_DIRECTORY, url.toDisplayString());
        case FISH_LIST:
            myDebug(<< "list error. reason: " << static_cast<int>(listReason));
            if (listReason == LIST)
                return error(ERR_CANNOT_ENTER_DIRECTORY, url.toDisplayString());
            if (listReason == CHECK) {
                checkExist = false;
                finished();
            }
            break;
        case FISH_STAT:
            udsStatEntry.clear();
            return error(ERR_DOES_NOT_EXIST, url.toDisplayString());
        case FISH_CHMOD:
            return error(ERR_CANNOT_CHMOD, url.toDisplayString());
        case FISH_CHOWN:
        case FISH_CHGRP:
            return error(ERR_ACCESS_DENIED, url.toDisplayString());
        case FISH_MKD:
            if (rc == 501)
                return error(ERR_DIR_ALREADY_EXIST, url.toDisplayString());
            return error(ERR_CANNOT_MKDIR, url.toDisplayString());
        case FISH_RMD:
            return error(ERR_CANNOT_RMDIR, url.toDisplayString());
        case FISH_DELE:
            return error(ERR_CANNOT_DELETE, url.toDisplayString());
        case FISH_RENAME:
            return error(ERR_CANNOT_RENAME, url.toDisplayString());
        case FISH_COPY:
        case FISH_LINK:
        case FISH_SYMLINK:
            return error(ERR_CANNOT_WRITE, url.toDisplayString());
        default:
            break;
        }
    } else {
        if (fishCommand == FISH_STOR)
            fishCommand = (hasAppend ? FISH_APPEND : FISH_WRITE);
        if (fishCommand == FISH_LIST) {
            if (listReason == CHECK && !checkOverwrite && checkExist) {
                return error(ERR_FILE_ALREADY_EXIST, url.toDisplayString());
            }
        } else if (fishCommand == FISH_STAT) {
            udsStatEntry.replace(KIO::UDSEntry::UDS_NAME, url.fileName());
            statEntry(udsStatEntry);
        } else if (fishCommand == FISH_APPEND) {
            dataReq();
            if (readData(rawData) > 0)
                sendCommand(FISH_APPEND, E(QString::number(rawData.size())), E(url.path()));
            else if (!checkExist && putPerm > -1)
                sendCommand(FISH_CHMOD, E(QString::number(putPerm, 8)), E(url.path()));
            sendLen = rawData.size();
        } else if (fishCommand == FISH_WRITE) {
            dataReq();
            if (readData(rawData) > 0)
                sendCommand(FISH_WRITE, E(QString::number(putPos)), E(QString::number(rawData.size())), E(url.path()));
            else if (!checkExist && putPerm > -1)
                sendCommand(FISH_CHMOD, E(QString::number(putPerm, 8)), E(url.path()));
            putPos += rawData.size();
            sendLen = rawData.size();
        } else if (fishCommand == FISH_RETR) {
            data(QByteArray());
        }
        finished();
    }
    return KIO::WorkerResult::pass();
}

void fishProtocol::writeStdin(const QString &line)
{
    qlist.append(E(line));

    if (writeReady) {
        writeReady = false;
        // myDebug( << "Writing: " << qlist.first().mid(0,qlist.first().indexOf('\n')));
        myDebug(<< "Writing: " << qlist.first());
        myDebug(<< "---------");
        writeChild((const char *)qlist.first().constData(), qlist.first().length());
    }
}

void fishProtocol::sent()
{
    if (rawWrite > 0) {
        myDebug(<< "writing raw: " << rawData.size() << "/" << rawWrite);
        writeChild(rawData.data(), (rawWrite > rawData.size() ? rawData.size() : rawWrite));
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
        writeChild("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n", 15);
        rawWrite = -1;
        return;
    }
    if (qlist.count() > 0)
        qlist.erase(qlist.begin());
    if (qlist.count() == 0) {
        writeReady = true;
    } else {
        // myDebug( << "Writing: " << qlist.first().mid(0,qlist.first().indexOf('\n')));
        myDebug(<< "Writing: " << qlist.first());
        myDebug(<< "---------");
        writeChild((const char *)qlist.first().constData(), qlist.first().length());
    }
}

ReceivedResult fishProtocol::received(const char *buffer, KIO::fileoffset_t buflen)
{
    int pos = 0;
    do {
        if (buflen <= 0)
            break;

        if (rawRead > 0) {
            myDebug(<< "processedSize " << dataRead << ", len " << buflen << "/" << rawRead);
            int dataSize = (rawRead > buflen ? buflen : rawRead);
            if (!mimeTypeSent) {
                int mimeSize = qMin(dataSize, (int)(mimeBuffer.size() - dataRead));
                memcpy(mimeBuffer.data() + dataRead, buffer, mimeSize);
                dataRead += mimeSize;
                rawRead -= mimeSize;
                buffer += mimeSize;
                buflen -= mimeSize;
                if (rawRead == 0) // End of data
                    mimeBuffer.resize(dataRead);
                if (dataRead < (int)mimeBuffer.size()) {
                    myDebug(<< "wait for more");
                    break;
                }

                QMimeDatabase db;
                sendmimeType(db.mimeTypeForFileNameAndData(url.path(), mimeBuffer).name());
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

            QByteArray bdata(buffer, dataSize);
            data(bdata);

            dataRead += dataSize;
            rawRead -= dataSize;
            processedSize(dataRead);
            if (rawRead <= 0) {
                buffer += dataSize;
                buflen -= dataSize;
            } else {
                return {0, KIO::WorkerResult::pass()};
                ;
            }
        }

        if (buflen <= 0)
            break;

        pos = 0;
        // Find newline
        while ((pos < buflen) && (buffer[pos] != '\n'))
            ++pos;

        if (pos < buflen) {
            QString s = remoteEncoding()->decode(QByteArray(buffer, pos));

            buffer += pos + 1;
            buflen -= pos + 1;

            const auto result = manageConnection(s);
            if (!result.success()) {
                return {buflen, result};
            }

            pos = 0;
            // Find next newline
            while ((pos < buflen) && (buffer[pos] != '\n'))
                ++pos;
        }
    } while (childPid && buflen && (rawRead > 0 || pos < buflen));
    return {buflen, KIO::WorkerResult::pass()};
}
/** get a file */
KIO::WorkerResult fishProtocol::get(const QUrl &u)
{
    myDebug(<< "@@@@@@@@@ get " << u);
    setHostInternal(u);
    url = u;
    if (const auto result = openConnection(); !result.success()) {
        return result;
    }
    url = url.adjusted(QUrl::NormalizePathSegments);
    if (url.path().isEmpty()) {
        sendCommand(FISH_PWD);
    } else {
        recvLen = -1;
        sendCommand(FISH_RETR, E(url.path()));
    }
    return run();
}

/** put a file */
KIO::WorkerResult fishProtocol::put(const QUrl &u, int permissions, KIO::JobFlags flags)
{
    myDebug(<< "@@@@@@@@@ put " << u << " " << permissions << " " << (flags & KIO::Overwrite) << " " /* << resume */);
    setHostInternal(u);

    url = u;
    if (const auto result = openConnection(); !result.success()) {
        return result;
    }
    url = url.adjusted(QUrl::NormalizePathSegments);
    if (url.path().isEmpty()) {
        sendCommand(FISH_PWD);
    } else {
        putPerm = permissions;

        checkOverwrite = flags & KIO::Overwrite;
        checkExist = false;
        putPos = 0;
        listReason = CHECK;
        sendCommand(FISH_LIST, E(url.path()));
        sendCommand(FISH_STOR, "0", E(url.path()));

        const QString mtimeStr = metaData("modified");
        if (!mtimeStr.isEmpty()) {
            QDateTime dt = QDateTime::fromString(mtimeStr, Qt::ISODate);
            // TODO set modification time on url.path() somehow
            // see FileProtocol::put if using utime() to do that.
        }
    }
    return run();
}
/** executes next command in sequence or set isRunning false if all is done */
void fishProtocol::finished()
{
    if (commandList.count() > 0) {
        fishCommand = (fish_command_type)commandCodes.first();
        errorCount = -fishInfo[fishCommand].lines;
        rawRead = 0;
        rawWrite = -1;
        udsEntry.clear();
        udsStatEntry.clear();
        writeStdin(commandList.first());
        // if (fishCommand != FISH_APPEND && fishCommand != FISH_WRITE) infoMessage("Sending
        // "+(commandList.first().mid(1,commandList.first().indexOf("\n")-1))+"...");
        commandList.erase(commandList.begin());
        commandCodes.erase(commandCodes.begin());
    } else {
        isRunning = false;
    }
}

/** aborts command sequence and calls error() */
KIO::WorkerResult fishProtocol::error(int type, const QString &detail)
{
    commandList.clear();
    commandCodes.clear();
    myDebug(<< "ERROR: " << type << " - " << detail);
    isRunning = false;
    return KIO::WorkerResult::fail(type, detail);
}

/** executes a chain of commands */
KIO::WorkerResult fishProtocol::run()
/* This function writes to childFd fish commands (like #STOR 0 /tmp/test ...) that are stored in outBuf
and reads from childFd the remote host's response. ChildFd is the fd to a process that communicates
with .fishsrv.pl typically running on another computer. */
{
    if (isRunning) {
        // should have not been running, something is wrong
        return KIO::WorkerResult::fail();
    }

    int rc;
    isRunning = true;
    finished();
#ifndef Q_OS_WIN
    fd_set rfds, wfds;
    FD_ZERO(&rfds);
#endif
    char buf[32768];
    int offset = 0;
    while (isRunning) {
#ifndef Q_OS_WIN
        FD_SET(childFd, &rfds);
        FD_ZERO(&wfds);
        if (outBufPos >= 0)
            FD_SET(childFd, &wfds);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
        rc = select(childFd + 1, &rfds, &wfds, nullptr, &timeout);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            myDebug(<< "select failed, rc: " << rc << ", error: " << strerror(errno));
            shutdownConnection();
            return error(ERR_CONNECTION_BROKEN, connectionHost);
        }
        // We first write the complete buffer, including all newlines.
        // Do: send command and newlines, expect response then
        // Do not: send commands, expect response, send newlines, expect response on newlines
        // Newlines do not trigger a response.
        if (FD_ISSET(childFd, &wfds) && outBufPos >= 0) {
            if (outBuf.length() - outBufPos > 0) {
                rc = ::write(childFd, outBuf.constData() + outBufPos, outBuf.length() - outBufPos);
            }
#else
        if (outBufPos >= 0) {
            if (outBuf.length() - outBufPos > 0) {
                rc = childPid->write(outBuf);
            }
#endif
            else
                rc = 0;

            if (rc >= 0)
                outBufPos += rc;
            else {
#ifndef Q_OS_WIN
                if (errno == EINTR)
                    continue;
                myDebug(<< "write failed, rc: " << rc << ", error: " << strerror(errno));
#else
                myDebug(<< "write failed, rc: " << rc);
#endif
                shutdownConnection();
                return error(ERR_CONNECTION_BROKEN, connectionHost);
            }
            if (outBufPos >= outBuf.length()) {
                outBufPos = -1;
                outBuf = QByteArray();
                sent();
            }
        }
#ifndef Q_OS_WIN
        else if (FD_ISSET(childFd, &rfds)) {
            rc = ::read(childFd, buf + offset, sizeof(buf) - offset);
#else
        else if (childPid->waitForReadyRead(1000)) {
            rc = childPid->read(buf + offset, sizeof(buf) - offset);
#endif
            // myDebug( << "read " << rc << " bytes");
            if (rc <= 0) {
#ifndef Q_OS_WIN
                if (errno == EINTR)
                    continue;
                myDebug(<< "read failed, rc: " << rc << ", error: " << strerror(errno));
#else
                myDebug(<< "read failed, rc: " << rc);
#endif
                shutdownConnection();
                return error(ERR_CONNECTION_BROKEN, connectionHost);
            }

            const auto receivedResult = received(buf, rc + offset);
            if (!receivedResult.result.success()) {
                return receivedResult.result;
            }
            const int noff = receivedResult.remainingBufferSize;
            if (noff > 0)
                memmove(buf, buf + offset + rc - noff, noff);
            // myDebug( << "left " << noff << " bytes: " << QString::fromLatin1(buf,offset));
            offset = noff;
        }
        if (wasKilled()) {
            return KIO::WorkerResult::fail();
        }
    }

    return KIO::WorkerResult::pass();
}

/** stat a file */
KIO::WorkerResult fishProtocol::stat(const QUrl &u)
{
    myDebug(<< "@@@@@@@@@ stat " << u);
    setHostInternal(u);
    url = u;
    isStat = true; // FIXME: just a workaround for konq deficiencies
    const auto openConnectionResult = openConnection();
    isStat = false; // FIXME: just a workaround for konq deficiencies
    if (!openConnectionResult.success()) {
        return openConnectionResult;
    }
    url = url.adjusted(QUrl::NormalizePathSegments);
    if (url.path().isEmpty()) {
        sendCommand(FISH_PWD);
    } else {
        sendCommand(FISH_STAT, E(url.adjusted(QUrl::StripTrailingSlash).path()));
    }
    return run();
}
/** find mimetype for a file */
KIO::WorkerResult fishProtocol::mimetype(const QUrl &u)
{
    myDebug(<< "@@@@@@@@@ mimetype " << u);
    setHostInternal(u);
    url = u;
    if (const auto result = openConnection(); !result.success()) {
        return result;
    }
    url = url.adjusted(QUrl::NormalizePathSegments);
    if (url.path().isEmpty()) {
        sendCommand(FISH_PWD);
    } else {
        recvLen = 1024;
        sendCommand(FISH_READ, "0", "1024", E(url.path()));
    }
    return run();
}
/** list a directory */
KIO::WorkerResult fishProtocol::listDir(const QUrl &u)
{
    myDebug(<< "@@@@@@@@@ listDir " << u);
    setHostInternal(u);
    url = u;
    if (const auto result = openConnection(); !result.success()) {
        return result;
    }
    url = url.adjusted(QUrl::NormalizePathSegments);
    if (url.path().isEmpty()) {
        sendCommand(FISH_PWD);
    } else {
        listReason = LIST;
        sendCommand(FISH_LIST, E(url.path()));
    }
    return run();
}
/** create a directory */
KIO::WorkerResult fishProtocol::mkdir(const QUrl &u, int permissions)
{
    myDebug(<< "@@@@@@@@@ mkdir " << u << " " << permissions);
    setHostInternal(u);
    url = u;
    if (const auto result = openConnection(); !result.success()) {
        return result;
    }
    url = url.adjusted(QUrl::NormalizePathSegments);
    if (url.path().isEmpty()) {
        sendCommand(FISH_PWD);
    } else {
        sendCommand(FISH_MKD, E(url.path()));
        if (permissions > -1)
            sendCommand(FISH_CHMOD, E(QString::number(permissions, 8)), E(url.path()));
    }
    return run();
}
/** rename a file */
KIO::WorkerResult fishProtocol::rename(const QUrl &s, const QUrl &d, KIO::JobFlags flags)
{
    myDebug(<< "@@@@@@@@@ rename " << s << " " << d << " " << (flags & KIO::Overwrite));
    if (s.host() != d.host() || s.port() != d.port() || s.userName() != d.userName()) {
        return error(ERR_UNSUPPORTED_ACTION, s.toDisplayString());
    }
    setHostInternal(s);
    url = d;
    if (const auto result = openConnection(); !result.success()) {
        return result;
    }
    QUrl src = s;
    url = url.adjusted(QUrl::NormalizePathSegments);
    src = src.adjusted(QUrl::NormalizePathSegments);
    if (url.path().isEmpty()) {
        sendCommand(FISH_PWD);
    } else {
        if (!(flags & KIO::Overwrite)) {
            listReason = CHECK;
            checkOverwrite = false;
            sendCommand(FISH_LIST, E(url.path()));
        }
        sendCommand(FISH_RENAME, E(src.path()), E(url.path()));
    }
    return run();
}
/** create a symlink */
KIO::WorkerResult fishProtocol::symlink(const QString &target, const QUrl &u, KIO::JobFlags flags)
{
    myDebug(<< "@@@@@@@@@ symlink " << target << " " << u << " " << (flags & KIO::Overwrite));
    setHostInternal(u);
    url = u;
    if (const auto result = openConnection(); !result.success()) {
        return result;
    }
    url = url.adjusted(QUrl::NormalizePathSegments);
    if (url.path().isEmpty()) {
        sendCommand(FISH_PWD);
    } else {
        if (!(flags & KIO::Overwrite)) {
            listReason = CHECK;
            checkOverwrite = false;
            sendCommand(FISH_LIST, E(url.path()));
        }
        sendCommand(FISH_SYMLINK, E(target), E(url.path()));
    }
    return run();
}
/** change file permissions */
KIO::WorkerResult fishProtocol::chmod(const QUrl &u, int permissions)
{
    myDebug(<< "@@@@@@@@@ chmod " << u << " " << permissions);
    setHostInternal(u);
    url = u;
    if (const auto result = openConnection(); !result.success()) {
        return result;
    }
    url = url.adjusted(QUrl::NormalizePathSegments);
    if (url.path().isEmpty()) {
        sendCommand(FISH_PWD);
    } else {
        // TODO: no error?
        if (permissions < 0) {
            return KIO::WorkerResult::pass();
        }
        sendCommand(FISH_CHMOD, E(QString::number(permissions, 8)), E(url.path()));
    }
    return run();
}
/** copies a file */
KIO::WorkerResult fishProtocol::copy(const QUrl &s, const QUrl &d, int permissions, KIO::JobFlags flags)
{
    myDebug(<< "@@@@@@@@@ copy " << s << " " << d << " " << permissions << " " << (flags & KIO::Overwrite));
    if (s.host() != d.host() || s.port() != d.port() || s.userName() != d.userName()) {
        return error(ERR_UNSUPPORTED_ACTION, s.toDisplayString());
    }
    // myDebug( << s << endl << d);
    setHostInternal(s);
    url = d;
    if (const auto result = openConnection(); !result.success()) {
        return result;
    }
    QUrl src = s;
    url = url.adjusted(QUrl::NormalizePathSegments);
    src = src.adjusted(QUrl::NormalizePathSegments);
    if (src.path().isEmpty()) {
        sendCommand(FISH_PWD);
    } else {
        if (!(flags & KIO::Overwrite)) {
            listReason = CHECK;
            checkOverwrite = false;
            sendCommand(FISH_LIST, E(url.path()));
        }
        sendCommand(FISH_COPY, E(src.path()), E(url.path()));
        if (permissions > -1)
            sendCommand(FISH_CHMOD, E(QString::number(permissions, 8)), E(url.path()));
    }
    return run();
}
/** removes a file or directory */
KIO::WorkerResult fishProtocol::del(const QUrl &u, bool isFile)
{
    myDebug(<< "@@@@@@@@@ del " << u << " " << isFile);
    setHostInternal(u);
    url = u;
    if (const auto result = openConnection(); !result.success()) {
        return result;
    }
    url = url.adjusted(QUrl::NormalizePathSegments);
    if (url.path().isEmpty()) {
        sendCommand(FISH_PWD);
    } else {
        sendCommand((isFile ? FISH_DELE : FISH_RMD), E(url.path()));
    }
    return run();
}
/** special like background execute */
KIO::WorkerResult fishProtocol::special(const QByteArray &data)
{
    int tmp;

    QDataStream stream(data);

    stream >> tmp;
    switch (tmp) {
    case FISH_EXEC_CMD: // SSH EXEC
    {
        QUrl u;
        QString command;
        stream >> u;
        stream >> command;
        myDebug(<< "@@@@@@@@@ exec " << u << " " << command);
        setHostInternal(u);
        url = u;
        if (const auto result = openConnection(); !result.success()) {
            return result;
        }
        sendCommand(FISH_EXEC, E(command), E(url.path()));
        return run();
    }
    default:
        // Some command we don't understand.
        return error(ERR_UNSUPPORTED_ACTION, QString().setNum(tmp));
    }
}
/** report status */
void fishProtocol::worker_status()
{
    myDebug(<< "@@@@@@@@@ worker_status");
    if (childPid > 0)
        workerStatus(connectionHost, isLoggedIn);
    else
        workerStatus(QString(), false);
}

#include "fish.moc"
