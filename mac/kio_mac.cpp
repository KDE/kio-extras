/***************************************************************************
                          kio_mac.cpp
                             -------------------
    copyright            : (C) 2002 Jonathan Riddell
    email                : jr@jriddell.org
    version              : 1.0.1
    release date         : 19 July 2002
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define PARTITION "/dev/hda11"

#include <kinstance.h>
#include <kdebug.h>
#include <klocale.h>
#include <kregexp.h>
#include <kconfig.h>
#include <qstring.h>
#include <qregexp.h>
#include <qdatastream.h>

#include <sys/stat.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>

#include "kio_mac.moc"

using namespace KIO;

extern "C" {
    int kdemain(int, char **argv) {
        KInstance instance("kio_mac");
        MacProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();
        return 0;
    }
}

MacProtocol::MacProtocol(const QCString &pool, const QCString &app)
                                             : QObject(), SlaveBase("mac", pool, app) {
/*  logFile = new QFile("/home/jr/logfile");
    logFile->open(IO_ReadWrite | IO_Append);
    logStream = new QTextStream(logFile);
    *logStream << "Start Macprotocol()" << endl;
    */
}

MacProtocol::~MacProtocol() {
/*    *logStream << "destructor ~MacProtocol()" << endl;
    logFile->close();
    delete logFile;
    logFile = 0;
    delete logStream;
    logStream = 0;
*/
    delete myKProcess;
    myKProcess = 0L;
}

//get() called when a file is to be read
void MacProtocol::get(const KURL& url) {
    QString path = prepareHP(url);  //mount and change to correct directory - return the filename
    QString query = url.query();
    QString mode("-");
    QString mime = "";
    processedBytes = 0;

    //Find out the size and if it's a text file
    UDSEntry entry = doStat(url);
    UDSEntry::Iterator it;
    for(it = entry.begin(); it != entry.end(); ++it) {
        if ((*it).m_uds == KIO::UDS_MIME_TYPE) {
            mime = (*it).m_str;
        }
        if ((*it).m_uds == KIO::UDS_SIZE) {
            totalSize((*it).m_long);
        }
    }

    //find out if a mode has been specified in the query e.g. ?mode=t
    //or if it's a text file then set the mode to text
    int modepos = query.find("mode=");
    int textpos = mime.find("text");
    if (modepos != -1) {
        mode += query.mid(modepos + 5, 1);
        if (mode != "-r" && mode != "-b" && mode != "-m" && mode != "-t" && mode != "-a") {
            error(ERR_MALFORMED_URL, i18n("Unknown mode"));
        }
    } else if (textpos != -1) {
        mode += "t";
    } else {
        mode += "r";
    }

    //now we can read the file
    myKProcess = new KProcess();

    *myKProcess << "hpcopy" << mode << path << "-";

    //data is now sent directly from the slot
    connect(myKProcess, SIGNAL(receivedStdout(KProcess *, char *, int)),
            this, SLOT(slotSetDataStdOutput(KProcess *, char *, int)));

    myKProcess->start(KProcess::Block, KProcess::All);

    if (!myKProcess->normalExit() || !(myKProcess->exitStatus() == 0)) {
        error(ERR_CANNOT_LAUNCH_PROCESS,
              i18n("There was an error with hpcopy - please ensure it is installed"));
        return;
    }

    //clean up
    disconnect(myKProcess, SIGNAL(receivedStdout(KProcess *, char *, int)),
            this, SLOT(slotSetDataStdOutput(KProcess *, char *, int)));
    delete myKProcess; myKProcess = 0;
    //finish
    data(QByteArray());
    finished();
}

