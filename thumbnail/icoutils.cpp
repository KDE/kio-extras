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

#include <cstdlib> //for abs()

#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QProcess>
#include <QPair>
#include <QSet>

#define Icon QPair < int, QPair < int, QPair < int, int > > >
#define makeIcon(width, height, depth, index) qMakePair(width, qMakePair(height, qMakePair(depth, index)))

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

	foreach ( const QString &line, output ) {
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

bool IcoUtils::convertIcoToPng(const QString &inputPath, const QString &outputPath, int needWidth, int needHeight) {

	QProcess icotool;

	icotool.start("icotool", QStringList() << "-l" << inputPath);
	icotool.waitForFinished();

	if ( icotool.exitCode() != 0 )
		return false;

	QSet <Icon> icons;
	const QStringList output = QString(icotool.readAll()).split('\n');
	QRegExp regExp("--(.*) --index=(.*) --width=(.*) --height=(.*) --bit-depth=(.*) --palette-size=(.*)");

	foreach ( const QString &line, output ) {
		if ( regExp.indexIn(line) != -1 )
			icons << makeIcon(regExp.cap(3).toInt(), regExp.cap(4).toInt(), regExp.cap(5).toInt(), regExp.cap(2).toInt());
	}

	int min_w = 1024;
	int min_h = 1024;
	int max_d = 0;
	int index = -1;

	foreach ( const Icon &icon, icons ) {
		int i_width = icon.first;
		int i_height = icon.second.first;
		int i_depth = icon.second.second.first;
		int i_index = icon.second.second.second;
		int i_w = abs(i_width - needWidth);
		int i_h = abs(i_height - needHeight);
		if ( i_w < min_w || ( i_w == min_w && i_h < min_h ) || ( i_w == min_w && i_h == min_h && i_depth > max_d ) ) {
			min_w = i_w;
			min_h = i_h;
			max_d = i_depth;
			index = i_index;
		}
	}

	if ( index == -1 )
		return false;

	icotool.start("icotool", QStringList() << "-x" << "-i" << QString::number(index) << inputPath << "-o" << outputPath);
	icotool.waitForFinished();

	if ( icotool.exitCode() != 0 )
		return false;

	return true;

}

