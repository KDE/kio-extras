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
#include <QRegExp>
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

    const QStringList output = QString(wrestool.readAll()).split('\n');

    QRegExp regExp("--type=(.*) --name=(.*) --language=(.*) \\[(.*)\\]");

    // If we specify number of icon, use only group icons (Windows use only group icons)
    if ( iconNumber > 0 )
        regExp.setPattern("--type=(14) --name=(.*) --language=(.*) \\[(.*)\\]");

    QList <IconInExe> icons;

    // First try use group icons (type 14, default first for windows executables), then icons (type 3), then group cursors (type 12) and finally cursors (type 1)
    // Note: Last icon (type 3) could be in higher resolution

    // Group Icons
    for (const QString &line : output) {
        if ( regExp.indexIn(line) != -1 && regExp.cap(1).toInt() == 14 )
            icons << qMakePair(regExp.cap(2), 14);
    }

    // Icons
    for (const QString &line : output) {
        if ( regExp.indexIn(line) != -1 && regExp.cap(1).toInt() == 3 )
            icons << qMakePair(regExp.cap(2), 3);
    }

    if ( icons.isEmpty() )
        return false;

    if ( iconNumber > 0 && icons.size() >= iconNumber )
        icons = QList <IconInExe> () << icons.at(iconNumber+1);

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
