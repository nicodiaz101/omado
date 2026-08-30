#include "LocalRepository.h"
#include <QtConcurrent>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDebug>
#include <QThread>
#include <QStandardPaths>

LocalRepository::LocalRepository(QObject *parent) : QObject(parent) {}

static QSqlDatabase getThreadDb() {
    QString connectionName = QString::number((quintptr)QThread::currentThreadId());
    if (QSqlDatabase::contains(connectionName)) {
        return QSqlDatabase::database(connectionName);
    }
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/omado.db");
    if (!db.open()) {
        qWarning() << "[LocalRepository] Failed to open thread DB:" << db.lastError().text();
    }
    return db;
}

static TaskList mapToTaskList(const QSqlQuery &q) {
    TaskList l;
    l.id = q.value("id").toString();
    l.displayName = q.value("display_name").toString();
    l.isSpecial = q.value("is_special").toBool();
    l.sortOrder = q.value("sort_order").toInt();
    l.remoteId = q.value("remote_id").toString();
    return l;
}

static QList<TaskStep> loadStepsForTask(const QString &taskId, QSqlDatabase &db) {
    QList<TaskStep> steps;
    QSqlQuery q(db);
    q.prepare("SELECT id, task_id, title, is_completed, sort_order FROM task_steps WHERE task_id = :task_id ORDER BY sort_order ASC");
    q.bindValue(":task_id", taskId);
    if (q.exec()) {
        while (q.next()) {
            TaskStep s;
            s.id = q.value("id").toString();
            s.taskId = q.value("task_id").toString();
            s.title = q.value("title").toString();
            s.isCompleted = q.value("is_completed").toBool();
            s.sortOrder = q.value("sort_order").toInt();
            steps.append(s);
        }
    }
    return steps;
}

static Task mapToTask(const QSqlQuery &q, QSqlDatabase &db) {
    Task t;
    t.id = q.value("id").toString();
    t.listId = q.value("list_id").toString();
    t.title = q.value("title").toString();
    t.body = q.value("body").toString();
    t.isCompleted = q.value("is_completed").toBool();
    t.isMyDay = q.value("is_my_day").toBool();
    t.importance = q.value("importance").toString();
    QString dueDateStr = q.value("due_date").toString();
    if (!dueDateStr.isEmpty()) t.dueDate = QDate::fromString(dueDateStr, Qt::ISODate);
    QString reminderStr = q.value("reminder_at").toString();
    if (!reminderStr.isEmpty()) {
        t.reminderAt = QDateTime::fromString(reminderStr, Qt::ISODate);
        if (!t.reminderAt.isValid()) {
            t.reminderAt = QDateTime::fromString(reminderStr, "yyyy-MM-dd HH:mm:ss");
        }
        if (!t.reminderAt.isValid()) {
            t.reminderAt = QDateTime::fromString(reminderStr, "yyyy-MM-dd HH:mm");
        }
        if (t.reminderAt.isValid()) {
            t.reminderAt = t.reminderAt.toLocalTime();
        }
    }
    t.reminded = q.value("reminded").toBool();
    t.recurrence = q.value("recurrence").toString();
    t.sortOrder = q.value("sort_order").toInt();
    t.createdAt = QDateTime::fromString(q.value("created_at").toString(), Qt::ISODate);
    t.remoteId = q.value("remote_id").toString();
    t.steps = loadStepsForTask(t.id, db);
    return t;
}

QFuture<QList<TaskList>> LocalRepository::fetchLists() {
    return QtConcurrent::run([]() {
        QList<TaskList> lists;
        QSqlDatabase db = getThreadDb();
        QSqlQuery q("SELECT id, display_name, is_special, sort_order, remote_id FROM task_lists ORDER BY sort_order ASC, created_at ASC", db);
        while (q.next()) lists.append(mapToTaskList(q));
        return lists;
    });
}

