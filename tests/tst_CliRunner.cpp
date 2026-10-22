#include <QtTest>
#include <QStandardPaths>
#include "core/Database.h"
#include "core/CliRunner.h"
#include "models/LocalRepository.h"

class tst_CliRunner : public QObject {
    Q_OBJECT

private:
    Database *m_db = nullptr;
    LocalRepository *m_repo = nullptr;

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        m_db = new Database(this);
        QVERIFY(m_db->initialize());
        m_repo = new LocalRepository(this);
    }

    void cleanupTestCase() {
        delete m_repo;
        delete m_db;
    }

    void testCliCommands() {
        // 1. omado add
        QStringList addArgs = {"add", "Comprar café de prueba", "--today"};
        int resAdd = CliRunner::run(addArgs, m_repo);
        QCOMPARE(resAdd, 0);

        // 2. omado list --today --json
        QStringList listArgs = {"list", "--today", "--json"};
        int resList = CliRunner::run(listArgs, m_repo);
        QCOMPARE(resList, 0);

        // 3. Buscar tarea creada para probar done y reschedule
        auto todayTasks = m_repo->fetchMyDayTasks().result();
        QVERIFY(!todayTasks.isEmpty());
        QString taskId = todayTasks.first().id;

        // 4. omado done
        QStringList doneArgs = {"done", taskId};
        int resDone = CliRunner::run(doneArgs, m_repo);
        QCOMPARE(resDone, 0);

        Task doneTask = m_repo->fetchTaskById(taskId).result();
        QCOMPARE(doneTask.isCompleted, true);

        // 5. omado reschedule
        QStringList reschedArgs = {"reschedule", taskId};
        int resResched = CliRunner::run(reschedArgs, m_repo);
        QCOMPARE(resResched, 0);

        // Limpieza
        m_repo->deleteTask(taskId).result();
    }
};

QTEST_MAIN(tst_CliRunner)
#include "tst_CliRunner.moc"
