/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cmdtool.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStorageInfo>

CmdTool::CmdTool(const QString &path)
    : m_toolDir(path)
{
    m_name = m_toolDir.dirName();

    if (!m_toolDir.exists() || !m_toolDir.exists(QStringLiteral("metadata.json")) || !m_toolDir.exists(QStringLiteral("check"))
        || !m_toolDir.exists(QStringLiteral("run"))) {
        return;
    }

    // Read metadata
    QFile metadataFile(m_toolDir.absoluteFilePath(QStringLiteral("metadata.json")));
    if (!metadataFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open" << metadataFile.fileName();
        return;
    }

    QJsonObject metadata = QJsonDocument::fromJson(metadataFile.readAll()).object();
    metadataFile.close();
    QString separator = metadata[QStringLiteral("separator")].toString();
    if (separator == QStringLiteral("nul")) {
        m_separator = SEP_NUL;
    } else if (separator == QStringLiteral("newline")) {
        m_separator = SEP_NEWLINE;
    } else {
        return;
    }

    m_isValid = true;

    // Check if the tool is available
    QProcess process;
    process.start(m_toolDir.absoluteFilePath(QStringLiteral("check")), QStringList(), QIODevice::ReadOnly);
    if (process.waitForFinished() && process.exitCode() == 0) {
        m_isAvailable = true;
    }
}

QString CmdTool::path() const
{
    return m_toolDir.absolutePath();
}

QString CmdTool::name() const
{
    return m_name;
}

bool CmdTool::isValid() const
{
    return m_isAvailable;
}

bool CmdTool::isAvailable() const
{
    return m_isAvailable;
}

CmdTool::Separator CmdTool::separator() const
{
    return m_separator;
}

bool CmdTool::run(const QString &searchDir, const QString &searchPattern, bool searchFileContents)
{
    if (!isAvailable()) {
        return false;
    }

    QProcess process;
    process.setWorkingDirectory(searchDir);
    process.setProgram(m_toolDir.absoluteFilePath(QStringLiteral("run")));

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("SEARCH_PATTERN"), searchPattern);
    env.insert(QStringLiteral("SEARCH_FILE_CONTENTS"), searchFileContents ? QStringLiteral("1") : QStringLiteral("0"));
    env.insert(QStringLiteral("ON_HDD"), isDirOnHdd(searchDir) ? QStringLiteral("1") : QStringLiteral("0"));
    process.setProcessEnvironment(env);

    process.start(QIODeviceBase::ReadWrite | QIODeviceBase::Unbuffered);
    // Explicitly close the write channel, to avoid some tools waiting for input (e.g. ripgrep, when no path is given on cmdline)
    process.closeWriteChannel();

    QDir rootDir(searchDir);
    QByteArray output;
    const char sep = separator() == CmdTool::SEP_NUL ? '\0' : '\n';

    do {
        if (!process.waitForReadyRead()) {
            continue;
        }
        output.append(process.readAll());
        int begin = 0;
        while (begin < output.size()) {
            const int end = output.indexOf(sep, begin);
            if (end < 0) {
                // incomplete output, wait for more
                break;
            }

            if (end > begin) {
                QString s = QString::fromUtf8(output.mid(begin, end - begin));
                Q_EMIT result(s);
            }

            begin = end + 1;
        }
        if (begin < output.size()) {
            output = output.mid(begin);
        } else {
            output.clear();
        }
    } while (process.state() == QProcess::Running);

    if (!output.isEmpty()) {
        QString s = QString::fromUtf8(output);
        Q_EMIT result(s);
    }

    return process.exitCode() == 0;
}

bool CmdTool::isDirOnHdd(const QString &dir)
{
#ifdef Q_OS_LINUX
    QStorageInfo storageInfo(dir);
    if (!storageInfo.isValid()) {
        return false;
    }

    QString device = QString::fromUtf8(storageInfo.device());
    if (!device.startsWith(QStringLiteral("/dev/"))) {
        return false;
    }
    device = device.mid(5);

    QDir sysfs(QStringLiteral("/sys/block"));
    QStringList diskList = sysfs.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    diskList.sort();

    // Try to match from the longest prefix
    for (auto i = diskList.rbegin(); i != diskList.rend(); ++i) {
        const QString &diskName = *i;
        if (!device.startsWith(diskName)) {
            continue;
        }
        QFile f(sysfs.absoluteFilePath(diskName + QStringLiteral("/queue/rotational")));
        if (!f.open(QIODevice::ReadOnly)) {
            return false;
        }
        QString rotational = QString::fromUtf8(f.readAll()).trimmed();
        f.close();
        return rotational.toInt() == 1;
    }
#endif
    return false;
}

#include "cmdtool.moc"
