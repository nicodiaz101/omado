#pragma once

#include <QObject>
#include <QString>
#include <QtDBus/QDBusConnection>

class NotificationService : public QObject {
    Q_OBJECT
public:
    explicit NotificationService(QObject *parent = nullptr);
    explicit NotificationService(const QDBusConnection &connection, QObject *parent = nullptr);

    // Envía una notificación mediante org.freedesktop.Notifications
    virtual bool send(const QString &title,
                      const QString &body,
                      const QString &iconName = QStringLiteral("checkbox-checked"),
                      int timeoutMs = 5000);

    // Configuración para pruebas y mocking
    void setCustomTarget(const QString &service, const QString &path, const QString &interface);

private:
    QDBusConnection m_connection;
    QString m_service;
    QString m_path;
    QString m_interface;
};
