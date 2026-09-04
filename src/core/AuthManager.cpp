#include "AuthManager.h"
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QUrlQuery>
#include <QTcpSocket>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QUuid>
#include <QTimer>
#include <QDebug>

AuthManager::AuthManager(KeychainStore *keychain, QObject *parent)
    : QObject(parent), m_keychain(keychain), m_nam(new QNetworkAccessManager(this)) {
    
    QString envId = QProcessEnvironment::systemEnvironment().value("OMADO_CLIENT_ID");
    m_clientId = !envId.trimmed().isEmpty() ? envId.trimmed() : QString::fromLatin1(kDefaultClientId);

    checkSavedCredentials([this](bool success) {
        if (!success) {
            // Si las credenciales fallan al iniciar (p.ej. keyring no desbloqueado en inicio del sistema),
            // reintentar cada 5s hasta 6 veces (30 segundos).
            auto *retryTimer = new QTimer(this);
            retryTimer->setInterval(5000);
            auto attemptCount = std::make_shared<int>(0);
            connect(retryTimer, &QTimer::timeout, this, [this, retryTimer, attemptCount]() {
                (*attemptCount)++;
                checkSavedCredentials([this, retryTimer, attemptCount](bool ok) {
                    if (ok || *attemptCount >= 6) {
                        retryTimer->stop();
                        retryTimer->deleteLater();
                    }
                });
            });
            retryTimer->start();
        }
    });
}

AuthManager::~AuthManager() {
    stopLocalServer();
}

void AuthManager::setClientId(const QString &id) {
    if (m_clientId == id || id.trimmed().isEmpty()) return;
    m_clientId = id.trimmed();
    emit clientIdChanged();
}

QString AuthManager::generateCodeVerifier() {
    // 32 bytes criptográficamente seguros convertidos a Base64URL sin padding
    QByteArray bytes;
    bytes.resize(32);
    for (int i = 0; i < 32; ++i) {
        bytes[i] = static_cast<char>(QRandomGenerator::securelySeeded().generate() & 0xFF);
    }
    return QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QString AuthManager::generateCodeChallenge(const QString &verifier) {
    QByteArray hash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

void AuthManager::checkSavedCredentials(std::function<void(bool success)> callback) {
    if (!m_keychain) {
        if (callback) callback(false);
        return;
    }

    m_keychain->readKey(KeychainStore::kKeyRefreshToken, [this, callback](const QString &refreshToken, bool success) {
        if (!success || refreshToken.trimmed().isEmpty()) {
            m_isAuthenticated = false;
            emit authStatusChanged();
            if (callback) callback(false);
            return;
        }

        m_refreshToken = refreshToken;
        m_isAuthenticated = true;

        m_keychain->readKey(KeychainStore::kKeyAccessToken, [this](const QString &accessToken, bool okToken) {
            if (okToken) m_accessToken = accessToken;
        });

        m_keychain->readKey(KeychainStore::kKeyTokenExpiry, [this](const QString &expiryStr, bool okExpiry) {
            if (okExpiry && !expiryStr.isEmpty()) {
                m_tokenExpiry = QDateTime::fromString(expiryStr, Qt::ISODate);
            }
        });

        m_keychain->readKey(KeychainStore::kKeyUserEmail, [this, callback](const QString &email, bool okEmail) {
            if (okEmail) m_userEmail = email;
            m_keychain->readKey(KeychainStore::kKeyUserName, [this, callback](const QString &name, bool okName) {
                if (okName) m_userName = name;
                emit userChanged();
                emit authStatusChanged();
                qDebug() << "[AuthManager] Sesión restaurada para:" << m_userEmail;
                if (callback) callback(true);
            });
        });
    });
}

void AuthManager::startLogin() {
    if (m_isAuthenticating) return;

    stopLocalServer();
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &AuthManager::handleNewConnection);

    quint16 port = kDefaultPort;
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        qWarning() << "[AuthManager] Puerto 8080 ocupado, buscando puerto dinámico...";
        if (!m_server->listen(QHostAddress::LocalHost, 0)) {
            qWarning() << "[AuthManager] No se pudo iniciar el servidor local para OAuth:" << m_server->errorString();
            emit authFailed(m_server->errorString());
            return;
        }
        port = m_server->serverPort();
    }

    m_codeVerifier = generateCodeVerifier();
    QString challenge = generateCodeChallenge(m_codeVerifier);
    QString redirectUri = QStringLiteral("http://localhost:%1").arg(port);
    m_state = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QUrl url(kAuthEndpoint);
    QUrlQuery query;
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("response_type", "code");
    query.addQueryItem("redirect_uri", redirectUri);
    query.addQueryItem("response_mode", "query");
    query.addQueryItem("scope", kScopes);
    query.addQueryItem("code_challenge", challenge);
    query.addQueryItem("code_challenge_method", "S256");
    query.addQueryItem("state", m_state);
    url.setQuery(query);

    m_isAuthenticating = true;
    emit authStatusChanged();

    qDebug() << "[AuthManager] Abriendo navegador para login con Client ID:" << m_clientId;
    QDesktopServices::openUrl(url);
}