QFuture<QList<Task>> LocalRepository::fetchTasks(const QString &listId) {
    return QtConcurrent::run([listId]() {
        QList<Task> tasks;
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("SELECT * FROM tasks WHERE list_id = :list_id ORDER BY is_completed ASC, sort_order DESC, created_at DESC");
        q.bindValue(":list_id", listId);
        if (q.exec()) {
            while (q.next()) tasks.append(mapToTask(q, db));
        } else {
            qWarning() << "[LocalRepository] fetchTasks error:" << q.lastError().text();
        }
        return tasks;
    });
}

QFuture<QList<Task>> LocalRepository::fetchMyDayTasks() {
    return QtConcurrent::run([]() {
        QList<Task> tasks;
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("SELECT * FROM tasks WHERE is_my_day = 1 OR due_date = :today ORDER BY is_completed ASC, sort_order DESC, created_at DESC");
        q.bindValue(":today", QDate::currentDate().toString(Qt::ISODate));
        if (q.exec()) {
            while (q.next()) tasks.append(mapToTask(q, db));
        } else {
            qWarning() << "[LocalRepository] fetchMyDayTasks error:" << q.lastError().text();
        }
        return tasks;
    });
}

QFuture<QList<Task>> LocalRepository::fetchScheduleTasks() {
    return QtConcurrent::run([]() {
        QList<Task> tasks;
        QSqlDatabase db = getThreadDb();
        QSqlQuery q("SELECT * FROM tasks WHERE due_date IS NOT NULL AND due_date != '' ORDER BY due_date ASC, is_completed ASC, sort_order DESC", db);
        while (q.next()) tasks.append(mapToTask(q, db));
        return tasks;
    });
}

QFuture<QList<Task>> LocalRepository::fetchAllTasks() {
    return QtConcurrent::run([]() {
        QList<Task> tasks;
        QSqlDatabase db = getThreadDb();
        QSqlQuery q("SELECT * FROM tasks ORDER BY is_completed ASC, sort_order DESC, created_at DESC", db);
        while (q.next()) tasks.append(mapToTask(q, db));
        return tasks;
    });
}

QFuture<TaskList> LocalRepository::createList(const QString &name) {
    return QtConcurrent::run([name]() {
        TaskList l;
        l.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        l.displayName = name;
        l.isSpecial = false;
        
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("INSERT INTO task_lists (id, display_name, is_special, sort_order, created_at) VALUES (:id, :name, 0, 10, :created)");
        q.bindValue(":id", l.id);
        q.bindValue(":name", l.displayName);
        q.bindValue(":created", QDateTime::currentDateTime().toString(Qt::ISODate));
        if (!q.exec()) {
            qWarning() << "[LocalRepository] createList error:" << q.lastError().text();
        }
        return l;
    });
}

QFuture<Task> LocalRepository::createTask(const Task &task) {
    return QtConcurrent::run([task]() {
        Task t = task;
        if (t.id.isEmpty()) t.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!t.createdAt.isValid()) t.createdAt = QDateTime::currentDateTime();
        
        QSqlDatabase db = getThreadDb();
        db.transaction();

        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO tasks (id, list_id, title, body, is_completed, is_my_day, importance, due_date, reminder_at, reminded, recurrence, sort_order, created_at)
            VALUES (:id, :list_id, :title, :body, :is_completed, :is_my_day, :importance, :due_date, :reminder_at, :reminded, :recurrence, :sort_order, :created_at)
        )");
        q.bindValue(":id", t.id);
        q.bindValue(":list_id", t.listId);
        q.bindValue(":title", t.title);
        q.bindValue(":body", t.body.isNull() ? QStringLiteral("") : t.body);
        q.bindValue(":is_completed", t.isCompleted ? 1 : 0);
        q.bindValue(":is_my_day", t.isMyDay ? 1 : 0);
        q.bindValue(":importance", t.importance.isEmpty() ? QStringLiteral("normal") : t.importance);
        q.bindValue(":due_date", t.dueDate.isValid() ? t.dueDate.toString(Qt::ISODate) : QVariant());
        q.bindValue(":reminder_at", t.reminderAt.isValid() ? t.reminderAt.toString(Qt::ISODate) : QVariant());
        q.bindValue(":reminded", t.reminded ? 1 : 0);
        q.bindValue(":recurrence", t.recurrence.isEmpty() ? QStringLiteral("none") : t.recurrence);
        q.bindValue(":sort_order", t.sortOrder);
        q.bindValue(":created_at", t.createdAt.toString(Qt::ISODate));
        
        bool ok = q.exec();
        if (!ok) {
            qWarning() << "[LocalRepository] createTask error:" << q.lastError().text();
            db.rollback();
            return t;
        }

        // Insert initial steps if any
        for (int i = 0; i < t.steps.count(); ++i) {
            TaskStep &s = t.steps[i];
            if (s.id.isEmpty()) s.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            s.taskId = t.id;
            s.sortOrder = i;

            QSqlQuery qs(db);
            qs.prepare("INSERT INTO task_steps (id, task_id, title, is_completed, sort_order) VALUES (:id, :task_id, :title, :is_completed, :sort_order)");
            qs.bindValue(":id", s.id);
            qs.bindValue(":task_id", s.taskId);
            qs.bindValue(":title", s.title.isNull() ? QStringLiteral("") : s.title);
            qs.bindValue(":is_completed", s.isCompleted ? 1 : 0);
            qs.bindValue(":sort_order", s.sortOrder);
            qs.exec();
        }

        db.commit();
        return t;
    });
}

