#pragma once
#include <QAbstractListModel>
#include <QFutureWatcher>
#include "LocalRepository.h"

class GraphClient;

class TaskListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { 
        IdRole = Qt::UserRole + 1, 
        DisplayNameRole, 
        IsSpecialRole, 
        SortOrderRole, 
        PendingCountRole 
    };
    
    explicit TaskListModel(LocalRepository *repo, GraphClient *graph = nullptr, QObject *parent = nullptr);
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setGraphClient(GraphClient *graph) { m_graph = graph; }

    Q_INVOKABLE void loadLists();
    Q_INVOKABLE void createList(const QString &name);
    Q_INVOKABLE void deleteList(const QString &id);
    
private:
    LocalRepository *m_repo = nullptr;
    GraphClient     *m_graph = nullptr;
    QList<TaskList>  m_lists;
    QFutureWatcher<QList<TaskList>> m_watcher;
};

