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

#ifndef SANDBOXEDTHUMBNAILERRUNNER_H
#define SANDBOXEDTHUMBNAILERRUNNER_H

#include <QDataStream>
#include <QProcess>
#include <QSize>

class SandboxedThumbnailerRunner
{
public:
    ~SandboxedThumbnailerRunner();

    /**
     * Creates a thumbnail inside a sandbox.
     * @param pluginName Name of the plugin to use for thumbnailing, e.g. "svgthumbnail"
     * @param path Path to the original file
     * @param size The desired size of the thumbnail
     * @param img The output image
     * @param sequenceIndex See ThumbSequenceCreator::sequenceIndex
     * @return whether creation was successful.
     */
    bool create(const QString &pluginName, const QString &path, QSize size, QImage &img, float sequenceIndex);

private:
    /**
     * Starts the helper and returns true on success.
     */
    bool startHelper();

    /**
     * QProcess does not support blocking reads, which is necessary to use QDataStream directly.
     * This helper function implements reading QDataStream in a loop until everything was
     * read successfully.
     */
    template<typename T> bool helperReadTransaction(T &&handler)
    {
        QDataStream controlIn(&helperProcess);

        for(;;) {
            controlIn.startTransaction();

            // Do whatever is requested with QDataStream
            handler(controlIn);

            if(controlIn.status() == QDataStream::Ok) {
                controlIn.commitTransaction();
                break;
            } else if(controlIn.status() == QDataStream::ReadPastEnd) {
                // Not enough data, try again from the start
                controlIn.rollbackTransaction();
                controlIn.resetStatus();
                if(!helperProcess.waitForReadyRead(30000)) {
                    return false;
                }
            } else {
                // Corrupt data?
                return false;
            }
        }

        return true;
    }

    // For writes, blocking can be implemented much easier - just wait after each write.
    class BlockingQProcess : public QProcess {
    protected:
        virtual qint64 writeData(const char *data, qint64 len) override {
            auto ret = QProcess::writeData(data, len);
            if (ret > 0) {
                waitForBytesWritten();
            }

            return ret;
        }
    };

    BlockingQProcess helperProcess;
};

#endif // SANDBOXEDTHUMBNAILERRUNNER_H