//listDir() called when the user is looking at a directory
void MacProtocol::listDir(const KURL& url) {
    QString filename = prepareHP(url);

    if (filename.isNull()) {
        error(ERR_CANNOT_LAUNCH_PROCESS, i18n("No filename was found"));
    } else {
        myKProcess = new KProcess();
        *myKProcess << "hpls" << "-la" << filename;

        standardOutputStream = QString::null;
        connect(myKProcess, SIGNAL(receivedStdout(KProcess *, char *, int)),
                this, SLOT(slotGetStdOutput(KProcess *, char *, int)));

        myKProcess->start(KProcess::Block, KProcess::All);

        if ((!myKProcess->normalExit()) || (!myKProcess->exitStatus() == 0)) {
            error(ERR_CANNOT_LAUNCH_PROCESS,
                  i18n("There was an error with hpls - please ensure it is installed"));
        }

        //clean up
        delete myKProcess; myKProcess = 0;
        disconnect(myKProcess, SIGNAL(receivedStdout(KProcess *, char *, int)),
                this, SLOT(slotGetStdOutput(KProcess *, char *, int)));

        UDSEntry entry;
        if (!standardOutputStream.isEmpty()) {
            QTextStream in(&standardOutputStream, IO_ReadOnly);
            QString line = in.readLine(); //throw away top file which shows current directory
            line = in.readLine();

            while (line != NULL) {
                //1.0.4 puts this funny line in sometimes, we don't want it
                if (line.contains("Thread               ") == 0) {
                    entry = makeUDS(line);
                    listEntry(entry, false);
                }
                line = in.readLine();
            }
        }//if standardOutputStream != null

        listEntry(entry, true);
        finished();

    }//if filename == null
}

//stat() called to see if it's a file or directory, called before listDir() or get()
void MacProtocol::stat(const KURL& url) {
    statEntry(doStat(url));
    finished();
}

//doStat(), does all the work that stat() needs
//it's been separated out so it can be called from get() which
//also need information
QValueList<KIO::UDSAtom> MacProtocol::doStat(const KURL& url) {
    QString filename = prepareHP(url);

    if (filename.isNull()) {
        error(ERR_DOES_NOT_EXIST, i18n("No filename was found in the URL"));
    } else if (! filename.isEmpty()) {
        myKProcess = new KShellProcess();

        *myKProcess << "hpls" << "-ld" << filename;

        standardOutputStream = QString::null;
        connect(myKProcess, SIGNAL(receivedStdout(KProcess *, char *, int)),
                this, SLOT(slotGetStdOutput(KProcess *, char *, int)));

        myKProcess->start(KProcess::Block, KProcess::All);

        if ((!myKProcess->normalExit()) || (!myKProcess->exitStatus() == 0)) {
            error(ERR_CANNOT_LAUNCH_PROCESS,
                  i18n("hpls did not exit normally - please ensure you have installed the hfsplus tools"));
        }

        //clean up
        delete myKProcess; myKProcess = 0;
        disconnect(myKProcess, SIGNAL(receivedStdout(KProcess *, char *, int)),
                this, SLOT(slotGetStdOutput(KProcess *, char *, int)));

        if (standardOutputStream.isEmpty()) {
            filename.replace("\\ ", " "); //get rid of escapes
            filename.replace("\\&", "&"); //mm, slashes...
            filename.replace("\\!", "!");
            filename.replace("\\(", "(");
            filename.replace("\\)", ")");
            error(ERR_DOES_NOT_EXIST, filename);
        } else {
            //remove trailing \n
            QString line = standardOutputStream.left(standardOutputStream.length()-1);
            UDSEntry entry = makeUDS(line);
            return entry;
        }
    } else {     //filename is empty means we're looking at root dir
                 //we don't have a listing for the root directory so here's a dummy one
            UDSEntry entry = makeUDS("d         0 item               Jan 01  2000 /");
            return entry;
    }//if filename == null

    return QValueList<KIO::UDSAtom>();
}

