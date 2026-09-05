#include <QtTest>
#include "core/Database.h"
#include "models/LocalRepository.h"

class tst_LocalRepository : public QObject {
    Q_OBJECT

private:
    Database *m_db = nullptr;
    LocalRepository *m_repo = nullptr;

private slots:
    void initTestCase() {
        m_db = new Database(this);
        QVERIFY(m_db->initialize());
        m_repo = new LocalRepository(this);
    }

    void cleanupTestCase() {
        delete m_repo;
        delete m_db;
    }

    void testListCrud() {
        // 1. Create list
        auto createFuture = m_repo->createList("Proyectos Omarchy");
        TaskList created = createFuture.result();
        QVERIFY(!created.id.isEmpty());
        QCOMPARE(created.displayName, QStringLiteral("Proyectos Omarchy"));

        // 2. Fetch lists
        auto fetchFuture = m_repo->fetchLists();
        QList<TaskList> lists = fetchFuture.result();
        bool found = false;
        for (const TaskList &l : lists) {
            if (l.id == created.id) {
                found = true;
                break;
            }
        }
        QVERIFY(found);

        // 3. Update list
        created.displayName = "Proyectos Omarchy Editado";
        auto updateFuture = m_repo->updateList(created);
        QVERIFY(updateFuture.result());

        // 4. Delete list
        auto deleteFuture = m_repo->deleteList(created.id);
        QVERIFY(deleteFuture.result());
    }

    void testTaskCrudAndToggle() {
        // Crear lista de prueba
        TaskList list = m_repo->createList("Lista de Tareas Test").result();

        // 1. Create task
        Task t;
        t.listId = list.id;
        t.title = "Completar Hito 2 de OmaDo";
        t.body = "Detalles de implementación";
        t.isCompleted = false;
        t.isMyDay = true;
        t.importance = "high";
        t.dueDate = QDate::currentDate();
        t.reminderAt = QDateTime::currentDateTime().addSecs(3600);

        Task createdTask = m_repo->createTask(t).result();
        QVERIFY(!createdTask.id.isEmpty());
        QCOMPARE(createdTask.title, QStringLiteral("Completar Hito 2 de OmaDo"));
        QCOMPARE(createdTask.isCompleted, false);
        QCOMPARE(createdTask.reminded, false);

        // 2. Fetch task by id
        Task fetched = m_repo->fetchTaskById(createdTask.id).result();
        QCOMPARE(fetched.id, createdTask.id);
        QCOMPARE(fetched.title, createdTask.title);

        // 3. Toggle task
        bool toggled = m_repo->toggleTask(createdTask.id, true).result();
        QVERIFY(toggled);

        Task toggledTask = m_repo->fetchTaskById(createdTask.id).result();
        QCOMPARE(toggledTask.isCompleted, true);

        // 4. Update task
        toggledTask.title = "Título modificado";
        bool updated = m_repo->updateTask(toggledTask).result();
        QVERIFY(updated);

        Task updatedTask = m_repo->fetchTaskById(createdTask.id).result();
        QCOMPARE(updatedTask.title, QStringLiteral("Título modificado"));

        // 5. Delete task
        bool deleted = m_repo->deleteTask(createdTask.id).result();
        QVERIFY(deleted);

        // Limpiar lista
        m_repo->deleteList(list.id).result();
    }

    void testTaskSteps() {
        TaskList list = m_repo->createList("Lista Steps").result();
        Task t;
        t.listId = list.id;
        t.title = "Tarea con Subtareas";
        Task createdTask = m_repo->createTask(t).result();

        // 1. Add step
        TaskStep step = m_repo->addStep(createdTask.id, "Paso 1").result();
        QVERIFY(!step.id.isEmpty());
        QCOMPARE(step.title, QStringLiteral("Paso 1"));

        // 2. Update step
        step.title = "Paso 1 modificado";
        step.isCompleted = true;
        bool stepUpdated = m_repo->updateStep(step).result();
        QVERIFY(stepUpdated);

        // 3. Delete step
        bool stepDeleted = m_repo->deleteStep(step.id).result();
        QVERIFY(stepDeleted);

        // Limpieza
        m_repo->deleteTask(createdTask.id).result();
        m_repo->deleteList(list.id).result();
    }

    void testPendingCountsAndReminders() {
        TaskList list = m_repo->createList("Lista Reminders").result();

        Task t1;
        t1.listId = list.id;
        t1.title = "Tarea Pasada";
        t1.reminderAt = QDateTime::currentDateTime().addSecs(-60); // En el pasado
        t1.isCompleted = false;
        t1.reminded = false;
        Task createdT1 = m_repo->createTask(t1).result();

        // Verificar conteos
        int pending = m_repo->getPendingCount(list.id).result();
        QVERIFY(pending >= 1);

        // Verificar pending reminders
        QList<Task> reminders = m_repo->getPendingReminders().result();
        bool foundT1 = false;
        for (const Task &r : reminders) {
            if (r.id == createdT1.id) {
                foundT1 = true;
                break;
            }
        }
        QVERIFY(foundT1);

        // Marcar reminded
        bool markOk = m_repo->markReminded(createdT1.id, true).result();
        QVERIFY(markOk);

        // Ya no debe figurar en pending reminders
        QList<Task> remindersAfter = m_repo->getPendingReminders().result();
        bool foundAfter = false;
        for (const Task &r : remindersAfter) {
            if (r.id == createdT1.id) {
                foundAfter = true;
                break;
            }
        }
        QVERIFY(!foundAfter);

        // Limpieza
        m_repo->deleteTask(createdT1.id).result();
        m_repo->deleteList(list.id).result();
    }

