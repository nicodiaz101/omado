#include <QTest>
#include <QCryptographicHash>
#include "core/KeychainStore.h"
#include "core/AuthManager.h"

class tst_AuthManager : public QObject {
    Q_OBJECT

private slots:
    void testPkceGeneration() {
        QString verifier = AuthManager::generateCodeVerifier();
        QVERIFY(!verifier.isEmpty());
        QVERIFY(verifier.length() >= 43);
        // Validar que no tenga padding = ni caracteres no URL-safe (+, /)
        QVERIFY(!verifier.contains('='));
        QVERIFY(!verifier.contains('+'));
        QVERIFY(!verifier.contains('/'));

        QString challenge = AuthManager::generateCodeChallenge(verifier);
        QVERIFY(!challenge.isEmpty());
        QVERIFY(!challenge.contains('='));
        QVERIFY(!challenge.contains('+'));
        QVERIFY(!challenge.contains('/'));

        // Validar determinismo del challenge
        QByteArray expectedHash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
        QString expectedChallenge = QString::fromLatin1(expectedHash.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
        QCOMPARE(challenge, expectedChallenge);
    }

    void testClientIdConfiguration() {
        KeychainStore keychain;
        AuthManager auth(&keychain);
        QCOMPARE(auth.clientId(), QStringLiteral("ae0de121-412b-438c-a604-6614b1d28ab3"));

        auth.setClientId("custom-client-id-123");
        QCOMPARE(auth.clientId(), QStringLiteral("custom-client-id-123"));
    }

    void testInitialState() {
        KeychainStore keychain;
        AuthManager auth(&keychain);
        QCOMPARE(auth.isAuthenticating(), false);
    }

    void testLogoutCleansState() {
        KeychainStore keychain;
        AuthManager auth(&keychain);
        auth.logout();
        QCOMPARE(auth.isAuthenticated(), false);
        QCOMPARE(auth.userEmail(), QString());
        QCOMPARE(auth.userName(), QString());
    }
};

QTEST_MAIN(tst_AuthManager)
#include "tst_AuthManager.moc"
