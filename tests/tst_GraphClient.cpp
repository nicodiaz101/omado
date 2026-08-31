#include <QTest>
#include <QJsonObject>
#include <QSqlQuery>
#include "models/GraphClient.h"
#include "models/LocalRepository.h"
#include "core/Database.h"

class tst_GraphClient : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "test_graph_conn");
        db.setDatabaseName(":memory:");
        QVERIFY(db.open());

        QSqlQuery q(db);
        q.exec("CREATE TABLE task_lists (id TEXT PRIMARY KEY, display_name TEXT, is_special INT DEFAULT 0, sort_order INT DEFAULT 0, remote_id TEXT, created_at TEXT, synced_at TEXT)");
        q.exec("CREATE TABLE tasks (id TEXT PRIMARY KEY, list_id TEXT, title TEXT, body TEXT, is_completed INT DEFAULT 0, is_my_day INT DEFAULT 0, importance TEXT, due_date TEXT, reminder_at TEXT, reminded INT DEFAULT 0, recurrence TEXT, sort_order INT DEFAULT 0, created_at TEXT, completed_at TEXT, synced_at TEXT, remote_id TEXT)");
        q.exec("CREATE TABLE task_steps (id TEXT PRIMARY KEY, task_id TEXT, title TEXT, is_completed INT DEFAULT 0, sort_order INT DEFAULT 0, remote_id TEXT)");
    }

    void testTaskToJsonSerialization() {
        Task t;
        t.title = "Comprar café";
        t.body = "Tostado natural";
        t.isCompleted = true;
        t.importance = "high";
        t.dueDate = QDate(2026, 9, 1);
        t.reminderAt = QDateTime(QDate(2026, 9, 1), QTime(10, 30));

        QJsonObject json = GraphClient::taskToJson(t);
        QCOMPARE(json.value("title").toString(), QStringLiteral("Comprar café"));
        QCOMPARE(json.value("status").toString(), QStringLiteral("completed"));
        QCOMPARE(json.value("importance").toString(), QStringLiteral("high"));
        QCOMPARE(json.value("body").toObject().value("content").toString(), QStringLiteral("Tostado natural"));
        QCOMPARE(json.value("isReminderOn").toBool(), true);
        QVERIFY(json.value("dueDateTime").toObject().value("dateTime").toString().startsWith("2026-09-01"));
        QVERIFY(json.value("reminderDateTime").toObject().value("dateTime").toString().contains("2026-09-01"));
    }

    void testTaskToJsonWithoutReminder() {
        Task t;
        t.title = "Tarea simple";
        t.isCompleted = false;

        QJsonObject json = GraphClient::taskToJson(t);
        QCOMPARE(json.value("status").toString(), QStringLiteral("notStarted"));
        QCOMPARE(json.value("isReminderOn").toBool(), false);
    }
};

QTEST_MAIN(tst_GraphClient)
#include "tst_GraphClient.moc"