    void testSyncTrackingAndDirtyCheck() {
        TaskList list = m_repo->createList("Lista Sync Test").result();

        // 1. Crear tarea local y simular que se sincronizó con Graph
        Task t;
        t.listId = list.id;
        t.title = "Tarea para Sincronizar";
        t.isCompleted = false;
        Task created = m_repo->createTask(t).result();
        QVERIFY(!created.id.isEmpty());

        QString testRemoteId = QStringLiteral("graph-task-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        bool linked = m_repo->updateTaskRemoteId(created.id, testRemoteId).result();
        QVERIFY(linked);

        Task synced = m_repo->fetchTaskById(created.id).result();
        QCOMPARE(synced.remoteId, testRemoteId);
        QVERIFY(synced.syncedAt.isValid());
        QVERIFY(synced.updatedAt.isValid());

        // 2. Modificar la tarea localmente (marcar completada)
        QThread::msleep(50);
        bool toggled = m_repo->toggleTask(created.id, true).result();
        QVERIFY(toggled);

        Task dirtyTask = m_repo->fetchTaskById(created.id).result();
        QCOMPARE(dirtyTask.isCompleted, true);
        QVERIFY(dirtyTask.updatedAt > dirtyTask.syncedAt);

        // 3. Simular que Graph aún la tiene sin completar y el daemon hace upsertRemoteTask
        Task staleRemoteTask;
        staleRemoteTask.remoteId = testRemoteId;
        staleRemoteTask.title = "Tarea para Sincronizar";
        staleRemoteTask.isCompleted = false;

        Task preserved = m_repo->upsertRemoteTask(list.id, staleRemoteTask).result();
        QCOMPARE(preserved.isCompleted, true); // No debe ser pisada

        Task checkDb = m_repo->fetchTaskById(created.id).result();
        QCOMPARE(checkDb.isCompleted, true); // Sigue completada en la base de datos

        // 4. Marcar como sincronizada
        bool marked = m_repo->markTaskSynced(created.id).result();
        QVERIFY(marked);

        // 5. Eliminar tarea localmente y comprobar que se guarda en deleted_tasks
        bool deleted = m_repo->deleteTask(created.id).result();
        QVERIFY(deleted);

        QList<LocalRepository::DeletedTaskRecord> deletedRecords = m_repo->fetchDeletedTasks().result();
        bool foundDel = false;
        for (const auto &d : deletedRecords) {
            if (d.remoteId == testRemoteId) {
                foundDel = true;
                m_repo->removeDeletedTaskRecord(d.id).result();
                break;
            }
        }
        QVERIFY(foundDel);

        // Limpieza
        m_repo->deleteList(list.id).result();
    }

    void testRemindedStatePreservedOnSync() {
        TaskList list = m_repo->createList("Lista Reminded Test").result();

        Task t;
        t.listId = list.id;
        t.title = "Tarea con Recordatorio";
        t.reminderAt = QDateTime::currentDateTime().addSecs(-60); // 1 minuto en el pasado
        Task created = m_repo->createTask(t).result();

        QString testRemoteId = QStringLiteral("graph-reminder-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_repo->updateTaskRemoteId(created.id, testRemoteId).result();

        // Marcar como recordada
        m_repo->markReminded(created.id, true).result();
        Task remindedTask = m_repo->fetchTaskById(created.id).result();
        QCOMPARE(remindedTask.reminded, true);

        // Simular ciclo de sincronización desde Graph (Graph no envía el campo reminded)
        Task remoteTask;
        remoteTask.remoteId = testRemoteId;
        remoteTask.title = "Tarea con Recordatorio";
        remoteTask.reminderAt = t.reminderAt;
        remoteTask.reminded = false; // Como viene de Graph

        Task synced = m_repo->upsertRemoteTask(list.id, remoteTask).result();
        QCOMPARE(synced.reminded, true); // Debe preservar reminded = true

        Task checkDb = m_repo->fetchTaskById(created.id).result();
        QCOMPARE(checkDb.reminded, true); // En la base de datos debe seguir reminded = true

        // Limpieza
        m_repo->deleteTask(created.id).result();
        m_repo->deleteList(list.id).result();
    }
};

QTEST_MAIN(tst_LocalRepository)
#include "tst_LocalRepository.moc"