QFuture<bool> LocalRepository::updateTask(const Task &task) {
    return QtConcurrent::run([task]() {
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare(R"(
            UPDATE tasks SET 
                title = :title, 
                body = :body, 
                is_completed = :is_completed, 
                is_my_day = :is_my_day, 
                importance = :importance,
                due_date = :due_date, 
                reminder_at = :reminder_at, 
                reminded = :reminded,
                recurrence = :recurrence, 
                sort_order = :sort_order
            WHERE id = :id
        )");
        q.bindValue(":id", task.id);
        q.bindValue(":title", task.title);
        q.bindValue(":body", task.body.isNull() ? QStringLiteral("") : task.body);
        q.bindValue(":is_completed", task.isCompleted ? 1 : 0);
        q.bindValue(":is_my_day", task.isMyDay ? 1 : 0);
        q.bindValue(":importance", task.importance.isEmpty() ? QStringLiteral("normal") : task.importance);
        q.bindValue(":due_date", task.dueDate.isValid() ? task.dueDate.toString(Qt::ISODate) : QVariant());
        q.bindValue(":reminder_at", task.reminderAt.isValid() ? task.reminderAt.toString(Qt::ISODate) : QVariant());
        q.bindValue(":reminded", task.reminded ? 1 : 0);
        q.bindValue(":recurrence", task.recurrence.isEmpty() ? QStringLiteral("none") : task.recurrence);
        q.bindValue(":sort_order", task.sortOrder);
        
        bool ok = q.exec();
        if (!ok) {
            qWarning() << "[LocalRepository] updateTask error:" << q.lastError().text();
        }
        return ok;
    });
}

QFuture<Task> LocalRepository::fetchTaskById(const QString &id) {
    return QtConcurrent::run([id]() {
        Task t;
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("SELECT * FROM tasks WHERE id = :id");
        q.bindValue(":id", id);
        if (q.exec() && q.next()) {
            t = mapToTask(q, db);
        }
        return t;
    });
}

QFuture<bool> LocalRepository::toggleTask(const QString &taskId, bool completed) {
    return QtConcurrent::run([taskId, completed]() {
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("UPDATE tasks SET is_completed = :completed, completed_at = :completed_at WHERE id = :id");
        q.bindValue(":completed", completed ? 1 : 0);
        q.bindValue(":completed_at", completed ? QDateTime::currentDateTime().toString(Qt::ISODate) : QVariant());
        q.bindValue(":id", taskId);
        bool ok = q.exec();
        if (!ok) {
            qWarning() << "[LocalRepository] toggleTask error:" << q.lastError().text();
        }
        return ok;
    });
}

QFuture<bool> LocalRepository::markReminded(const QString &taskId, bool reminded) {
    return QtConcurrent::run([taskId, reminded]() {
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("UPDATE tasks SET reminded = :reminded WHERE id = :id");
        q.bindValue(":reminded", reminded ? 1 : 0);
        q.bindValue(":id", taskId);
        bool ok = q.exec();
        if (!ok) {
            qWarning() << "[LocalRepository] markReminded error:" << q.lastError().text();
        }
        return ok;
    });
}