void AuthManager::handleNewConnection() {
    if (!m_server) return;

    QTcpSocket *socket = m_server->nextPendingConnection();
    if (!socket) return;

    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        QByteArray data = socket->readAll();
        QString request = QString::fromUtf8(data);

        QString firstLine = request.split("\r\n").value(0);
        QString path = firstLine.split(" ").value(1);

        QUrl callbackUrl("http://localhost" + path);
        QUrlQuery query(callbackUrl);

        QString code = query.queryItemValue("code");
        QString error = query.queryItemValue("error");
        QString errorDesc = query.queryItemValue("error_description");
        QString state = query.queryItemValue("state");

        if (state != m_state || m_state.isEmpty()) {
            error = "invalid_state";
            errorDesc = "El parámetro state no coincide o es inválido. Posible ataque CSRF.";
            code.clear();
        }

        QString responseHtml = QStringLiteral(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Connection: close\r\n\r\n"
            "<!DOCTYPE html><html>"
            "<head><meta charset='utf-8'><title>OmaDo - Autenticación</title>"
            "<style>body{background:#181818;color:#f0f0f0;font-family:'iA Writer Mono',monospace,sans-serif;display:flex;align-items:center;justify-content:center;height:100vh;margin:0;}"
            ".card{background:#222;padding:32px 40px;border-radius:8px;border:1px solid #333;text-align:center;box-shadow:0 8px 24px rgba(0,0,0,0.5);}"
            "h2{color:#4CAF50;margin-top:0;}p{color:#aaa;line-height:1.6;}</style></head>"
            "<body><div class='card'>"
            "<h2>✓ Conexión con OmaDo exitosa</h2>"
            "<p>Ya podés cerrar esta pestaña del navegador y volver a la aplicación.</p>"
            "</div></body></html>"
        );

        if (!error.isEmpty()) {
            responseHtml = QStringLiteral(
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Connection: close\r\n\r\n"
                "<!DOCTYPE html><html>"
                "<head><meta charset='utf-8'><title>OmaDo - Error</title>"
                "<style>body{background:#181818;color:#f0f0f0;font-family:sans-serif;display:flex;align-items:center;justify-content:center;height:100vh;margin:0;}"
                ".card{background:#222;padding:32px;border-radius:8px;border:1px solid #ff5555;text-align:center;}"
                "h2{color:#ff5555;}p{color:#aaa;}</style></head>"
                "<body><div class='card'>"
                "<h2>✕ Error de autenticación</h2>"
                "<p>%1</p>"
                "</div></body></html>"
            ).arg(errorDesc.isEmpty() ? error : errorDesc);
        }

        socket->write(responseHtml.toUtf8());
        socket->flush();
        socket->disconnectFromHost();

        quint16 port = m_server ? m_server->serverPort() : kDefaultPort;
        QString redirectUri = QStringLiteral("http://localhost:%1").arg(port);
        stopLocalServer();

        if (!code.isEmpty()) {
            qDebug() << "[AuthManager] Código de autorización recibido con éxito";
            exchangeCodeForTokens(code, redirectUri);
        } else {
            qWarning() << "[AuthManager] Error en callback de autenticación:" << error << errorDesc;
            m_isAuthenticating = false;
            emit authStatusChanged();
            emit authFailed(!errorDesc.isEmpty() ? errorDesc : error);
        }
    });
}

