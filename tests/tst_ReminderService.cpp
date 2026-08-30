#include <QtTest>
#include <QSignalSpy>
#include "core/Database.h"
#include "core/ReminderService.h"
#include "core/NotificationService.h"
#include "models/LocalRepository.h"

class MockNotificationService : public NotificationService {
public:
    explicit MockNotificationService(QObject *parent = nullptr) : NotificationService(parent) {}

    bool send(const QString &title,
              const QString &body,
              const QString &iconName = QStringLiteral("checkbox-checked"),
              int timeoutMs = 5000) override {
        Q_UNUSED(iconName);
        Q_UNUSED(timeoutMs);
        lastTitle = title;
        lastBody = body;
        sendCount++;
        return true;
    }

    int sendCount = 0;
    QString lastTitle;
    QString lastBody;
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
        task.body = "Café de especialidad";
        task.reminderAt = QDateTime::currentDateTime().addSecs(-30); // En el pasado
        task.isCompleted = false;
        task.reminded = false;

        Task created = m_repo->createTask(task).result();
        QVERIFY(!created.id.isEmpty());

        QSignalSpy spy(m_service, &ReminderService::reminderTriggered);

        m_service->checkReminders();

        // Esperar a que el worker asíncrono termine
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 3000);
        QCOMPARE(m_notifier->sendCount, 1);
        QVERIFY(m_notifier->lastTitle.contains("Recordar comprar cafe"));

        // Verificar que la tarea quedó marcada como reminded=1 en DB
        QTRY_VERIFY_WITH_TIMEOUT([&]() {
            Task updated = m_repo->fetchTaskById(created.id).result();
            return updated.reminded == true;
        }(), 3000);

        // Limpieza
        m_repo->deleteTask(created.id).result();
        m_repo->deleteList(list.id).result();
    }
};

QTEST_MAIN(tst_ReminderService)
#include "tst_ReminderService.moc"
