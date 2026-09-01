#pragma once

#include <QObject>
#include <QFuture>
#include "TaskList.h"
#include "Task.h"
#include "TaskStep.h"

class LocalRepository : public QObject {
    Q_OBJECT
public:
    explicit LocalRepository(QObject *parent = nullptr);

    QFuture<QList<TaskList>> fetchLists();
    QFuture<QList<Task>> fetchTasks(const QString &listId);
    QFuture<QList<Task>> fetchMyDayTasks();
    QFuture<QList<Task>> fetchScheduleTasks();
    QFuture<QList<Task>> fetchAllTasks();

    QFuture<TaskList> createList(const QString &name);
    QFuture<bool> updateList(const TaskList &list);
    QFuture<bool> deleteList(const QString &id);

    QFuture<Task> createTask(const Task &task);
    QFuture<bool> updateTask(const Task &task);
    QFuture<bool> deleteTask(const QString &id);
    QFuture<Task> fetchTaskById(const QString &id);
    QFuture<bool> toggleTask(const QString &taskId, bool completed);
    QFuture<bool> markReminded(const QString &taskId, bool reminded = true);

    QFuture<TaskStep> addStep(const QString &taskId, const QString &title);
    QFuture<bool> updateStep(const TaskStep &step);
    QFuture<bool> deleteStep(const QString &stepId);

    QFuture<int> getPendingCount(const QString &listId);
    QFuture<int> getTotalPendingCount();
    QFuture<QList<Task>> getPendingReminders();

    // Sincronización remota
    QString getListRemoteId(const QString &localListId);
    QFuture<bool> updateListRemoteId(const QString &listId, const QString &remoteId);
    QFuture<bool> updateTaskRemoteId(const QString &taskId, const QString &remoteId);
    QFuture<TaskList> fetchListByRemoteId(const QString &remoteId);
    QFuture<Task> fetchTaskByRemoteId(const QString &remoteId);
    QFuture<TaskList> upsertRemoteList(const QString &remoteId, const QString &displayName);
    QFuture<Task> upsertRemoteTask(const QString &localListId, const Task &task);
};