//prepareHP() called from get() listDir() and stat()
//(re)mounts the partition and changes to the appropriate directory
QString MacProtocol::prepareHP(const KURL& url) {
    QString path = url.path(-1);
    if (path.left(1) == "/") {
        path = path.mid(1); // strip leading slash
    }

    if (path == NULL) {
        path = "";
    }

    //find out if a device has been specified in the query e.g. ?dev=/dev/fd0
    //or in the config file (query device entries are saved to config file)
    QString device;
    KConfig* config = new KConfig("macrc");

    QString query = url.query();
    int modepos = query.find("dev=");
    if (modepos == -1) {
        //no device specified, read from config or go with #define PARTITION
        device = config->readEntry("device",PARTITION);
    } else {
        //TODO this means dev=foo must be the last argument in the query
        device = query.mid(modepos + 4);
        config->writeEntry("device",device);
    }
    delete config; config = 0;

    //first we run just hpmount and check the output to see if it's version 1.0.2 or 1.0.4
    myKProcess = new KProcess();
    *myKProcess << "hpmount";
    standardOutputStream = QString::null;
    connect(myKProcess, SIGNAL(receivedStderr(KProcess *, char *, int)),
            this, SLOT(slotGetStdOutput(KProcess *, char *, int)));

    myKProcess->start(KProcess::Block, KProcess::All);

    bool version102 = true;

    if (standardOutputStream.contains("options") != 0) {
        version102 = false;
    }

    delete myKProcess; myKProcess = 0;
    disconnect(myKProcess, SIGNAL(receivedStderr(KProcess *, char *, int)),
            this, SLOT(slotGetStdOutput(KProcess *, char *, int)));

    //now mount the drive
    myKProcess = new KProcess();
    if (version102) {
        *myKProcess << "hpmount" << device;
    } else {
        *myKProcess << "hpmount" << "-r" << device;
    }

    myKProcess->start(KProcess::Block, KProcess::All);

    if ((!myKProcess->normalExit()) || (!myKProcess->exitStatus() == 0)) {
        //TODO this error interrupts the user when typing ?dev=foo on each letter of foo
        error(ERR_CANNOT_LAUNCH_PROCESS,
              i18n("hpmount did not exit normally - please ensure that hfsplus utils are installed,\n"
                   "that you have permission to read the partition (ls -l /dev/hdaX)\n"
                   "and that you have specified the correct partition.\n"
                   "You can specify partitions by adding ?dev=/dev/hda2 to the URL."));
        return NULL;
    }

    //clean up
    delete myKProcess; myKProcess = 0;

    //escape any funny characters
    //TODO are there any more characters to escape?
    path.replace(QRegExp(" "), "\\ ");
    path.replace(QRegExp("&"), "\\&");
    path.replace(QRegExp("!"), "\\!");
    path.replace(QRegExp("("), "\\(");
    path.replace(QRegExp(")"), "\\)");

    //then change to the right directory
    int s;  QString dir;
    s = path.find('/');
    while (s != -1) {
        dir = path.left(s);
        path = path.mid(s+1);

        myKProcess = new KProcess();
        *myKProcess << "hpcd" << dir;

        myKProcess->start(KProcess::Block, KProcess::All);

        if ((!myKProcess->normalExit()) || (!myKProcess->exitStatus() == 0)) {
            error(ERR_CANNOT_LAUNCH_PROCESS,
                  i18n("hpcd did not exit normally - please ensure it is installed"));
            return NULL;
        }

        //clean up
        delete myKProcess; myKProcess = 0;

        s = path.find('/');
    }

    return path;
}

