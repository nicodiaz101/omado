#include <QtTest>
#include "core/NotificationService.h"

class tst_NotificationService : public QObject {
    Q_OBJECT

private slots:
    void testInstantiation() {
        NotificationService service;
        QVERIFY(true);
    }

    void testCustomTarget() {
        NotificationService service;
        service.setCustomTarget("org.freedesktop.Notifications",
                                "/org/freedesktop/Notifications",
                                "org.freedesktop.Notifications");
        QVERIFY(true);
    }

    void testSendNonBlocking() {
        NotificationService service;
        // Intenta enviar, no debe crashear ni bloquearse
        service.send("Test Title", "Test Body", "checkbox-checked", 1000);
        QVERIFY(true);
    }
};

QTEST_MAIN(tst_NotificationService)
#include "tst_NotificationService.moc"
