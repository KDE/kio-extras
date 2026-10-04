/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
*/

#include <QTest>

#include <KIO/AuthInfo>

#include "smbauthenticator.h"
#include "smbcontext.h"

namespace
{
// Stands in for KPasswdServer: it holds one saved login for a share, and gives it out the way the
// server does, for any username when none is asked for, and otherwise only for that username.
class SavedLoginFrontend : public SMBAbstractFrontend
{
public:
    bool checkCachedAuthentication(KIO::AuthInfo &info) override
    {
        askedUsernames.append(info.username);
        if (savedUsername.isEmpty() || info.url != QUrl(QStringLiteral("smb://server/share"))) {
            return false;
        }
        if (!info.username.isEmpty() && info.username != savedUsername) {
            return false;
        }
        info.username = savedUsername;
        info.password = savedPassword;
        return true;
    }

    QString savedUsername;
    QString savedPassword;
    QStringList askedUsernames;
};

struct Credentials {
    QString username;
    QString password;
};

// Runs one authentication the way libsmbclient asks for it, with the username of the url.
Credentials authenticate(SMBAuthenticator &authenticator, const char *urlUsername)
{
    SMBCCTX *context = smbc_new_context();
    char workgroup[256] = "WORKGROUP";
    char username[256] = {};
    char password[256] = {};
    qstrncpy(username, urlUsername, sizeof(username));
    authenticator.auth(context, "server", "share", workgroup, sizeof(workgroup), username, sizeof(username), password, sizeof(password));
    smbc_free_context(context, 1);
    return {QString::fromUtf8(username), QString::fromUtf8(password)};
}
}

class SMBAuthenticatorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testAUrlWithoutUserGetsTheSavedLogin()
    {
        // A share added without a username has its login saved under the remote account, which
        // need not be the local one.
        SavedLoginFrontend frontend;
        frontend.savedUsername = QStringLiteral("user@domain.tld");
        frontend.savedPassword = QStringLiteral("secret");
        SMBAuthenticator authenticator(frontend);

        const Credentials credentials = authenticate(authenticator, "");

        QCOMPARE(frontend.askedUsernames, QStringList{QString()});
        QCOMPARE(credentials.username, QStringLiteral("user@domain.tld"));
        QCOMPARE(credentials.password, QStringLiteral("secret"));
    }

    void testAUrlWithoutUserAndNoSavedLoginTriesTheLocalUser()
    {
        qputenv("USER", "localuser");
        SavedLoginFrontend frontend;
        SMBAuthenticator authenticator(frontend);

        const Credentials credentials = authenticate(authenticator, "");

        QCOMPARE(credentials.username, QStringLiteral("localuser"));
        QCOMPARE(credentials.password, QString());
    }

    void testLibsmbclientAsksWithoutAUserForAUrlWithout()
    {
        // libsmbclient asks for credentials before it connects, with the username of the url, or with
        // the default user of the context when the url has none. Nothing has to listen on the port.
        qputenv("USER", "localuser");
        SavedLoginFrontend frontend;
        SMBContext context(new SMBAuthenticator(frontend)); // takes ownership
        QVERIFY(context.isValid());

        const int dir = smbc_opendir("smb://127.0.0.1/share");
        if (dir >= 0) {
            smbc_closedir(dir);
        }

        QVERIFY(!frontend.askedUsernames.isEmpty());
        QCOMPARE(frontend.askedUsernames.first(), QString());
    }

    void testAUrlWithAUserAsksForThatUser()
    {
        SavedLoginFrontend frontend;
        frontend.savedUsername = QStringLiteral("user@domain.tld");
        frontend.savedPassword = QStringLiteral("secret");
        SMBAuthenticator authenticator(frontend);

        const Credentials credentials = authenticate(authenticator, "alice");

        QCOMPARE(frontend.askedUsernames, QStringList{QStringLiteral("alice")});
        QCOMPARE(credentials.username, QStringLiteral("alice"));
        QCOMPARE(credentials.password, QString());
    }
};

QTEST_GUILESS_MAIN(SMBAuthenticatorTest)

#include "smbauthenticatortest.moc"
