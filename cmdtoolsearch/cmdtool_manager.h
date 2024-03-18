/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "cmdtool.h"

#include <QMap>
#include <QString>
#include <QStringList>

class CmdToolManager : public QObject
{
    Q_OBJECT

public:
    CmdToolManager();
    ~CmdToolManager();

    QStringList listAllTools();
    QStringList listAvailableTools();

    CmdTool *getTool(const QString &name);
    CmdTool *getDefaultFileNameSearchTool();
    CmdTool *getDefaultFileContentSearchTool();

private:
    CmdTool *getFirstAvailableToolInList(const QStringList &list);

    QMap<QString, CmdTool *> m_tools;
    QStringList m_defaultFileNameSearchTools;
    QStringList m_defaultFileContentSearchTools;
};
