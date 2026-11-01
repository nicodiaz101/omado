#pragma once

#include <QObject>
#include <QFuture>
#include <QThreadPool>
#include "TaskList.h"
#include "Task.h"
#include "TaskStep.h"

class LocalRepository : public QObject {
    Q_OBJECT
public:
    explicit LocalRepository(QObject *parent = nullptr);
    ~LocalRepository() override;

    QFuture<QList<TaskList>> fetchLists();
    QFuture<QList<Task>> fetchTasks(const QString &listId);
    QFuture<QList<Task>> fetchMyDayTasks();
    QFuture<QList<Task>> fetchScheduleTasks();
    QFuture<QList<Task>> fetchAllTasks();

    QFuture<TaskList> createList(const QString &name);
    QFuture<TaskList> createList(const TaskList &list);
    QFuture<bool> updateList(const TaskList &list);
    QFuture<bool> deleteList(const QString &id);

    QFuture<Task> createTask(const Task &task);
    QFuture<bool> updateTask(const Task &task);
    QFuture<bool> deleteTask(const QString &id, bool recordForRemoteSync = true);
    QFuture<Task> fetchTaskById(const QString &id);
    QFuture<bool> toggleTask(const QString &taskId, bool completed);
    QFuture<bool> markReminded(const QString &taskId, bool reminded = true);
    QFuture<bool> snoozeTaskReminder(const QString &taskId, int minutes = 15);
    QFuture<bool> rescheduleTaskForMyDay(const QString &taskId);
    QFuture<QList<Task>> fetchOverdueTasks();

    QFuture<TaskStep> addStep(const QString &taskId, const QString &title);
    QFuture<TaskStep> addStep(const TaskStep &step);
    QFuture<bool> updateStep(const TaskStep &step);
    QFuture<bool> deleteStep(const QString &stepId, bool recordForRemoteSync = true);
    QFuture<QList<TaskStep>> fetchStepsForTask(const QString &taskId);

    QFuture<int> getPendingCount(const QString &listId);
    QFuture<int> getTotalPendingCount();
    QFuture<QList<Task>> getPendingReminders();

    // Sincronización remota
    struct DeletedTaskRecord {
        QString id;
        QString remoteId;
        QString listRemoteId;
    };

    struct DeletedStepRecord {
        QString id;
        QString taskId;
        QString remoteId;
        QString taskRemoteId;
        QString listRemoteId;
    };

    QString getListRemoteId(const QString &localListId);
    QFuture<bool> updateListRemoteId(const QString &listId, const QString &remoteId);
    QFuture<bool> markListSynced(const QString &listId);
    QFuture<bool> updateTaskRemoteId(const QString &taskId, const QString &remoteId);
    QFuture<bool> markTaskSynced(const QString &taskId);
    QFuture<QList<DeletedTaskRecord>> fetchDeletedTasks();
    QFuture<bool> removeDeletedTaskRecord(const QString &id);
    QFuture<TaskList> fetchListByRemoteId(const QString &remoteId);
    QFuture<Task> fetchTaskByRemoteId(const QString &remoteId);
    QFuture<TaskList> upsertRemoteList(const QString &remoteId, const QString &displayName);
    QFuture<Task> upsertRemoteTask(const QString &localListId, const Task &task);

    // Subtasks remote sync
    QFuture<bool> updateStepRemoteId(const QString &stepId, const QString &remoteId);
    QFuture<TaskStep> upsertRemoteStep(const QString &taskId, const TaskStep &step);
    QFuture<bool> deleteStepByRemoteId(const QString &taskId, const QString &remoteStepId);
    QFuture<QList<DeletedStepRecord>> fetchDeletedSteps();
    QFuture<bool> removeDeletedStepRecord(const QString &id);

private:
    QThreadPool m_threadPool;
};
