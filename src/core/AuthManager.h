#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QTcpServer>
#include <functional>
#include "KeychainStore.h"

class AuthManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authStatusChanged)
    Q_PROPERTY(bool isAuthenticating READ isAuthenticating NOTIFY authStatusChanged)
    Q_PROPERTY(QString userEmail READ userEmail NOTIFY userChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY userChanged)
    Q_PROPERTY(QString clientId READ clientId WRITE setClientId NOTIFY clientIdChanged)

public:
    explicit AuthManager(KeychainStore *keychain, QObject *parent = nullptr);
    ~AuthManager() override;

    bool isAuthenticated() const { return m_isAuthenticated; }
    bool isAuthenticating() const { return m_isAuthenticating; }
    QString userEmail() const { return m_userEmail; }
    QString userName() const { return m_userName; }
    QString clientId() const { return m_clientId; }
    void setClientId(const QString &id);

    Q_INVOKABLE void startLogin();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void checkSavedCredentials();

    void ensureValidToken(std::function<void(const QString &accessToken, bool success)> callback);

    static QString generateCodeVerifier();
    static QString generateCodeChallenge(const QString &verifier);

signals:
    void authStatusChanged();
    void userChanged();
    void clientIdChanged();
    void authSuccess();
    void authFailed(const QString &error);

private slots:
    void handleNewConnection();

private:
    void exchangeCodeForTokens(const QString &code, const QString &redirectUri);
    void refreshAccessToken(std::function<void(bool success)> callback = nullptr);
    void fetchUserProfile();
    void stopLocalServer();

    KeychainStore          *m_keychain = nullptr;
    QNetworkAccessManager  *m_nam = nullptr;
    QTcpServer             *m_server = nullptr;

    QString                 m_clientId;
    QString                 m_codeVerifier;
    QString                 m_accessToken;
    QString                 m_refreshToken;
    QDateTime               m_tokenExpiry;
    QString                 m_userEmail;
    QString                 m_userName;
    QString                 m_state;

    bool                    m_isAuthenticated = false;
    bool                    m_isAuthenticating = false;

    static constexpr const char *kDefaultClientId = "ae0de121-412b-438c-a604-6614b1d28ab3";
    static constexpr const char *kAuthEndpoint    = "https://login.microsoftonline.com/common/oauth2/v2.0/authorize";
    static constexpr const char *kTokenEndpoint   = "https://login.microsoftonline.com/common/oauth2/v2.0/token";
    static constexpr const char *kGraphMeEndpoint = "https://graph.microsoft.com/v1.0/me";
    static constexpr const char *kScopes          = "Tasks.ReadWrite User.Read offline_access";
    static constexpr int         kDefaultPort     = 8080;
};
