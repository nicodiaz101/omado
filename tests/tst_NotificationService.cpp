#include <QtTest>
#include <QSignalSpy>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusContext>
#include <QStandardPaths>
#include "core/NotificationService.h"

class MockNotificationServer : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

public:
    explicit MockNotificationServer(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    Q_SCRIPTABLE uint Notify(const QString &appName,
                             uint replacesId,
                             const QString &appIcon,
                             const QString &summary,
                             const QString &body,
                             const QStringList &actions,
                             const QVariantMap &hints,
                             int expireTimeout)
    {
        Q_UNUSED(replacesId);
        Q_UNUSED(expireTimeout);
        lastAppName = appName;
        lastAppIcon = appIcon;
        lastSummary = summary;
        lastBody = body;
        lastActions = actions;
        lastHints = hints;
        notifyCount++;
        return ++nextId;
    }

    Q_SCRIPTABLE void CloseNotification(uint id) {
        lastClosedId = id;
        closedCount++;
        emit NotificationClosed(id, 3);
    }

    void simulateAction(uint id, const QString &actionKey) {
        emit ActionInvoked(id, actionKey);
    }

signals:
    Q_SCRIPTABLE void ActionInvoked(uint id, const QString &actionKey);
    Q_SCRIPTABLE void NotificationClosed(uint id, uint reason);

public:
    uint nextId = 100;
    int notifyCount = 0;
    int closedCount = 0;
    uint lastClosedId = 0;
    QString lastAppName;
    QString lastAppIcon;
    QString lastSummary;
    QString lastBody;
    QStringList lastActions;
    QVariantMap lastHints;
};

class tst_NotificationService : public QObject {
    Q_OBJECT

private:
    MockNotificationServer *m_server = nullptr;
    const QString m_serviceName = QStringLiteral("org.freedesktop.Notifications.TestMock");
    const QString m_path = QStringLiteral("/org/freedesktop/Notifications");
    const QString m_interface = QStringLiteral("org.freedesktop.Notifications");

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);

        m_server = new MockNotificationServer(this);
        QDBusConnection bus = QDBusConnection::sessionBus();
        QVERIFY(bus.isConnected());
        bus.registerService(m_serviceName);
        bus.registerObject(m_path, m_server, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);
    }

    void cleanupTestCase() {
        QDBusConnection bus = QDBusConnection::sessionBus();
        bus.unregisterObject(m_path);
        bus.unregisterService(m_serviceName);
    }

    void testSendBasic() {
        NotificationService service;
        service.setCustomTarget(m_serviceName, m_path, m_interface);

        bool ok = service.send("Prueba Resumen", "Prueba Cuerpo", "omado", 3000);
        QVERIFY(ok);
        QCOMPARE(m_server->lastAppName, QStringLiteral("OmaDo"));
        QCOMPARE(m_server->lastSummary, QStringLiteral("Prueba Resumen"));
        QCOMPARE(m_server->lastBody, QStringLiteral("Prueba Cuerpo"));
        QCOMPARE(m_server->lastAppIcon, QStringLiteral("omado"));
        QCOMPARE(m_server->lastHints.value("desktop-entry").toString(), QStringLiteral("omado"));
    }

    void testSendNotificationAdvancedAndSignals() {
        NotificationService service;
        service.setCustomTarget(m_serviceName, m_path, m_interface);

        QStringList actions;
        actions << "complete" << "Completar" << "snooze-15" << "Posponer";

        QVariantMap hints;
        hints.insert("urgency", 2);

        QSignalSpy actionSpy(&service, &NotificationService::actionInvoked);
        QSignalSpy closedSpy(&service, &NotificationService::notificationClosed);

        uint id = service.sendNotification("Tarea Urgente", "Cuerpo con detalle", actions, hints, "omado", 0);
        QVERIFY(id > 0);
        QCOMPARE(m_server->lastSummary, QStringLiteral("Tarea Urgente"));
        QCOMPARE(m_server->lastActions, actions);
        QCOMPARE(m_server->lastHints.value("urgency").toInt(), 2);
        QCOMPARE(m_server->lastHints.value("desktop-entry").toString(), QStringLiteral("omado"));

        // Simular emisión de señal ActionInvoked desde el servidor D-Bus
        m_server->simulateAction(id, "complete");
        QTRY_COMPARE_WITH_TIMEOUT(actionSpy.count(), 1, 2000);
        QCOMPARE(actionSpy.first().at(0).toUInt(), id);
        QCOMPARE(actionSpy.first().at(1).toString(), QStringLiteral("complete"));

        // Probar closeNotification y señal NotificationClosed
        service.closeNotification(id);
        QTRY_COMPARE_WITH_TIMEOUT(closedSpy.count(), 1, 2000);
        QCOMPARE(closedSpy.first().at(0).toUInt(), id);
        QCOMPARE(m_server->lastClosedId, id);
    }
};

QTEST_MAIN(tst_NotificationService)
#include "tst_NotificationService.moc"