QFuture<bool> LocalRepository::updateList(const TaskList &list) {
    return QtConcurrent::run([list]() {
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("UPDATE task_lists SET display_name = :name, sort_order = :sort WHERE id = :id");
        q.bindValue(":id", list.id);
        q.bindValue(":name", list.displayName);
        q.bindValue(":sort", list.sortOrder);
        return q.exec();
    });
}

QFuture<bool> LocalRepository::deleteList(const QString &id) {
    return QtConcurrent::run([id]() {
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("DELETE FROM task_lists WHERE id = :id AND is_special = 0");
        q.bindValue(":id", id);
        return q.exec();
    });
}

QFuture<bool> LocalRepository::deleteTask(const QString &id) {
    return QtConcurrent::run([id]() {
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("DELETE FROM tasks WHERE id = :id");
        q.bindValue(":id", id);
        bool ok = q.exec();
        if (!ok) {
            qWarning() << "[LocalRepository] deleteTask error:" << q.lastError().text();
        }
        return ok;
    });
}

QFuture<TaskStep> LocalRepository::addStep(const QString &taskId, const QString &title) {
    return QtConcurrent::run([taskId, title]() {
        TaskStep s;
        s.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        s.taskId = taskId;
        s.title = title;
        s.isCompleted = false;

        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("INSERT INTO task_steps (id, task_id, title, is_completed, sort_order) VALUES (:id, :task_id, :title, 0, 0)");
        q.bindValue(":id", s.id);
        q.bindValue(":task_id", s.taskId);
        q.bindValue(":title", s.title.isNull() ? QStringLiteral("") : s.title);
        if (!q.exec()) {
            qWarning() << "[LocalRepository] addStep error:" << q.lastError().text();
        }
        return s;
    });
}

QFuture<bool> LocalRepository::updateStep(const TaskStep &step) {
    return QtConcurrent::run([step]() {
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("UPDATE task_steps SET title = :title, is_completed = :is_completed WHERE id = :id");
        q.bindValue(":id", step.id);
        q.bindValue(":title", step.title.isNull() ? QStringLiteral("") : step.title);
        q.bindValue(":is_completed", step.isCompleted ? 1 : 0);
        return q.exec();
    });
}

QFuture<bool> LocalRepository::deleteStep(const QString &stepId) {
    return QtConcurrent::run([stepId]() {
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("DELETE FROM task_steps WHERE id = :id");
        q.bindValue(":id", stepId);
        return q.exec();
    });
}

QFuture<int> LocalRepository::getPendingCount(const QString &listId) {
    return QtConcurrent::run([listId]() {
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("SELECT COUNT(*) FROM tasks WHERE list_id = :list_id AND is_completed = 0");
        q.bindValue(":list_id", listId);
        if (q.exec() && q.next()) return q.value(0).toInt();
        return 0;
    });
}

QFuture<int> LocalRepository::getTotalPendingCount() {
    return QtConcurrent::run([]() {
        QSqlDatabase db = getThreadDb();
        QSqlQuery q("SELECT COUNT(*) FROM tasks WHERE is_completed = 0", db);
        if (q.next()) return q.value(0).toInt();
        return 0;
    });
}

QFuture<QList<Task>> LocalRepository::getPendingReminders() {
    return QtConcurrent::run([]() {
        QList<Task> tasks;
        QSqlDatabase db = getThreadDb();
        QSqlQuery q(db);
        q.prepare("SELECT * FROM tasks WHERE reminder_at IS NOT NULL AND reminder_at != '' AND reminder_at <= :now AND is_completed = 0 AND reminded = 0");
        q.bindValue(":now", QDateTime::currentDateTime().toString(Qt::ISODate));
        if (q.exec()) {
            while (q.next()) tasks.append(mapToTask(q, db));
        } else {
            qWarning() << "[LocalRepository] getPendingReminders error:" << q.lastError().text();
        }
        return tasks;
    });
}
