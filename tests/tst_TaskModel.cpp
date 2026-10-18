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

    void testAddTaskWithNaturalLanguageDate() {
        TaskList list = m_repo->createList("Lista NL Test").result();
        TaskModel model(m_repo);
        model.setCurrentListId(list.id);
        QTRY_COMPARE(model.rowCount(), 0);

        model.addTask("Comprar leche mañana");
        QCOMPARE(model.rowCount(), 1);

        QModelIndex idx = model.index(0, 0);
        QCOMPARE(model.data(idx, TaskModel::TitleRole).toString(), QStringLiteral("Comprar leche"));
        QCOMPARE(model.data(idx, TaskModel::DueDateRole).toString(), QDate::currentDate().addDays(1).toString(Qt::ISODate));
        QVERIFY(model.data(idx, TaskModel::ReminderAtRole).toString().isEmpty());

        m_repo->deleteList(list.id).result();
    }

    void testAddTaskWithNaturalLanguageDateTime() {
        TaskList list = m_repo->createList("Lista NL DateTime Test").result();
        TaskModel model(m_repo);
        model.setCurrentListId(list.id);
        QTRY_COMPARE(model.rowCount(), 0);

        model.addTask("Reunión con equipo mañana a las 15:30");
        QCOMPARE(model.rowCount(), 1);

        QModelIndex idx = model.index(0, 0);
        QCOMPARE(model.data(idx, TaskModel::TitleRole).toString(), QStringLiteral("Reunión con equipo"));
        QCOMPARE(model.data(idx, TaskModel::DueDateRole).toString(), QDate::currentDate().addDays(1).toString(Qt::ISODate));
        QString expectedReminder = QString("%1 15:30").arg(QDate::currentDate().addDays(1).toString(Qt::ISODate));
        QCOMPARE(model.data(idx, TaskModel::ReminderAtRole).toString(), expectedReminder);

        m_repo->deleteList(list.id).result();
    }

    void testParseInput() {
        TaskModel model(m_repo);
        QVariantMap map = model.parseInput("Dentista mañana a las 10:00");
        QCOMPARE(map["cleanTitle"].toString(), QStringLiteral("Dentista"));
        QCOMPARE(map["hasDueDate"].toBool(), true);
        QCOMPARE(map["hasReminder"].toBool(), true);
        QCOMPARE(map["dueDate"].toString(), QDate::currentDate().addDays(1).toString(Qt::ISODate));
        QCOMPARE(map["reminderTime"].toString(), QStringLiteral("10:00"));
    }

    void testSortByDueDate() {
        TaskList list = m_repo->createList("Lista Sort Test").result();

        Task tNoDate;
        tNoDate.listId = list.id;
        tNoDate.title = "Sin fecha";
        tNoDate.isCompleted = false;
        m_repo->createTask(tNoDate).result();

        Task tTomorrow;
        tTomorrow.listId = list.id;
        tTomorrow.title = "Mañana";
        tTomorrow.dueDate = QDate::currentDate().addDays(1);
        tTomorrow.isCompleted = false;
        m_repo->createTask(tTomorrow).result();

        Task tTodayAfternoon;
        tTodayAfternoon.listId = list.id;
        tTodayAfternoon.title = "Hoy tarde";
        tTodayAfternoon.dueDate = QDate::currentDate();
        tTodayAfternoon.reminderAt = QDateTime(QDate::currentDate(), QTime(17, 0));
        tTodayAfternoon.isCompleted = false;
        m_repo->createTask(tTodayAfternoon).result();

        Task tTodayMorning;
        tTodayMorning.listId = list.id;
        tTodayMorning.title = "Hoy temprano";
        tTodayMorning.dueDate = QDate::currentDate();
        tTodayMorning.reminderAt = QDateTime(QDate::currentDate(), QTime(9, 0));
        tTodayMorning.isCompleted = false;
        m_repo->createTask(tTodayMorning).result();

        Task tCompleted;
        tCompleted.listId = list.id;
        tCompleted.title = "Completada hoy";
        tCompleted.dueDate = QDate::currentDate();
        tCompleted.reminderAt = QDateTime(QDate::currentDate(), QTime(8, 0));
        tCompleted.isCompleted = true;
        m_repo->createTask(tCompleted).result();

        TaskModel model(m_repo);
        model.setHideCompleted(false);
        model.setSortByDueDate(false);

        QSignalSpy sortSpy(&model, &TaskModel::sortByDueDateChanged);

        model.setCurrentListId(list.id);
        QTRY_COMPARE(model.rowCount(), 5);

        // Activar orden por fecha de vencimiento y recordatorio
        model.setSortByDueDate(true);
        QCOMPARE(sortSpy.count(), 1);
        QCOMPARE(model.sortByDueDate(), true);
        QCOMPARE(model.rowCount(), 5);

        // Verificar el orden correcto:
        // 0: Hoy temprano (09:00 hoy)
        // 1: Hoy tarde (17:00 hoy)
        // 2: Mañana
        // 3: Sin fecha
        // 4: Completada hoy (las completadas van al final)
        QCOMPARE(model.data(model.index(0, 0), TaskModel::TitleRole).toString(), QStringLiteral("Hoy temprano"));
        QCOMPARE(model.data(model.index(1, 0), TaskModel::TitleRole).toString(), QStringLiteral("Hoy tarde"));
        QCOMPARE(model.data(model.index(2, 0), TaskModel::TitleRole).toString(), QStringLiteral("Mañana"));
        QCOMPARE(model.data(model.index(3, 0), TaskModel::TitleRole).toString(), QStringLiteral("Sin fecha"));
        QCOMPARE(model.data(model.index(4, 0), TaskModel::TitleRole).toString(), QStringLiteral("Completada hoy"));

        // Agregar una tarea con vencimiento intermedio (12:00 hoy)
        QString todayStr = QDate::currentDate().toString(Qt::ISODate);
        model.addTask("Almuerzo hoy a las 12:00", todayStr, QString("%1 12:00").arg(todayStr));
        QCOMPARE(model.rowCount(), 6);
        // Debe ubicarse en la posición 1 (entre 09:00 y 17:00)
        QCOMPARE(model.data(model.index(1, 0), TaskModel::TitleRole).toString(), QStringLiteral("Almuerzo hoy a las 12:00"));

        // Desactivar orden por vencimiento y volver al default
        model.setSortByDueDate(false);
        QCOMPARE(sortSpy.count(), 2);
        QCOMPARE(model.sortByDueDate(), false);

        // Limpieza
        m_repo->deleteList(list.id).result();
    }
};

QTEST_MAIN(tst_TaskModel)
#include "tst_TaskModel.moc"
