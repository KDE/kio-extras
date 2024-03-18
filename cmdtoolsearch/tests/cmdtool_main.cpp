/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cmdtool.h"
#include "cmdtool_manager.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QString>
#include <QTimer>

namespace Options
{

static QCommandLineOption list()
{
    const static QCommandLineOption o{QStringLiteral("list"), QStringLiteral("list all tools")};
    return o;
}

static QCommandLineOption check()
{
    const static QCommandLineOption o{QStringLiteral("check"), QStringLiteral("check if <tool> is available"), QStringLiteral("tool")};
    return o;
}

static QCommandLineOption run()
{
    const static QCommandLineOption o{QStringLiteral("run"), QStringLiteral("run <tool>"), QStringLiteral("tool")};
    return o;
}

} // namespace Options

Q_GLOBAL_STATIC_WITH_ARGS(QTextStream, cout, (stdout))
Q_GLOBAL_STATIC_WITH_ARGS(QTextStream, cerr, (stderr))

class CmdToolApp : public QCoreApplication
{
    Q_OBJECT

public:
    CmdToolApp(int &argc, char **argv, QCommandLineParser *parser)
        : QCoreApplication(argc, argv)
        , m_parser(parser)
    {
        QTimer::singleShot(0, this, &CmdToolApp::runMain);
    }

private Q_SLOTS:
    void runMain()
    {
        CmdToolManager manager;

        if (m_parser->isSet(Options::list())) {
            // list all tools
            for (auto i : manager.listAllTools()) {
                std::optional<CmdTool *> tool = manager.getTool(i);
                *cout << i << '\t' << (*tool)->path() << '\t' << (*tool)->isAvailable() << '\n';
            }
            exit(0);
        }

        if (m_parser->isSet(Options::check())) {
            QString name = m_parser->value(Options::check());
            std::optional<CmdTool *> tool = manager.getTool(name);
            if (!tool) {
                *cerr << QStringLiteral("Tool %1 not found").arg(name) << '\n';
                exit(1);
            }
            bool available = (*tool)->isAvailable();
            if (available) {
                *cout << QStringLiteral("Tool %1 is available").arg(name) << '\n';
                exit(0);
            } else {
                *cout << QStringLiteral("Tool %1 is not available").arg(name) << '\n';
                exit(1);
            }
        }

        if (m_parser->isSet(Options::run())) {
            QString name = m_parser->value(Options::run());
            std::optional<CmdTool *> tool = manager.getTool(name);
            if (!tool) {
                *cerr << QStringLiteral("Tool %1 not found").arg(name) << '\n';
                exit(1);
            }
            QString searchDir = m_parser->positionalArguments().at(0);
            QString searchPattern = m_parser->positionalArguments().at(1);
            bool searchFileContents = false;
            connect(*tool, &CmdTool::result, [](QString pathStr) {
                *cout << pathStr << '\n';
            });
            bool success = (*tool)->run(searchDir, searchPattern, searchFileContents);
            exit(success ? 0 : 1);
        }
    }

private:
    QCommandLineParser *m_parser;
};

int main(int argc, char **argv)
{
    QCommandLineParser parser;
    CmdToolApp app(argc, argv, &parser);

    const QString description = QStringLiteral("CmdTool");
    const auto version = QStringLiteral("1.0");

    app.setApplicationVersion(version);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.setApplicationDescription(description);
    parser.addOptions({Options::list(), Options::check(), Options::run()});
    parser.addPositionalArgument(QStringLiteral("search_dir"), QStringLiteral("The search dir"));
    parser.addPositionalArgument(QStringLiteral("search_pattern"), QStringLiteral("The search pattern"));
    parser.process(app);

    // at least one operation should be specified
    if (!parser.isSet(Options::list()) && !parser.isSet(Options::check()) && !parser.isSet(Options::run())) {
        parser.showHelp(0);
    }
    return app.exec();
}

#include "cmdtool_main.moc"