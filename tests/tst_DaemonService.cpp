#include <QtTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include "core/Database.h"
#include "core/DaemonService.h"
#include "models/LocalRepository.h"

class tst_DaemonService : public QObject {
    Q_OBJECT

private:
    Database *m_db = nullptr;
    LocalRepository *m_repo = nullptr;
    DaemonService *m_daemon = nullptr;

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        m_db = new Database(this);
        QVERIFY(m_db->initialize());
        m_repo = new LocalRepository(this);
        m_daemon = new DaemonService(m_repo, this);
    }

    void cleanupTestCase() {
        delete m_daemon;
        delete m_repo;
        delete m_db;
    }

    void testGetLists() {
        TaskList created = m_repo->createList("Lista Daemon Test").result();
        
        QList<OmaDoListEntry> lists = m_daemon->GetLists();
        QVERIFY(!lists.isEmpty());

        bool found = false;
        for (const auto &entry : lists) {
            if (entry.id == created.id) {
                found = true;
                QCOMPARE(entry.displayName, QStringLiteral("Lista Daemon Test"));
                break;
            }
        }
        QVERIFY(found);

        m_repo->deleteList(created.id).result();
    }

    void testGetTasksForToday() {
        TaskList list = m_repo->createList("Lista Today Test").result();

        Task t;
        t.listId = list.id;
        t.title = "Tarea de Hoy Daemon";
        t.isMyDay = true;
        t.isCompleted = false;
        Task created = m_repo->createTask(t).result();

        QString jsonStr = m_daemon->GetTasksForToday();
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        QVariantList todayTasks = doc.array().toVariantList();
        bool found = false;
        for (const auto &item : todayTasks) {
            QVariantMap map = item.toMap();
            if (map.value("id").toString() == created.id) {
                found = true;
                QCOMPARE(map.value("title").toString(), QStringLiteral("Tarea de Hoy Daemon"));
                QCOMPARE(map.value("isMyDay").toBool(), true);
                break;
            }
        }
        QVERIFY(found);

        m_repo->deleteTask(created.id).result();
        m_repo->deleteList(list.id).result();
    }

    void testPendingCounts() {
        TaskList list = m_repo->createList("Count Test List").result();

        Task t;
        t.listId = list.id;
        t.title = "Tarea Pendiente";
        t.isCompleted = false;
        Task created = m_repo->createTask(t).result();

        int listCount = m_daemon->GetPendingCount(list.id);
        QCOMPARE(listCount, 1);

        int totalCount = m_daemon->GetTotalPendingCount();
        QVERIFY(totalCount >= 1);

        m_repo->deleteTask(created.id).result();
        m_repo->deleteList(list.id).result();
    }

    void testToggleTaskAndSignals() {
        TaskList list = m_repo->createList("Signal Test List").result();

        Task t;
        t.listId = list.id;
        t.title = "Tarea Toggle";
        t.isCompleted = false;
        Task created = m_repo->createTask(t).result();

        QSignalSpy spyTasksChanged(m_daemon, &DaemonService::TasksChanged);
        QSignalSpy spyTodayTasksChanged(m_daemon, &DaemonService::TodayTasksChanged);

        bool toggled = m_daemon->ToggleTask(created.id, true);
        QVERIFY(toggled);

        QCOMPARE(spyTasksChanged.count(), 1);
        QCOMPARE(spyTodayTasksChanged.count(), 1);

        // Verificar parámetro de señal TasksChanged
        QList<QVariant> arguments = spyTasksChanged.takeFirst();
        QCOMPARE(arguments.at(0).toString(), list.id);

        Task fetched = m_repo->fetchTaskById(created.id).result();
        QCOMPARE(fetched.isCompleted, true);

        m_repo->deleteTask(created.id).result();
        m_repo->deleteList(list.id).result();
    }

    void testDeleteTaskAndSignals() {
        TaskList list = m_repo->createList("Delete Test List").result();

        Task t;
        t.listId = list.id;
        t.title = "Tarea Delete";
        t.isCompleted = false;
        Task created = m_repo->createTask(t).result();

        QSignalSpy spyTasksChanged(m_daemon, &DaemonService::TasksChanged);
        QSignalSpy spyTodayTasksChanged(m_daemon, &DaemonService::TodayTasksChanged);

        bool deleted = m_daemon->DeleteTask(created.id);
        QVERIFY(deleted);

        QCOMPARE(spyTasksChanged.count(), 1);
        QCOMPARE(spyTodayTasksChanged.count(), 1);

        Task fetched = m_repo->fetchTaskById(created.id).result();
        QVERIFY(fetched.id.isEmpty());

        m_repo->deleteList(list.id).result();
    }

    void testOmarchy45Extensions() {
        // 1. Preview natural text
        QString previewJson = m_daemon->PreviewNaturalText("Comprar leche mañana 16:00 #Supermercado !");
        QVERIFY(!previewJson.isEmpty());
        QJsonDocument doc = QJsonDocument::fromJson(previewJson.toUtf8());
        QVERIFY(doc.isObject());
        QJsonObject obj = doc.object();
        QCOMPARE(obj.value("cleanTitle").toString(), QStringLiteral("Comprar leche"));
        QCOMPARE(obj.value("targetListName").toString(), QStringLiteral("Supermercado"));
        QCOMPARE(obj.value("importance").toString(), QStringLiteral("high"));
        QCOMPARE(obj.value("hasDueDate").toBool(), true);
        QCOMPARE(obj.value("hasReminder").toBool(), true);

        // 2. Create task from natural text via D-Bus
        QSignalSpy spyTasksChanged(m_daemon, &DaemonService::TasksChanged);
        QSignalSpy spyTodayTasksChanged(m_daemon, &DaemonService::TodayTasksChanged);

        QString taskId = m_daemon->CreateTaskFromNaturalText("Entregar reporte urgente hoy 17:00 #\"Reportes Trimestrales\" !!", true);
        QVERIFY(!taskId.isEmpty());
        QVERIFY(spyTasksChanged.count() >= 1);
        QVERIFY(spyTodayTasksChanged.count() >= 1);

        Task created = m_repo->fetchTaskById(taskId).result();
        QCOMPARE(created.title, QStringLiteral("Entregar reporte urgente"));
        QCOMPARE(created.importance, QStringLiteral("high"));
        QCOMPARE(created.isMyDay, true);

        // 3. SnoozeTask
        bool snoozeOk = m_daemon->SnoozeTask(taskId, 30);
        QVERIFY(snoozeOk);
        Task snoozed = m_repo->fetchTaskById(taskId).result();
        QVERIFY(snoozed.reminderAt > QDateTime::currentDateTime().addSecs(20 * 60));

        // 4. GetOverdueTasks & RescheduleTaskForMyDay
        // Marcar tarea vencida
        created.dueDate = QDate::currentDate().addDays(-2);
        created.isMyDay = false;
        m_repo->updateTask(created).result();

        QString overdueJson = m_daemon->GetOverdueTasks();
        QVERIFY(overdueJson.contains(taskId));

        bool reschedOk = m_daemon->RescheduleTaskForMyDay(taskId);
        QVERIFY(reschedOk);

        Task rescheduled = m_repo->fetchTaskById(taskId).result();
        QCOMPARE(rescheduled.isMyDay, true);
        QCOMPARE(rescheduled.dueDate, QDate::currentDate());

        // Limpieza
        m_repo->deleteTask(taskId).result();
        m_repo->deleteList(created.listId).result();
    }
};

QTEST_MAIN(tst_DaemonService)
#include "tst_DaemonService.moc"
