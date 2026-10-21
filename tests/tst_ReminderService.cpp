#include <QtTest>
#include <QSignalSpy>
#include <QStandardPaths>
#include "core/Database.h"
#include "core/ReminderService.h"
#include "core/NotificationService.h"
#include "models/LocalRepository.h"

class MockNotificationService : public NotificationService {
    Q_OBJECT
public:
    explicit MockNotificationService(QObject *parent = nullptr) : NotificationService(parent) {}

    uint sendNotification(const QString &title,
                          const QString &body,
                          const QStringList &actions = QStringList(),
                          const QVariantMap &hints = QVariantMap(),
                          const QString &iconName = QStringLiteral("omado"),
                          int timeoutMs = 5000) override
    {
        Q_UNUSED(iconName);
        Q_UNUSED(timeoutMs);
        lastTitle = title;
        lastBody = body;
        lastActions = actions;
        lastHints = hints;
        sendCount++;
        uint id = ++nextId;
        lastAssignedId = id;
        return id;
    }

    void simulateAction(uint id, const QString &actionKey) {
        emit actionInvoked(id, actionKey);
    }

    int sendCount = 0;
    uint nextId = 50;
    uint lastAssignedId = 0;
    QString lastTitle;
    QString lastBody;
    QStringList lastActions;
    QVariantMap lastHints;
};

class tst_ReminderService : public QObject {
    Q_OBJECT

private:
    Database *m_db = nullptr;
    LocalRepository *m_repo = nullptr;
    MockNotificationService *m_notifier = nullptr;
    ReminderService *m_service = nullptr;

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        m_db = new Database(this);
        QVERIFY(m_db->initialize());
        m_repo = new LocalRepository(this);
        m_notifier = new MockNotificationService(this);
        m_service = new ReminderService(m_repo, m_notifier, this);
    }

    void cleanupTestCase() {
        delete m_service;
        delete m_notifier;
        delete m_repo;
        delete m_db;
    }

    void testReminderTriggerAndDbUpdate() {
        TaskList list = m_repo->createList("Reminder Test List").result();

        Task task;
        task.listId = list.id;
        task.title = "Recordar comprar cafe";
        task.body = "Café de especialidad en https://cafe.example.com";
        task.importance = "high";
        task.reminderAt = QDateTime::currentDateTime().addSecs(-30); // En el pasado
        task.isCompleted = false;
        task.reminded = false;

        Task created = m_repo->createTask(task).result();
        QVERIFY(!created.id.isEmpty());

        // Agregar subtareas para verificar que aparecen en el body
        m_repo->addStep(created.id, "Comprar grano colombiano").result();
        m_repo->addStep(created.id, "Pedir factura").result();

        QSignalSpy spy(m_service, &ReminderService::reminderTriggered);

        m_service->checkReminders();

        // Esperar a que el worker asíncrono termine
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 3000);
        QCOMPARE(m_notifier->sendCount, 1);
        QVERIFY(m_notifier->lastTitle.contains("Recordar comprar cafe"));
        QVERIFY(m_notifier->lastBody.contains("https://cafe.example.com"));
        QVERIFY(m_notifier->lastBody.contains("• Comprar grano colombiano"));
        QVERIFY(m_notifier->lastActions.contains("complete"));
        QVERIFY(m_notifier->lastActions.contains("snooze-15"));
        QCOMPARE(m_notifier->lastHints.value("urgency").toInt(), 2); // Alta prioridad = urgencia crítica

        // Verificar que la tarea quedó marcada como reminded=1 en DB
        QTRY_VERIFY_WITH_TIMEOUT([&]() {
            Task updated = m_repo->fetchTaskById(created.id).result();
            return updated.reminded == true;
        }(), 3000);

        // Probar acción interactiva 'complete'
        uint notifId = m_notifier->lastAssignedId;
        QSignalSpy actionProcessedSpy(m_service, &ReminderService::reminderActionProcessed);
        m_notifier->simulateAction(notifId, "complete");

        QTRY_COMPARE_WITH_TIMEOUT(actionProcessedSpy.count(), 1, 3000);
        QCOMPARE(actionProcessedSpy.first().at(1).toString(), QStringLiteral("complete"));

        // Verificar que se completó en la base de datos
        QTRY_VERIFY_WITH_TIMEOUT([&]() {
            Task updated = m_repo->fetchTaskById(created.id).result();
            return updated.isCompleted == true;
        }(), 3000);

        // Limpieza
        m_repo->deleteTask(created.id).result();
        m_repo->deleteList(list.id).result();
    }

    void testReminderSnoozeAction() {
        TaskList list = m_repo->createList("Snooze Test List").result();

        Task task;
        task.listId = list.id;
        task.title = "Reunión de equipo";
        task.reminderAt = QDateTime::currentDateTime().addSecs(-10);
        task.isCompleted = false;
        task.reminded = false;

        Task created = m_repo->createTask(task).result();
        QVERIFY(!created.id.isEmpty());

        QSignalSpy spy(m_service, &ReminderService::reminderTriggered);
        m_service->checkReminders();
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 3000);

        uint notifId = m_notifier->lastAssignedId;
        QSignalSpy actionProcessedSpy(m_service, &ReminderService::reminderActionProcessed);
        m_notifier->simulateAction(notifId, "snooze-15");

        QTRY_COMPARE_WITH_TIMEOUT(actionProcessedSpy.count(), 1, 3000);
        QCOMPARE(actionProcessedSpy.first().at(1).toString(), QStringLiteral("snooze-15"));

        // Verificar que reminder_at se movió a futuro y reminded volvió a 0
        QTRY_VERIFY_WITH_TIMEOUT([&]() {
            Task updated = m_repo->fetchTaskById(created.id).result();
            return updated.reminded == false && updated.reminderAt > QDateTime::currentDateTime().addSecs(10 * 60);
        }(), 3000);

        // Limpieza
        m_repo->deleteTask(created.id).result();
        m_repo->deleteList(list.id).result();
    }
};

QTEST_MAIN(tst_ReminderService)
#include "tst_ReminderService.moc"
