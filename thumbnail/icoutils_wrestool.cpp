/*
    icoutils_wrestool.cpp - Extract Microsoft Window icons and images using icoutils package

    Copyright (c) 2009-2010 by Pali Rohár <pali.rohar@gmail.com>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU General Public                   *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "icoutils.h"

#include <QList>
#include <QPair>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QProcess>
#include <QSet>

#define abs(n) ( ( n < 0 ) ? -n : n )
typedef QPair < QString, int > IconInExe;

bool IcoUtils::loadIcoImageFromExe(const QString &inputFileName, QIODevice *outputDevice, const qint32 iconNumber)
{

    QProcess wrestool;

    wrestool.start("wrestool", QStringList() << "-l" << inputFileName);
    wrestool.waitForFinished();

    if ( wrestool.exitCode() != 0 )
        return false;

    QList<IconInExe> icons;

    const QString output = QString::fromUtf8(wrestool.readAll());

    // 16 bit binaries don't have "--language"
    const QRegularExpression regExp(QStringLiteral("--type=(\\d+) (?:--name=(.*) --language=(.*)|--name=(.*)) \\[.*\\]"));

    auto matches = regExp.globalMatch(output);

    while (matches.hasNext()) {
        const auto match = matches.next();

        const int type = match.capturedRef(1).toInt();
        if (type != 14) {
            continue;
        }

        QString name = match.captured(2);
        if (name.isEmpty()) {
            name = match.captured(4);
        }

        icons << qMakePair(name, type);
    }

    if ( icons.isEmpty() )
        return false;

    // iconNumber 0 is ambiguous...
    if (iconNumber > 0 && iconNumber < icons.count()) {
        icons = {icons.at(iconNumber)};
    }

    for (const IconInExe &icon : qAsConst(icons)) {

        QString name = icon.first;
        int type = icon.second;

        if ( name.at(0) == '\'' )
            name = name.mid(1, name.size()-2);

        wrestool.start("wrestool", QStringList() << "-x" << "-t" << QString::number(type) << "-n" << name << inputFileName);
        wrestool.waitForFinished();

        if (wrestool.exitCode() != 0) {
            return false;
        }

        const QByteArray iconData = wrestool.readAllStandardOutput();

        if (outputDevice->write(iconData) != iconData.size()) {
            return false;
        }

        return true;
    }

    return false;

}
