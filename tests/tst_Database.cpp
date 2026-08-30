#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "core/Database.h"

class tst_Database : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Inicializar Database
        Database db;
        QVERIFY(db.initialize());
    }

    void testSchemaAndTables() {
        QSqlDatabase db = QSqlDatabase::database();
        QVERIFY(db.isOpen());

        QStringList tables = db.tables();
        QVERIFY(tables.contains("task_lists"));
        QVERIFY(tables.contains("tasks"));
        QVERIFY(tables.contains("task_steps"));
        QVERIFY(tables.contains("schema_version"));
    }

    void testSchemaVersion() {
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery q("SELECT MAX(version) FROM schema_version", db);
        QVERIFY(q.next());
        int version = q.value(0).toInt();
        QVERIFY(version >= 2);
    }

    void testSpecialListsSeed() {
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery q("SELECT id, display_name, is_special FROM task_lists WHERE is_special = 1", db);
        
        QSet<QString> specialIds;
        while (q.next()) {
            specialIds.insert(q.value("id").toString());
        }
        
        QVERIFY(specialIds.contains("special-myday"));
        QVERIFY(specialIds.contains("special-schedule"));
        QVERIFY(specialIds.contains("special-tasks"));
    }

    void testForeignKeyConstraints() {
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery q(db);
        // Intentar insertar una tarea con list_id inexistente
        q.prepare("INSERT INTO tasks (id, list_id, title, created_at) VALUES ('fk_test', 'non_existent_list', 'Test', '2026-01-01')");
        bool ok = q.exec();
        QVERIFY(!ok); // Debe fallar por violación de foreign key
    }
};

QTEST_MAIN(tst_Database)
#include "tst_Database.moc"
