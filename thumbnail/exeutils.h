/*
    exeutils.h - Extract Microsoft Window icons from Microsoft Windows executables

    SPDX-FileCopyrightText: 2023 John Chadwick <john@jchw.io>

    SPDX-License-Identifier: LGPL-2.0-or-later OR BSD-2-Clause
*/

#pragma once

#include <QtGlobal>

class QDataStream;
class QIODevice;

namespace ExeUtils
{

bool loadIcoDataFromExe(QIODevice *inputDevice, QIODevice *outputDevice);

}
