#include "NotificationService.h"
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QVariantMap>
#include <QStringList>
#include <QDebug>

NotificationService::NotificationService(QObject *parent)
    : QObject(parent)
    , m_connection(QDBusConnection::sessionBus())
    , m_service(QStringLiteral("org.freedesktop.Notifications"))
    , m_path(QStringLiteral("/org/freedesktop/Notifications"))
    , m_interface(QStringLiteral("org.freedesktop.Notifications"))
{
}

NotificationService::NotificationService(const QDBusConnection &connection, QObject *parent)
    : QObject(parent)
    , m_connection(connection)
    , m_service(QStringLiteral("org.freedesktop.Notifications"))
    , m_path(QStringLiteral("/org/freedesktop/Notifications"))
    , m_interface(QStringLiteral("org.freedesktop.Notifications"))
{
}

void NotificationService::setCustomTarget(const QString &service, const QString &path, const QString &interface) {
    m_service = service;
    m_path = path;
    m_interface = interface;
}

bool NotificationService::send(const QString &title,
                               const QString &body,
                               const QString &iconName,
                               int timeoutMs)
{
    if (!m_connection.isConnected()) {
        qWarning() << "[NotificationService] D-Bus session bus no conectado";
        return false;
    }

    QDBusInterface iface(m_service, m_path, m_interface, m_connection);
    if (!iface.isValid()) {
        qWarning() << "[NotificationService] Interfaz D-Bus no válida:" << iface.lastError().message();
        return false;
    }

    QDBusMessage reply = iface.call(
        QStringLiteral("Notify"),
        QStringLiteral("OmaDo"),
        0u,
        iconName,
        title,
        body,
        QStringList(),
        QVariantMap(),
        timeoutMs
    );

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "[NotificationService] Error enviando notificación D-Bus:" << reply.errorMessage();
        return false;
    }

    return true;
}
