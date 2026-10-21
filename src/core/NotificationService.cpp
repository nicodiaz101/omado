#include "NotificationService.h"
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
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
    setupDBusConnections();
}

NotificationService::NotificationService(const QDBusConnection &connection, QObject *parent)
    : QObject(parent)
    , m_connection(connection)
    , m_service(QStringLiteral("org.freedesktop.Notifications"))
    , m_path(QStringLiteral("/org/freedesktop/Notifications"))
    , m_interface(QStringLiteral("org.freedesktop.Notifications"))
{
    setupDBusConnections();
}

NotificationService::~NotificationService() {
    if (m_connection.isConnected()) {
        m_connection.disconnect(
            m_service,
            m_path,
            m_interface,
            QStringLiteral("ActionInvoked"),
            this,
            SLOT(onActionInvoked(uint,QString))
        );
        m_connection.disconnect(
            m_service,
            m_path,
            m_interface,
            QStringLiteral("NotificationClosed"),
            this,
            SLOT(onNotificationClosed(uint,uint))
        );
    }
}

void NotificationService::setupDBusConnections() {
    if (!m_connection.isConnected()) {
        return;
    }

    m_connection.connect(
        m_service,
        m_path,
        m_interface,
        QStringLiteral("ActionInvoked"),
        this,
        SLOT(onActionInvoked(uint,QString))
    );

    m_connection.connect(
        m_service,
        m_path,
        m_interface,
        QStringLiteral("NotificationClosed"),
        this,
        SLOT(onNotificationClosed(uint,uint))
    );
}

void NotificationService::setCustomTarget(const QString &service, const QString &path, const QString &interface) {
    if (m_connection.isConnected()) {
        m_connection.disconnect(
            m_service,
            m_path,
            m_interface,
            QStringLiteral("ActionInvoked"),
            this,
            SLOT(onActionInvoked(uint,QString))
        );
        m_connection.disconnect(
            m_service,
            m_path,
            m_interface,
            QStringLiteral("NotificationClosed"),
            this,
            SLOT(onNotificationClosed(uint,uint))
        );
    }

    m_service = service;
    m_path = path;
    m_interface = interface;

    setupDBusConnections();
}

void NotificationService::onActionInvoked(uint id, const QString &actionKey) {
    emit actionInvoked(id, actionKey);
}

void NotificationService::onNotificationClosed(uint id, uint reason) {
    emit notificationClosed(id, reason);
}

bool NotificationService::send(const QString &title,
                               const QString &body,
                               const QString &iconName,
                               int timeoutMs)
{
    uint id = sendNotification(title, body, QStringList(), QVariantMap(), iconName, timeoutMs);
    return id > 0;
}

uint NotificationService::sendNotification(const QString &title,
                                          const QString &body,
                                          const QStringList &actions,
                                          const QVariantMap &hints,
                                          const QString &iconName,
                                          int timeoutMs)
{
    if (!m_connection.isConnected()) {
        qWarning() << "[NotificationService] D-Bus session bus no conectado";
        return 0;
    }

    QDBusInterface iface(m_service, m_path, m_interface, m_connection);
    if (!iface.isValid()) {
        qWarning() << "[NotificationService] Interfaz D-Bus no válida:" << iface.lastError().message();
        return 0;
    }

    // Asegurar que el desktop-entry apunte a omado para agrupación correcta en Omapager
    QVariantMap finalHints = hints;
    if (!finalHints.contains(QStringLiteral("desktop-entry"))) {
        finalHints.insert(QStringLiteral("desktop-entry"), QStringLiteral("omado"));
    }

    QDBusReply<uint> reply = iface.call(
        QStringLiteral("Notify"),
        QStringLiteral("OmaDo"),
        0u,
        iconName,
        title,
        body,
        actions,
        finalHints,
        timeoutMs
    );

    if (!reply.isValid()) {
        qWarning() << "[NotificationService] Error enviando notificación D-Bus:" << reply.error().message();
        return 0;
    }

    return reply.value();
}

void NotificationService::closeNotification(uint id) {
    if (!m_connection.isConnected() || id == 0) {
        return;
    }

    QDBusInterface iface(m_service, m_path, m_interface, m_connection);
    if (iface.isValid()) {
        iface.call(QDBus::NoBlock, QStringLiteral("CloseNotification"), id);
    }
}
