#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QtDBus/QDBusConnection>

class NotificationService : public QObject {
    Q_OBJECT
public:
    explicit NotificationService(QObject *parent = nullptr);
    explicit NotificationService(const QDBusConnection &connection, QObject *parent = nullptr);
    ~NotificationService() override;

    // Envía una notificación básica mediante org.freedesktop.Notifications
    virtual bool send(const QString &title,
                      const QString &body,
                      const QString &iconName = QStringLiteral("omado"),
                      int timeoutMs = 5000);

    // Envía una notificación avanzada con acciones, hints y retorna el id asignado (0 si falló)
    virtual uint sendNotification(const QString &title,
                                  const QString &body,
                                  const QStringList &actions = QStringList(),
                                  const QVariantMap &hints = QVariantMap(),
                                  const QString &iconName = QStringLiteral("omado"),
                                  int timeoutMs = 5000);

    // Cierra una notificación activa por ID
    virtual void closeNotification(uint id);

    // Configuración para pruebas y mocking
    void setCustomTarget(const QString &service, const QString &path, const QString &interface);

signals:
    void actionInvoked(uint id, const QString &actionKey);
    void notificationClosed(uint id, uint reason);

private slots:
    void onActionInvoked(uint id, const QString &actionKey);
    void onNotificationClosed(uint id, uint reason);

private:
    void setupDBusConnections();

    QDBusConnection m_connection;
    QString m_service;
    QString m_path;
    QString m_interface;
};
