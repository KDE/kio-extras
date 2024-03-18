/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDir>
#include <QString>

class CmdTool : public QObject
{
    Q_OBJECT

public:
    typedef enum { SEP_NUL, SEP_NEWLINE } Separator;

    CmdTool(const QString &path);

    QString path() const;
    QString name() const;
    Separator separator() const;

    bool isValid() const;
    bool isAvailable() const;
    bool run(const QString &searchDir, const QString &searchPattern, bool searchFileContents);

Q_SIGNALS:
    void result(const QString &pathStr);

private:
    bool isDirOnHdd(const QString &dir);

    QDir m_toolDir;
    QString m_name;
    bool m_isValid = false;
    bool m_isAvailable = false;
    Separator m_separator = SEP_NUL;
};
