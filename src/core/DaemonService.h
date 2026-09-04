#pragma once

#include <QObject>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusContext>
#include <QVariantMap>
#include <QList>
#include "models/TaskList.h"
#include "models/Task.h"

class LocalRepository;

class DaemonService : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.omarchy.OmaDo")

public:
    explicit DaemonService(LocalRepository *repo, QObject *parent = nullptr);
    explicit DaemonService(LocalRepository *repo, const QDBusConnection &connection, QObject *parent = nullptr);

    bool registerService();

public slots:
    Q_SCRIPTABLE QList<OmaDoListEntry> GetLists();
    Q_SCRIPTABLE QString GetTasksForToday();
    Q_SCRIPTABLE int GetPendingCount(const QString &listId);
    Q_SCRIPTABLE int GetTotalPendingCount();
    Q_SCRIPTABLE bool ToggleTask(const QString &taskId, bool completed);
    Q_SCRIPTABLE void RequestSync();

signals:
    Q_SCRIPTABLE void TasksChanged(const QString &listId);
    Q_SCRIPTABLE void TodayTasksChanged();
    void SyncRequested();

private:
    LocalRepository *m_repository;
    QDBusConnection  m_connection;
};
