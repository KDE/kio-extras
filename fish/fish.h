/*
    fish.h  -  a FISH kioslave
    SPDX-FileCopyrightText: 2001 Jörg Walter <trouble@garni.ch>
    SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef FISH_H
#define FISH_H

#include <kio/global.h>
#include <kio/slavebase.h>
#include <kio/authinfo.h>

#define FISH_EXEC_CMD 'X'

class fishProtocol : public KIO::SlaveBase
{
public:
    fishProtocol(const QByteArray &pool_socket, const QByteArray &app_socket);
    ~fishProtocol() override;

    /**
    Connects to a server and logs us in via SSH. Then starts FISH protocol.
    @ref isConnected is set to true if logging on was successful.
    It is set to false if the connection becomes closed.

    */
    void openConnection() override;

    /**
     Clean up connection
    */
    void shutdownConnection(bool forced=false);
    /** sets connection information for subsequent commands */
    void setHost(const QString & host, quint16 port, const QString & user, const QString & pass) override;
    /** Forced close of the connection */
    void closeConnection() override;
    /** get a file */
    void get(const QUrl& url) override;
    /** put a file */
    void put(const QUrl& url, int permissions, KIO::JobFlags flags ) override;
    /** aborts command sequence and calls error() */
    void error(int type, const QString &detail);
    /** executes next command in sequence or calls finished() if all is done */
    void finished();
    /** stat a file */
    void stat(const QUrl& url) override;
    /** find mimetype for a file */
    void mimetype(const QUrl& url) override;
    /** list a directory */
    void listDir(const QUrl& url) override;
    /** create a directory */
    void mkdir(const QUrl&url, int permissions) override;
    /** rename a file */
    void rename(const QUrl& src, const QUrl& dest, KIO::JobFlags flags) override;
    /** create a symlink */
    void symlink(const QString& target, const QUrl& dest, KIO::JobFlags flags) override;
    /** change file permissions */
    void chmod(const QUrl& url, int permissions) override;
    /** copies a file */
    void copy(const QUrl &src, const QUrl &dest, int permissions, KIO::JobFlags flags) override;
    /** report status */
    void slave_status() override;
    /** removes a file or directory */
    void del(const QUrl &u, bool isfile) override;
    /** special like background execute */
    void special( const QByteArray &data ) override;

private: // Private attributes
    /** fd for reading and writing to the process */
    int childFd;
    /** buffer for data to be written */
#ifndef Q_OS_WIN
    const char *outBuf;
#else
    QByteArray outBuf;
#endif
    /** current write position in buffer */
    KIO::fileoffset_t outBufPos;
    /** length of buffer */
    KIO::fileoffset_t outBufLen;
    /** use su if true else use ssh */
    bool local;
    /**  // FIXME: just a workaround for konq deficiencies */
    bool isStat;
    /**  // FIXME: just a workaround for konq deficiencies */
    QString redirectUser, redirectPass;

protected: // Protected attributes
    /** for LIST/STAT */
    KIO::UDSEntry udsEntry;
    /** for LIST/STAT */
    KIO::UDSEntry udsStatEntry;
    /** for LIST/STAT */
    long long udsType;
    /** for LIST/STAT */
    QString udsMime;
    /** for LIST/STAT */
    QString thisFn;
    /** for STAT */
    QString wantedFn;
    QString statPath;
    /** url of current request */
    QUrl url;
    /** true if connection is logged in successfully */
    bool isLoggedIn;
    /** host name of current connection */
    QString connectionHost;
    /** user name of current connection */
    QString connectionUser;
    /** port of current connection */
    int connectionPort;
    /** password of current connection */
    QString connectionPassword;
    /** AuthInfo object used for logging in */
    KIO::AuthInfo connectionAuth;
    /** number of lines received, == 0 -> everything went ok */
    int errorCount;
    /** queue for lines to be sent */
    QList<QByteArray> qlist;
    /** queue for commands to be sent */
    QStringList commandList;
    /** queue for commands to be sent */
    QList<int> commandCodes;
    /** bytes still to be read in raw mode */
    KIO::fileoffset_t rawRead;
    /** bytes still to be written in raw mode */
    KIO::fileoffset_t rawWrite;
    /** data bytes to read in next read command */
    KIO::fileoffset_t recvLen;
    /** data bytes to write in next write command */
    KIO::fileoffset_t sendLen;
    /** true if the last write operation was finished */
    bool writeReady;
    /** true if a command stack is currently executing */
    bool isRunning;
    /** reason of LIST command */
    enum { CHECK, LIST } listReason;
    /** true if FISH server understands APPEND command */
    bool hasAppend;
    /** permission of created file */
    int putPerm;
    /** true if file may be overwritten */
    bool checkOverwrite;
    /** current position of write */
    KIO::fileoffset_t putPos;
    /** true if file already existed */
    bool checkExist;
    /** true if this is the first login attempt (== use cached password) */
    bool firstLogin;
    /** write buffer */
    QByteArray rawData;
    /** buffer for storing bytes used for MimeMagic */
    QByteArray mimeBuffer;
    /** whther the mimetype has been sent already */
    bool mimeTypeSent;
    /** number of bytes read so far */
    KIO::fileoffset_t dataRead;
    /** details about each fishCommand */
    static const struct fish_info {
        const char *command;
        int params;
        const char *alt;
        int lines;
    } fishInfo[];
    /** last FISH command sent to server */
    enum fish_command_type { FISH_FISH, FISH_VER, FISH_PWD, FISH_LIST, FISH_STAT,
                             FISH_RETR, FISH_STOR,
                             FISH_CWD, FISH_CHMOD, FISH_DELE, FISH_MKD, FISH_RMD,
                             FISH_RENAME, FISH_LINK, FISH_SYMLINK, FISH_CHOWN,
                             FISH_CHGRP, FISH_READ, FISH_WRITE, FISH_COPY, FISH_APPEND, FISH_EXEC
                           } fishCommand;
    int fishCodeLen;
protected: // Protected methods
    /** manages initial communication setup including password queries */
#ifndef Q_OS_WIN
    int establishConnection(char *buffer, KIO::fileoffset_t buflen);
#else
    int establishConnection(const QByteArray &buffer);
#endif
    int received(const char *buffer, KIO::fileoffset_t buflen);
    void sent();
    /** builds each FISH request and sets the error counter */
    bool sendCommand(fish_command_type cmd, ...);
    /** checks response string for result code, converting 000 and 001 appropriately */
    int handleResponse(const QString &str);
    /** parses a ls -l time spec */
    int makeTimeFromLs(const QString &dayStr, const QString &monthStr, const QString &timeyearStr);
    /** executes a chain of commands */
    void run();
    /** creates the subprocess */
    bool connectionStart();
    /** writes one chunk of data to stdin of child process */
#ifndef Q_OS_WIN
    void writeChild(const char *buf, KIO::fileoffset_t len);
#else
    void writeChild(const QByteArray &buf, KIO::fileoffset_t len);
#endif
    /** parses response from server and acts accordingly */
    void manageConnection(const QString &line);
    /** writes to process */
    void writeStdin(const QString &line);
    /** Verify port **/
    void setHostInternal(const QUrl & u);

};


#endif