//makeUDS()  takes a line of output from hpls -l and converts it into
// one of these UDSEntrys to return
//called from listDir() and stat()
QValueList<KIO::UDSAtom> MacProtocol::makeUDS(const QString& _line) {
    QString line(_line);
    UDSEntry entry;

    //is it a file or a directory
    KRegExp dirRE("^d. +([^ ]+) +([^ ]+) +([^ ]+) +([^ ]+) +([^ ]+) +(.*)");
    KRegExp fileRE("^([f|F]). +(....)/(....) +([^ ]+) +([^ ]+) +([^ ]+) +([^ ]+) +([^ ]+) +(.*)");
    if (dirRE.match(line.latin1())) {
        UDSAtom atom;
        atom.m_uds = KIO::UDS_NAME;
        atom.m_str = dirRE.group(6);
        entry.append(atom);

        atom.m_uds = KIO::UDS_MODIFICATION_TIME;
        atom.m_long = makeTime(dirRE.group(4), dirRE.group(3), dirRE.group(5));
        entry.append(atom);

        atom.m_uds = KIO::UDS_FILE_TYPE;
        atom.m_long = S_IFDIR;
        entry.append(atom);

        atom.m_uds = KIO::UDS_ACCESS;
        atom.m_long = 0755;
        entry.append(atom);

    } else if (fileRE.match(line.latin1())) {
        UDSAtom atom;
        atom.m_uds = KIO::UDS_NAME;
        atom.m_str = fileRE.group(9);
        entry.append(atom);

        atom.m_uds = KIO::UDS_SIZE;
        QString theSize(fileRE.group(4)); //TODO: this is data size, what about  resource size?
        atom.m_long = theSize.toLong();
        entry.append(atom);

        atom.m_uds = KIO::UDS_MODIFICATION_TIME;
        atom.m_long = makeTime(fileRE.group(7), fileRE.group(6), fileRE.group(8));
        entry.append(atom);

        atom.m_uds = KIO::UDS_ACCESS;
        if (QString(fileRE.group(1)) == QString("F")) { //if locked then read only
            atom.m_long = 0444;
        } else {
            atom.m_long = 0644;
        }
        entry.append(atom);

        atom.m_uds = KIO::UDS_MIME_TYPE;
        QString mimetype = getMimetype(fileRE.group(2),fileRE.group(3));
        atom.m_str = mimetype.local8Bit();
        entry.append(atom);

        // Is it a file or a link/alias, just make aliases link to themselves
        if (QString(fileRE.group(2)) == QString("adrp") ||
            QString(fileRE.group(2)) == QString("fdrp")) {
            atom.m_uds = KIO::UDS_FILE_TYPE;
            atom.m_long = S_IFREG;
            entry.append(atom);

            atom.m_uds = KIO::UDS_LINK_DEST;
            atom.m_str = fileRE.group(9); //I have a file called "Mozilla alias" the name
                                          // of which displays funny because of this.
                                          // No idea why.  Same for other kioslaves. A font thing?
            entry.append(atom);
            } else {
            atom.m_uds = KIO::UDS_FILE_TYPE;
            atom.m_long = S_IFREG;
            entry.append(atom);
        }
    } else {
        error(ERR_INTERNAL, "hpls output was not matched");
    } //if match dirRE or fileRE

    return entry;
}

//slotGetStdOutput() grabs output from the hp commands
// and adds it to the buffer
void MacProtocol::slotGetStdOutput(KProcess*, char *s, int len) {
  standardOutputStream += QString::fromLocal8Bit(s, len);
}

//slotSetDataStdOutput() is used during hpcopy to give
//standard output to KDE
void MacProtocol::slotSetDataStdOutput(KProcess*, char *s, int len) {
    processedBytes += len;
    processedSize(processedBytes);
    QByteArray array;
    array.setRawData(s, len);
    data(array);
    array.resetRawData(s, len);
}

