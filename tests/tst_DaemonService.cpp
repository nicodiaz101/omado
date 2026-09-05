#include <QtTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonArray>
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
};

QTEST_MAIN(tst_DaemonService)
#include "tst_DaemonService.moc"
