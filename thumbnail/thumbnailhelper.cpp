/*  This file is part of the KDE libraries
    Copyright (C) 2019 Fabian Vogt <fabian@ritter-vogt.de> 

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <seccomp.h>

#include <QGuiApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QImage>

#include <KServiceTypeTrader>
#include <kio/thumbsequencecreator.h>

#include "thumbnailhelper-debug.h"

int main(int argc, char *argv[])
{
    setenv("QT_QPA_PLATFORM", "offscreen", 1);
    QGuiApplication app(argc, argv);

    if(app.arguments().size() != 1) {
        qWarning(THUMBNAILHELPER_LOG) << "Only for internal use";
        return 1;
    }

    // Open control streams
    QFile stdinFile, stdoutFile;
    if (!stdinFile.open(stdin, QIODevice::ReadOnly) || !stdoutFile.open(stdout, QIODevice::WriteOnly)) {
        qWarning(THUMBNAILHELPER_LOG) << "Failed to open control streams";
        return 1;
    }
    // Disable buffering to allow QFile direct access
    setvbuf(stdin, nullptr, _IONBF, 0);
    setvbuf(stdout, nullptr, _IONBF, 0);

    // Close everything except the control FDs
    struct rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
        for(unsigned int fd = 3; fd < rlim.rlim_cur; ++fd)
            close(fd);
    }

    QMap<QString,ThumbCreator*> creators;

    // Load all thumbnail creators which support sandboxing
    const KService::List plugins = KServiceTypeTrader::self()->query(QStringLiteral("ThumbCreator"));
    Q_FOREACH(KService::Ptr plugin, plugins) {
        if(!plugin->property(QStringLiteral("SupportsSandbox")).toBool())
            continue; // TODO Use weird KServiceTypeTrader syntax

        QLibrary library(KPluginLoader::findPlugin((plugin->library())));
        if(!library.load())
            continue;

        newCreator creatorFactory = (newCreator)library.resolve("new_creator");
        ThumbCreator *creator;
        if (!creatorFactory || !(creator = creatorFactory())) {
            library.unload();
            continue;
        }

        if ((creator->flags() & ThumbCreator::SupportsSandbox) == 0) {
            delete creator;
            library.unload();
            continue;
        }

        qDebug(THUMBNAILHELPER_LOG) << "Loaded" << plugin->desktopEntryName();
        creators[plugin->desktopEntryName()] = creator;
    }

    // Establish the sandbox: Whitelist only specific calls.
    // For SCMP_CMP_EQ, the argument has to be duplicated to avoid compiler warnings.
    auto ctx = seccomp_init(SCMP_ACT_ERRNO(EPERM));
    // Allow opening files (but not creating new ones)

    // Only allow open(at) if flags & openflags == O_RDONLY
    int openflags = O_CREAT | O_TMPFILE | O_ACCMODE;
    // O_TMPFILE is actually (__O_TMPFILE | O_DIRECTORY), but we don't care about O_DIRECTORY
    openflags &= ~O_DIRECTORY;

    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(open), 1,
                     SCMP_A1(SCMP_CMP_MASKED_EQ, openflags, O_RDONLY));
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(openat), 1,
                     SCMP_A2(SCMP_CMP_MASKED_EQ, openflags, O_RDONLY));
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);

    // Allow writing to stdout and stderr
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 1,
                     SCMP_A0(SCMP_CMP_EQ, 1, 1));
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 1,
                     SCMP_A0(SCMP_CMP_EQ, 2, 2));

    // Allow read operations
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lseek), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(access), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lstat), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(stat), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(statx), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(readlink), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstatfs), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getdents), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getdents64), 0);

    // Allow changing fd flags
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fcntl), 1,
                     SCMP_A1(SCMP_CMP_EQ, F_GETFL, F_GETFL));
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fcntl), 1,
                     SCMP_A1(SCMP_CMP_EQ, F_SETFL, F_SETFL));

    // Memory allocation
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);

    // Exiting
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
    if(seccomp_load(ctx) != 0) {
        qWarning(THUMBNAILHELPER_LOG) << "Failed to establish sandbox";
        return 1;
    }

    seccomp_release(ctx); // It's in the kernel now

    // Do some selftests
    auto failed_eperm = [](int ret) { return ret < 0 && errno == EPERM; };
    if(!failed_eperm(open(argv[0], O_WRONLY))
            || !failed_eperm(openat(AT_FDCWD, argv[0], O_RDWR))
            || !failed_eperm(write(3, "nope", 4))
            || !failed_eperm(dup2(1, 3))) {
        qWarning(THUMBNAILHELPER_LOG) << "Sandbox selftest failed";
        return 1;
    }

    QDataStream controlIn(&stdinFile), controlOut(&stdoutFile);

    /* Do a handshake.
     * If the protocol version has to be bumped, it should be taken care
     * that old kio_thumbnail processes are still supported - otherwise
     * during an update a running kio_thumbnail slave might try to do a
     * handshake with the new thumbnailhelper and fail, falling back to not
     * using the sandbox at all. */
    int ourProtocolVersion = 1, theirProtocolVersion;
    controlIn >> theirProtocolVersion;

    if (ourProtocolVersion != theirProtocolVersion) {
        qWarning(THUMBNAILHELPER_LOG) << "Protocol version mismatch";
        return 1;
    }

    controlOut << ourProtocolVersion;
    stdoutFile.flush();

    // A QSocketNotifier could be used instead, but it only makes everything more complex
    while (!controlIn.atEnd()) {
        QString pluginName, filename;
        QSize size;
        float sequenceIndex;

        // Read what we should do
        controlIn >> pluginName >> filename >> size >> sequenceIndex;

        if (controlIn.status() != QDataStream::Ok)
            break;

        auto creatorIt = creators.find(pluginName);

        if (creatorIt == creators.end()) {
            controlOut << false; // Not found
            continue;
        }

        if(auto *sequenceCreator = dynamic_cast<ThumbSequenceCreator*>(*creatorIt)) {
            // Set the sequence index, if possible
            sequenceCreator->setSequenceIndex(sequenceIndex);
        }

        QImage image;
        if ((*creatorIt)->create(filename, size.width(), size.height(), image)) {
            // Creation succeeded, prepare image sending
            QImage imgToSend = image.convertToFormat(QImage::Format_ARGB32);

            if(imgToSend.sizeInBytes() > ssize_t(INT_MAX)) {
                // Can't send, too big
                controlOut << false;
            } else {
                // Send the image data
                controlOut << true << image.size() << int(imgToSend.sizeInBytes());
                controlOut.writeRawData(reinterpret_cast<const char*>(imgToSend.bits()), imgToSend.sizeInBytes());
            }
        } else {
            // Creation failed
            controlOut << false;
        }

        // Make sure the other side can read it
        stdoutFile.flush();
    }

    // Don't bother shutting anything down
    _exit(0);
}
