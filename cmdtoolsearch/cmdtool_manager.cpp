/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cmdtool_manager.h"
#include "config.h"
#include "kio_cmdtoolsearch_debug.h"

#include <QStandardPaths>

static void loadDefaults(QStringList &list, const QString &name, const QStringList &searchDirs)
{
    for (const QString &i : searchDirs) {
        QDir dir(i);
        QFile file(dir.filePath(name));
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                line = line.trimmed();
                if (!line.isEmpty() && !line.startsWith(QLatin1Char('#'))) {
                    list.append(line);
                }
            }
            return;
        }
    }
}

CmdToolManager::CmdToolManager()
{
    QList<QString> searchDirs;
    for (auto i : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
        QString path = i + QStringLiteral("/cmdtoolsearch");
        searchDirs.append(path);
    }

    for (const QString &searchDir : searchDirs) {
        qCDebug(KIO_CMDTOOLSEARCH) << "Loading tools from dir " << searchDir;
        QDir dir(searchDir);
        if (!dir.exists()) {
            continue;
        }

        for (const QString &entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (m_tools.contains(entry)) {
                qCDebug(KIO_CMDTOOLSEARCH) << "Tool " << entry << " overriden by " << m_tools[entry]->path();
                continue;
            }

            QDir toolDir(dir.absoluteFilePath(entry));
            CmdTool *tool = new CmdTool(toolDir.canonicalPath());
            if (tool->isValid()) {
                qCDebug(KIO_CMDTOOLSEARCH) << "Found tool:" << entry;
                m_tools.insert(entry, tool);
            } else {
                qCDebug(KIO_CMDTOOLSEARCH) << "Bad tool format:" << entry;
                delete tool;
            }
        }
    }

    loadDefaults(m_defaultFileNameSearchTools, QStringLiteral("default_file_name_search"), searchDirs);
    loadDefaults(m_defaultFileContentSearchTools, QStringLiteral("default_file_content_search"), searchDirs);
}

CmdToolManager::~CmdToolManager()
{
    for (auto i : m_tools.values()) {
        delete i;
    }
}

QStringList CmdToolManager::listAllTools()
{
    auto result = m_tools.keys();
    result.sort();
    return result;
}

QStringList CmdToolManager::listAvailableTools()
{
    QStringList result;
    for (auto i : m_tools.keys()) {
        if (m_tools[i]->isAvailable()) {
            result.append(i);
        }
    }
    result.sort();
    return result;
}

CmdTool *CmdToolManager::getTool(const QString &name)
{
    if (m_tools.contains(name)) {
        qCDebug(KIO_CMDTOOLSEARCH) << "getTool:" << name << "found in" << m_tools[name]->path();
        return m_tools[name];
    } else {
        qCDebug(KIO_CMDTOOLSEARCH) << "getTool:" << name << "not found";
        return nullptr;
    }
}

CmdTool *CmdToolManager::getDefaultFileNameSearchTool()
{
    qCDebug(KIO_CMDTOOLSEARCH) << "Finding default tool for file name search";
    return getFirstAvailableToolInList(m_defaultFileNameSearchTools);
}

CmdTool *CmdToolManager::getDefaultFileContentSearchTool()
{
    qCDebug(KIO_CMDTOOLSEARCH) << "Finding default tool for file content search";
    return getFirstAvailableToolInList(m_defaultFileContentSearchTools);
}

CmdTool *CmdToolManager::getFirstAvailableToolInList(const QStringList &list)
{
    for (const QString &name : list) {
        CmdTool *tool = getTool(name);
        if (!tool) {
            qCDebug(KIO_CMDTOOLSEARCH) << name << "not found";
        } else if (!tool->isAvailable()) {
            qCDebug(KIO_CMDTOOLSEARCH) << name << "not available";
        } else {
            qCDebug(KIO_CMDTOOLSEARCH) << "return" << name;
            return tool;
        }
    }
    return nullptr;
}

#include "cmdtool_manager.moc"