#pragma once
#include <QAbstractListModel>
#include <QFutureWatcher>
#include "LocalRepository.h"

class GraphClient;
class SyncEngine;

class TaskListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { 
        IdRole = Qt::UserRole + 1, 
        DisplayNameRole, 
        NameRole,
        IconRole,
        IsSpecialRole, 
        SortOrderRole, 
        PendingCountRole 
    };
    
    explicit TaskListModel(LocalRepository *repo, GraphClient *graph = nullptr, SyncEngine *sync = nullptr, QObject *parent = nullptr);
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setGraphClient(GraphClient *graph) { m_graph = graph; }
    void setSyncEngine(SyncEngine *sync) { m_sync = sync; }

    static void parseDisplayName(const QString &raw, QString &outEmoji, QString &outCleanName);

    Q_INVOKABLE void loadLists();
    Q_INVOKABLE void createList(const QString &name, const QString &icon = QString());
    Q_INVOKABLE void renameList(const QString &id, const QString &newName);
    Q_INVOKABLE void setListIcon(const QString &id, const QString &emoji);
    Q_INVOKABLE void deleteList(const QString &id);
    Q_INVOKABLE QString getListIcon(const QString &id) const;
    Q_INVOKABLE QString getListName(const QString &id) const;
    
private:
    LocalRepository *m_repo = nullptr;
    GraphClient     *m_graph = nullptr;
    SyncEngine      *m_sync = nullptr;
    QList<TaskList>  m_lists;
    QFutureWatcher<QList<TaskList>> m_watcher;
};

