#pragma once

#include <QObject>
#include <QString>
#include <functional>

class KeychainStore : public QObject {
    Q_OBJECT
public:
    explicit KeychainStore(QObject *parent = nullptr);

    void writeKey(const QString &key, const QString &value, std::function<void(bool success, const QString &error)> callback = nullptr);
    void readKey(const QString &key, std::function<void(const QString &value, bool success)> callback);
    void deleteKey(const QString &key, std::function<void(bool success)> callback = nullptr);

    void clearAll(std::function<void()> callback = nullptr);

    static constexpr const char *kServiceName = "io.omarchy.OmaDo";
    static constexpr const char *kKeyAccessToken = "ms_access_token";
    static constexpr const char *kKeyRefreshToken = "ms_refresh_token";
    static constexpr const char *kKeyTokenExpiry = "ms_token_expiry";
    static constexpr const char *kKeyUserEmail = "ms_user_email";
    static constexpr const char *kKeyUserName = "ms_user_name";
};
