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

#include <QRegularExpression>
#include <QString>
#include <QProcess>

bool IcoUtils::loadIcoImageFromExe(const QString &inputFileName, QIODevice *outputDevice)
{
    QProcess wrestool;

    // list all resources with type RT_GROUP_ICON=14
    wrestool.start(QStringLiteral("wrestool"), {QStringLiteral("-t14"), QStringLiteral("-l"), inputFileName});
    wrestool.waitForFinished();

    if (wrestool.exitCode() != 0) {
        return false;
    }

    const QString output = QString::fromUtf8(wrestool.readAll());

    // 16 bit binaries don't have "--language"
    const QRegularExpression regExp(QStringLiteral("--type=\\d+ --name=(\\S+) (?:--language=.* )?\\[.*\\]"));

    // https://docs.microsoft.com/en-us/windows/win32/menurc/about-icons#icon-display
    // "Select the RT_GROUP_ICON resource. If more than one such resource exists,
    // the system uses the first resource listed in the resource scrip."
    auto match = regExp.match(output);
    if (!match.hasMatch()) {
        return false;
    }

    QString name = match.captured(1);
    if (name.at(0) == '\'') {
        name = name.mid(1, name.size()-2);
    }

    wrestool.start(QStringLiteral("wrestool"), {QStringLiteral("-x"), QStringLiteral("-t14"), QStringLiteral("-n"), name, inputFileName});
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
