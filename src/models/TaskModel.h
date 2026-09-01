#pragma once
#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QVariantMap>
#include "LocalRepository.h"

class GraphClient;
class SyncEngine;

class TaskModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString currentListId READ currentListId WRITE setCurrentListId NOTIFY currentListIdChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(QVariantMap selectedTask READ selectedTask NOTIFY selectedTaskChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles { 
        IdRole = Qt::UserRole + 1, 
        ListIdRole,
        TitleRole, 
        BodyRole,
        IsCompletedRole, 
        IsMyDayRole, 
        DueDateRole, 
        ImportanceRole, 
        ReminderAtRole, 
        RecurrenceRole,
        StepsRole,
        HasStepsRole,
        StepsCountRole,
        CompletedStepsCountRole
    };
    
    explicit TaskModel(LocalRepository *repo, GraphClient *graph = nullptr, SyncEngine *sync = nullptr, QObject *parent = nullptr);
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setGraphClient(GraphClient *graph) { m_graph = graph; }
    void setSyncEngine(SyncEngine *sync) { m_sync = sync; }

    QString currentListId() const { return m_currentListId; }
    void setCurrentListId(const QString &id);

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int idx);

    QVariantMap selectedTask() const;

    Q_INVOKABLE void loadTasks();
    Q_INVOKABLE void addTask(const QString &title, const QString &dueDate = "", const QString &reminderAt = "", const QString &recurrence = "none");
    Q_INVOKABLE void addTaskWithSteps(const QString &title, const QString &dueDate, const QString &reminderAt, const QString &recurrence, const QStringList &steps);
    Q_INVOKABLE void toggleTaskCompletion(int row);
    Q_INVOKABLE void toggleTaskImportance(int row);
    Q_INVOKABLE void toggleTaskMyDay(int row);
    Q_INVOKABLE void updateTaskTitle(int row, const QString &title);
    Q_INVOKABLE void updateTaskBody(int row, const QString &body);
    Q_INVOKABLE void updateTaskDueDate(int row, const QString &dueDate);
    Q_INVOKABLE void updateTaskReminder(int row, const QString &reminderAt);
    Q_INVOKABLE void deleteTask(int row);

    Q_INVOKABLE void addStep(int row, const QString &stepTitle);
    Q_INVOKABLE void toggleStep(int row, int stepIndex);
    Q_INVOKABLE void deleteStep(int row, int stepIndex);
    
signals:
    void currentListIdChanged();
    void selectedIndexChanged();
    void selectedTaskChanged();
    void countChanged();

private:
    LocalRepository *m_repo = nullptr;
    GraphClient     *m_graph = nullptr;
    SyncEngine      *m_sync = nullptr;
    QString m_currentListId;
    int m_selectedIndex = -1;
    QList<Task> m_tasks;
    QFutureWatcher<QList<Task>> m_watcher;

    void notifyRowChanged(int row);
};