void AuthManager::exchangeCodeForTokens(const QString &code, const QString &redirectUri) {
    QNetworkRequest req(QUrl(QString::fromLatin1(kTokenEndpoint)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery body;
    body.addQueryItem("client_id", m_clientId);
    body.addQueryItem("grant_type", "authorization_code");
    body.addQueryItem("code", code);
    body.addQueryItem("redirect_uri", redirectUri);
    body.addQueryItem("code_verifier", m_codeVerifier);

    QNetworkReply *reply = m_nam->post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_isAuthenticating = false;

        if (reply->error() != QNetworkReply::NoError) {
            QString err = reply->readAll();
            qWarning() << "[AuthManager] Error al canjear tokens:" << reply->errorString() << err;
            emit authStatusChanged();
            emit authFailed(reply->errorString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        m_accessToken = obj.value("access_token").toString();
        m_refreshToken = obj.value("refresh_token").toString();
        int expiresIn = obj.value("expires_in").toInt(3600);
        m_tokenExpiry = QDateTime::currentDateTime().addSecs(expiresIn - 60);

        if (m_keychain) {
            m_keychain->writeKey(KeychainStore::kKeyAccessToken, m_accessToken);
            m_keychain->writeKey(KeychainStore::kKeyRefreshToken, m_refreshToken);
            m_keychain->writeKey(KeychainStore::kKeyTokenExpiry, m_tokenExpiry.toString(Qt::ISODate));
        }

        m_isAuthenticated = true;
        emit authStatusChanged();

        qDebug() << "[AuthManager] Tokens obtenidos. Consultando perfil de usuario...";
        fetchUserProfile();
    });
}

void AuthManager::fetchUserProfile() {
    if (m_accessToken.isEmpty()) return;

    QNetworkRequest req(QUrl(QString::fromLatin1(kGraphMeEndpoint)));
    req.setRawHeader("Authorization", "Bearer " + m_accessToken.toUtf8());

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[AuthManager] No se pudo obtener el perfil de Graph:" << reply->errorString();
            emit authSuccess();
            return;
        }

        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        m_userName = obj.value("displayName").toString();
        m_userEmail = obj.value("mail").toString();
        if (m_userEmail.isEmpty()) {
            m_userEmail = obj.value("userPrincipalName").toString();
        }

        if (m_keychain) {
            m_keychain->writeKey(KeychainStore::kKeyUserEmail, m_userEmail);
            m_keychain->writeKey(KeychainStore::kKeyUserName, m_userName);
        }

        emit userChanged();
        emit authSuccess();
        qDebug() << "[AuthManager] Autenticación completada para:" << m_userName << "<" << m_userEmail << ">";
    });
}

void AuthManager::refreshAccessToken(std::function<void(bool success)> callback) {
    if (m_refreshToken.isEmpty()) {
        if (callback) callback(false);
        return;
    }

    QNetworkRequest req(QUrl(QString::fromLatin1(kTokenEndpoint)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery body;
    body.addQueryItem("client_id", m_clientId);
    body.addQueryItem("grant_type", "refresh_token");
    body.addQueryItem("refresh_token", m_refreshToken);

    QNetworkReply *reply = m_nam->post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[AuthManager] Error refrescando token:" << reply->errorString();
            if (callback) callback(false);
            return;
        }

        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        m_accessToken = obj.value("access_token").toString();
        if (obj.contains("refresh_token")) {
            m_refreshToken = obj.value("refresh_token").toString();
        }
        int expiresIn = obj.value("expires_in").toInt(3600);
        m_tokenExpiry = QDateTime::currentDateTime().addSecs(expiresIn - 60);

        if (m_keychain) {
            m_keychain->writeKey(KeychainStore::kKeyAccessToken, m_accessToken);
            m_keychain->writeKey(KeychainStore::kKeyRefreshToken, m_refreshToken);
            m_keychain->writeKey(KeychainStore::kKeyTokenExpiry, m_tokenExpiry.toString(Qt::ISODate));
        }

        qDebug() << "[AuthManager] Access token refrescado con éxito";
        if (callback) callback(true);
    });
}

void AuthManager::ensureValidToken(std::function<void(const QString &accessToken, bool success)> callback) {
    if (!m_isAuthenticated) {
        checkSavedCredentials([this, callback](bool restored) {
            if (!restored) {
                if (callback) callback(QString(), false);
                return;
            }
            ensureValidToken(callback);
        });
        return;
    }

    if (!m_accessToken.isEmpty() && m_tokenExpiry.isValid() && QDateTime::currentDateTime() < m_tokenExpiry) {
        if (callback) callback(m_accessToken, true);
        return;
    }

    refreshAccessToken([this, callback](bool ok) {
        if (ok && !m_accessToken.isEmpty()) {
            if (callback) callback(m_accessToken, true);
        } else {
            if (callback) callback(QString(), false);
        }
    });
}

void AuthManager::logout() {
    stopLocalServer();
    m_isAuthenticated = false;
    m_isAuthenticating = false;
    m_accessToken.clear();
    m_refreshToken.clear();
    m_tokenExpiry = QDateTime();
    m_userEmail.clear();
    m_userName.clear();

    if (m_keychain) {
        m_keychain->clearAll();
    }

    emit authStatusChanged();
    emit userChanged();
    qDebug() << "[AuthManager] Sesión cerrada";
}

void AuthManager::stopLocalServer() {
    if (m_server) {
        if (m_server->isListening()) {
            m_server->close();
        }
        m_server->deleteLater();
        m_server = nullptr;
    }
}
