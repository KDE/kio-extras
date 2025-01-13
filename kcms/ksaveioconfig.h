/*
    SPDX-FileCopyrightText: 2001 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSAVEIO_CONFIG_H_
#define KSAVEIO_CONFIG_H_

#include <QString>

class QWidget;

namespace KSaveIOConfig
{

int proxyDisplayUrlFlags();
void setProxyDisplayUrlFlags(int);

/** Timeout Settings */
void setReadTimeout(int);

void setConnectTimeout(int);

void setProxyConnectTimeout(int);

void setResponseTimeout(int);

/** Miscellaneous Settings */
void setMarkPartial(bool);

void setMinimumKeepSize(int);

/** Update all running KIO workers */
void updateRunningWorkers(QWidget *parent = nullptr);
}

#endif
