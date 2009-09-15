/*
    icoutils.cpp - Extract Microsoft Window icons and images using icoutils package

    Copyright (c) 2009 by Pali Rohár <pali.rohar@gmail.com>

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

#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QProcess>

bool IcoUtils::convertExeToIco(const QString &inputPath, const QString &outputPath) {

	QProcess wrestool;

	wrestool.start("wrestool", QStringList() << "-l" << inputPath);
	wrestool.waitForFinished();

	if ( wrestool.exitCode() != 0 )
		return false;

	const QStringList output = QString(wrestool.readAll()).split('\n');

	QRegExp regExp("--type=(.*) --name=(.*) --language=(.*) \\[(.*)\\]");

	QStringList icons;
	QStringList groupIcons;

	foreach ( const QString& line, output ) {
		if ( regExp.indexIn(line) != -1 ) {
			if ( regExp.cap(1).toInt() == 14 )
				groupIcons << regExp.cap(2);
			if ( regExp.cap(1).toInt() == 3 )
				icons << regExp.cap(2);
		}
	}

	QString name;
	int type;

	if ( groupIcons.isEmpty() && icons.isEmpty() ) 
		return false;

	if ( groupIcons.isEmpty() ) {
		name = *(--icons.end());
		type = 3;
	} else {
		name = *groupIcons.begin();
		type = 14;
	}

	if ( name.at(0) == '\'' ) {
		name = name.remove(0, 1);
		name = name.remove(name.size()-1, name.size());
	}

	wrestool.start("wrestool", QStringList() << "-x" << "-t" << QString::number(type) << "-n" << name << inputPath << "-o" << outputPath);
	wrestool.waitForFinished();

	if ( wrestool.exitCode() != 0 )
		return false;

	return true;

}

bool IcoUtils::convertIcoToPng(const QString &inputPath, const QString &outputPath) {

	QProcess icotool;

	icotool.start("icotool", QStringList() << "-l" << inputPath);
	icotool.waitForFinished();

	if ( icotool.exitCode() != 0 )
		return false;

	const QStringList output = QString(icotool.readAll()).split('\n');

	QRegExp regExp("--(.*) --index=(.*) --width=(.*) --height=(.*) --bit-depth=(.*) --palette-size=(.*)");

	int index = 0;
	int width = 0;
	int depth = 0;

	foreach ( const QString& line, output ) {
		if ( regExp.indexIn(line) != -1 ) {
			if ( regExp.cap(3).toInt() > width || ( regExp.cap(3).toInt() == width && regExp.cap(5).toInt() > depth ) ) {
				index = regExp.cap(2).toInt();
				width = regExp.cap(3).toInt();
				depth = regExp.cap(5).toInt();
			}
		}
	}

	if ( index == 0 )
		return false;

	icotool.start("icotool", QStringList() << "-x" << "-i" << QString::number(index) << inputPath << "-o" << outputPath);
	icotool.waitForFinished();

	if ( icotool.exitCode() != 0 )
		return false;

	return true;

}

