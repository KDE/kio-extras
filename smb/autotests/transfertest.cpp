/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
*/

#include <QTest>

#include <future>
#include <thread>

#include "transfer.h"

class TransferTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSegmentOnSmallFile()
    {
        // Files smaller than our minimal segment size ought to be transferred in one go
        // otherwise we have a chance of degrading performance.
        QCOMPARE(TransferSegment(1).buf.size(), 1);
    }

    void testMaxSegment()
    {
        // Large files may only use up to a given maximum.
        QCOMPARE(TransferSegment(512 * 1024 * 1024).buf.size(), c_maxSegmentSize);
    }

    void testIdealSegmentSize()
    {
        QCOMPARE(TransferSegment(64 * 1024 * 1024).buf.size(), 1342177);
    }

    void testSegment()
    {
        TransferSegment s(8);
        QCOMPARE(s.buf.size(), 8);
        memset(s.buf.data(), 1, 8);
        QCOMPARE(s.buf.data()[0], 1);
    }

    void testRing()
    {
        TransferRingBuffer ring(8);
        for (auto i = 0; i <= 32; ++i) {
            {
                auto s = ring.nextFree();
                memset(s->buf.data(), i, 8);
                ring.push();
            }
            {
                auto s = ring.pop();
                QCOMPARE(s->buf.data()[0], static_cast<char>(i));
                ring.unpop();
            }
        }
    }

    void testRingThreadedSlowPush()
    {
        const auto runs = 127;
        const auto fileSize = 8;
        TransferRingBuffer ring(fileSize);

        std::atomic<bool> abort(false);

        auto pullFuture = std::async(std::launch::async, [&ring, &abort]() -> bool {
            for (auto i = 0; i <= runs && !abort; ++i) {
                auto s = ring.pop();
                if (!QTest::qCompare(s->buf.data()[0], static_cast<char>(i),
                                     qPrintable(QStringLiteral("On pull iteration %1").arg(i)), "",
                                     __FILE__, __LINE__)) {
                    abort = true;
                    return false;
                }
                ring.unpop();
            }
            return true;
        });

        auto pushFuture = std::async(std::launch::async, [&ring, &abort]() -> bool {
            for (auto i = 0; i <= runs && !abort; ++i) {
                auto s = ring.nextFree();
                memset(s->buf.data(), i, fileSize);
                ring.push();
                if (abort) {
                    ring.done();
                    return false;
                }
                // Slow down this thread to simulate slow network reads.
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            ring.done();
            return true;
        });

        pushFuture.wait();
        pullFuture.wait();

        QVERIFY(pushFuture.get());
        QVERIFY(pullFuture.get());
    }

    void testRingThreadedSlowPull()
    {
        const auto runs = 127;
        const auto fileSize = 8;
        TransferRingBuffer ring(fileSize);

        std::atomic<bool> abort(false);

        auto pullFuture = std::async(std::launch::async, [&ring, &abort]() -> bool {
            for (auto i = 0; i <= runs && !abort; ++i) {
                auto s = ring.pop();
                if (!QTest::qCompare(s->buf.data()[0], static_cast<char>(i),
                                     qPrintable(QStringLiteral("On pull iteration %1").arg(i)), "",
                                     __FILE__, __LINE__)) {
                    abort = true;
                }
                // Slow down this thread to simulate slow local writes.
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                ring.unpop();
            }
            return true;
        });

        auto pushFuture = std::async(std::launch::async, [&ring, &abort]() -> bool {
            for (auto i = 0; i <= runs && !abort; ++i) {
                auto s = ring.nextFree();
                memset(s->buf.data(), i, fileSize);
                if (abort) {
                    ring.done();
                    return false;
                }
                ring.push();
            }
            ring.done();
            return true;
        });

        pushFuture.wait();
        pullFuture.wait();

        QVERIFY(pushFuture.get());
        QVERIFY(pullFuture.get());
    }
};

QTEST_GUILESS_MAIN(TransferTest)

#include "transfertest.moc"
