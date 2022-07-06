/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
*/

#include <utility>

#include <QProcess>
#include <QTest>

#include <KIO/Job>

#include "transfer_resume.h"

struct FakeWorker {
    struct FakeError {
        const int id;
        const QString message;
    };

    void error(int id, const QString &message)
    {
        m_errors.push_back(FakeError{id, message});
    }

    bool configValue(const QString &key, bool defaultValue) const
    {
        return m_config.value(key, defaultValue);
    }

    bool canResume(KIO::filesize_t offset)
    {
        Q_UNUSED(offset); // in reality this is a user query, always assume the user says yes for now
        return true;
    }

    void debugErrors()
    {
        for (const auto &error : std::as_const(m_errors)) {
            qDebug() << "ERROR{" << KIO::buildErrorString(error.id, error.message) << "}";
        }
    }

    QVector<FakeError> m_errors;
    QHash<QString, bool> m_config = {{"MarkPartial", true}};
};

class ShouldResumeTest : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir m_tmpDir;
private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_tmpDir.isValid());
        const QString fixturesPath = QFINDTESTDATA("fixtures/.");
        QProcess proc;
        proc.setProcessChannelMode(QProcess::ForwardedChannels);
        proc.start("cp", {"-rv", fixturesPath, m_tmpDir.path()});
        QVERIFY(proc.waitForFinished());
        QCOMPARE(proc.exitCode(), 0);
    }

    QString tmpPath(const QString &subpath)
    {
        return QDir(m_tmpDir.path()).filePath(subpath);
    }

    QUrl tmpUrl(const QString &subpath)
    {
        return QUrl::fromLocalFile(tmpPath(subpath));
    }

    void noResumeButPartial()
    {
        // NB: this has no fixture ;)
        FakeWorker worker;

        auto url = tmpUrl("noResumeButPartial/thing");
        auto partUrl = tmpUrl("noResumeButPartial/thing.part");
        auto resumeVariant = Transfer::shouldResume<QFileResumeIO>(url, KIO::JobFlags(), &worker);
        QVERIFY(std::holds_alternative<TransferContext>(resumeVariant));
        auto resume = std::get<TransferContext>(resumeVariant);
        QCOMPARE(resume.resuming, false);
        QCOMPARE(resume.destination, partUrl);
        QCOMPARE(resume.completeDestination, url);
        QCOMPARE(resume.partDestination, partUrl);

        QDir().mkdir(tmpPath("noResumeButPartial"));
        QFile part(partUrl.toLocalFile());
        QVERIFY(part.open(QFile::WriteOnly));
        part.write("");
        QVERIFY(Transfer::concludeResumeHasError<QFileResumeIO>(WorkerResult::pass(), resume, &worker).success());
        QVERIFY(QFileInfo::exists(url.toLocalFile()));
        QVERIFY(!QFileInfo::exists(partUrl.toLocalFile()));
    }

    void noResumeAndNoPartial()
    {
        // NB: this has no fixture ;)
        FakeWorker worker;
        worker.m_config["MarkPartial"] = false;


        auto url = tmpUrl("noResumeAndNoPartial/thing");
        auto resumeVariant = Transfer::shouldResume<QFileResumeIO>(url, KIO::JobFlags(), &worker);
        worker.debugErrors();
        QVERIFY(std::holds_alternative<TransferContext>(resumeVariant));
        auto resume = std::get<TransferContext>(resumeVariant);
        QCOMPARE(resume.resuming, false);
        QCOMPARE(resume.destination, url);
        QCOMPARE(resume.completeDestination, url);
        QCOMPARE(resume.partDestination, QUrl());
        QVERIFY(Transfer::concludeResumeHasError<QFileResumeIO>(WorkerResult::pass(), resume, &worker).success());
    }

    void resume()
    {
        FakeWorker worker;

        auto url = tmpUrl("resume/thing");
        auto partUrl = tmpUrl("resume/thing.part");
        auto resumeVariant = Transfer::shouldResume<QFileResumeIO>(url, KIO::JobFlags(), &worker);
        QVERIFY(std::holds_alternative<TransferContext>(resumeVariant));
        auto resume = std::get<TransferContext>(resumeVariant);
        QCOMPARE(resume.resuming, true);
        QCOMPARE(resume.destination, partUrl);
        QCOMPARE(resume.completeDestination, url);
        QCOMPARE(resume.partDestination, partUrl);

        QVERIFY(Transfer::concludeResumeHasError<QFileResumeIO>(WorkerResult::pass(), resume, &worker).success());
        QVERIFY(QFileInfo::exists(url.toLocalFile()));
        QVERIFY(!QFileInfo::exists(partUrl.toLocalFile()));
    }

    void resumeInPlace()
    {
        FakeWorker worker;

        auto url = tmpUrl("resumeInPlace/thing");
        auto resumeVariant = Transfer::shouldResume<QFileResumeIO>(url, KIO::Resume, &worker);
        QVERIFY(std::holds_alternative<TransferContext>(resumeVariant));
        auto resume = std::get<TransferContext>(resumeVariant);
        QCOMPARE(resume.resuming, true);
        QCOMPARE(resume.destination, url);
        QCOMPARE(resume.completeDestination, url);
        QCOMPARE(resume.partDestination, url);

        QVERIFY(Transfer::concludeResumeHasError<QFileResumeIO>(WorkerResult::pass(), resume, &worker).success());
        QVERIFY(QFileInfo::exists(url.toLocalFile()));
    }

    void noResumeInPlace()
    {
        FakeWorker worker;

        auto url = tmpUrl("resumeInPlace/thing"); // intentionally the same path this scenario errors out
        auto resumeVariant = Transfer::shouldResume<QFileResumeIO>(url, KIO::JobFlags(), &worker);
        QVERIFY(std::holds_alternative<WorkerResult>(resumeVariant));
        auto result = std::get<WorkerResult>(resumeVariant);
        QCOMPARE(result.error(), KIO::ERR_FILE_ALREADY_EXIST);
    }
};

QTEST_GUILESS_MAIN(ShouldResumeTest)

#include "shouldresumetest.moc"
