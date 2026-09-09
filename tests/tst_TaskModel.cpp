#include <QtTest>
#include <QSignalSpy>
#include "core/Database.h"
#include "models/LocalRepository.h"
#include "models/TaskModel.h"

class tst_TaskModel : public QObject {
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

    void testHideCompletedFilter() {
        TaskList list = m_repo->createList("Lista Model Test").result();

        // Crear 1 tarea completada y 1 pendiente
        Task t1;
        t1.listId = list.id;
        t1.title = "Tarea Pendiente";
        t1.isCompleted = false;
        m_repo->createTask(t1).result();

        Task t2;
        t2.listId = list.id;
        t2.title = "Tarea Completada";
        t2.isCompleted = true;
        m_repo->createTask(t2).result();

        TaskModel model(m_repo);
        model.setHideCompleted(false);

        QSignalSpy countSpy(&model, &TaskModel::countChanged);
        QSignalSpy hideSpy(&model, &TaskModel::hideCompletedChanged);

        model.setCurrentListId(list.id);
        // Esperar a que el watcher termine
        QTRY_COMPARE(model.rowCount(), 2);
        QCOMPARE(model.property("count").toInt(), 2);

        // Activar hideCompleted
        model.setHideCompleted(true);
        QCOMPARE(hideSpy.count(), 1);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.property("count").toInt(), 1);

        // La única tarea visible debe ser "Tarea Pendiente"
        QModelIndex idx0 = model.index(0, 0);
        QCOMPARE(model.data(idx0, TaskModel::TitleRole).toString(), QStringLiteral("Tarea Pendiente"));
        QCOMPARE(model.data(idx0, TaskModel::IsCompletedRole).toBool(), false);

        // Marcar la tarea pendiente como completada
        model.setSelectedIndex(0);
        model.toggleTaskCompletion(0);

        // Ahora no debe quedar ninguna tarea visible
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.property("count").toInt(), 0);
        QCOMPARE(model.selectedIndex(), -1);

        // Desactivar hideCompleted
        model.setHideCompleted(false);
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.property("count").toInt(), 2);

        // Limpieza
        m_repo->deleteList(list.id).result();
    }
};

QTEST_MAIN(tst_TaskModel)
#include "tst_TaskModel.moc"