//makeTime() takes in the date output from hpls -l
//and returns as good a timestamp as we're going to get
int MacProtocol::makeTime(QString mday, QString mon, QString third) {
    int year; int month; int day;
    int hour; int minute;

    //find the month
    if (mon == "Jan") { month = 1; }
    else if (mon == "Feb") { month = 2; }
    else if (mon == "Mar") { month = 3; }
    else if (mon == "Apr") { month = 4; }
    else if (mon == "May") { month = 5; }
    else if (mon == "Jun") { month = 6; }
    else if (mon == "Jul") { month = 7; }
    else if (mon == "Aug") { month = 8; }
    else if (mon == "Sep") { month = 9; }
    else if (mon == "Oct") { month = 10; }
    else if (mon == "Nov") { month = 11; }
    else if (mon == "Dec") { month = 12; }
    else {
        error(ERR_CANNOT_LAUNCH_PROCESS,
              "Month output from hpls -l not matched, e-mail jr@jriddell.org");
        month = 13;
    }

    //if the file is recent (last 12 months) hpls gives us the time,
    // otherwise it only prints the year
    KRegExp hourMin("(..):(..)");
    if (hourMin.match(third.latin1())) {
        QDate currentDate(QDate::currentDate());

        if (month > currentDate.month()) {
            year = currentDate.year() - 1;
        } else {
            year = currentDate.year();
        }
        QString h(hourMin.group(1));
        QString m(hourMin.group(2));
        hour = h.toInt();
        minute = m.toInt();
    } else {
        year = third.toInt();
        hour = 0;
        minute = 0;
    }// if hour:min or year

    day = mday.toInt();

    //check it's valid
    if ( (!QDate::isValid(year, month, day)) || (!QTime::isValid(hour, minute, 0) ) ) {
        error(ERR_CANNOT_LAUNCH_PROCESS,
              "Could not parse a valid date from hpls, e-mail jr@jriddell.org");
    }

    //put it together and work it out
    QDate fileDate(year, month, day);
    QTime fileTime(hour, minute);
    QDateTime fileDateTime(fileDate, fileTime);
    QDateTime epoc(QDate(1970, 1, 1));

    int timestamp = epoc.secsTo(fileDateTime);
    return timestamp;
}

QString MacProtocol::getMimetype(QString type, QString app) {
    if (type == QString("TEXT") && app == QString("ttxt")) {
        return QString("text/plain");
    } else if (type == QString("TEXT") && app == QString("udog")) {
        return QString("text/html");
    } else if (type == QString("svgs")) {
        return QString("text/xml");
    } else if (type == QString("ZIP ")) {
        return QString("application/zip");
    } else if (type == QString("pZip")) {
        return QString("application/zip");
    } else if (type == QString("APPL")) {
        return QString("application/x-executable");
    } else if (type == QString("MooV")) {
        return QString("video/quicktime");
    } else if (type == QString("TEXT") && app == QString("MSWD")) {
        return QString("application/vnd.ms-word");
    } else if (type == QString("PDF ")) {
        return QString("application/pdf");
    } else if (app == QString("CARO")) {
        return QString("application/pdf");
    } else if (type == QString("SIT5")) {
        return QString("application/x-stuffit");
    } else if (type == QString("SITD")) {
        return QString("application/x-stuffit");
    } else if (type == QString("SIT!")) {
        return QString("application/x-stuffit");
    } else if (app == QString("SIT!")) {
        return QString("application/x-stuffit");
    } else if (type == QString("RTFf")) {
        return QString("text/rtf");
    } else if (type == QString("GIFf")) {
        return QString("image/gif");
    } else if (type == QString("JPEG")) {
        return QString("image/jpeg");
    } else if (type == QString("XBMm")) {
        return QString("image/x-xbm");
    } else if (type == QString("EPSF")) {
        return QString("application/postscript");
    } else if (type == QString("TIFF")) {
        return QString("image/tiff");
    } else if (type == QString("PICT")) {
        return QString("image/pict");
    } else if (type == QString("ULAW")) {
        return QString("audio/basic");
    } else if (type == QString("AIFF")) {
        return QString("audio/x-aiff");
    } else if (type == QString("WAVE")) {
        return QString("audio/x-wav");
    } else if (type == QString("FFIL") && app == QString("DMOV")) {
        return QString("application/x-font");
    } else if (type == QString("XLS3")) {
        return QString("application/vnd.ms-excel");
    } else if (type == QString("XLS4")) {
        return QString("application/vnd.ms-excel");
    } else if (type == QString("XLS5")) {
        return QString("application/vnd.ms-excel");
    } else if (app == QString("MSWD")) {
        return QString("application/vnd.ms-word");
    } else if (type == QString("TEXT")) {
        return QString("text/plain");
    } else if (app == QString("ttxt")) {
        return QString("text/plain");
    }
    return QString("application/octet-stream");
}


