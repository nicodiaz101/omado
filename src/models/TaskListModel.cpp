#include "TaskListModel.h"
#include "GraphClient.h"
#include "../core/SyncEngine.h"
#include <QUuid>

TaskListModel::TaskListModel(LocalRepository *repo, GraphClient *graph, SyncEngine *sync, QObject *parent)
    : QAbstractListModel(parent), m_repo(repo), m_graph(graph), m_sync(sync) {
    connect(&m_watcher, &QFutureWatcher<QList<TaskList>>::finished, this, [this]() {
        beginResetModel();
        m_lists = m_watcher.result();
        endResetModel();
    });
}

int TaskListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_lists.count();
}

QVariant TaskListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_lists.count()) return QVariant();
    const TaskList &l = m_lists.at(index.row());
    switch(role) {
        case IdRole: return l.id;
        case DisplayNameRole: return l.displayName;
        case IsSpecialRole: return l.isSpecial;
        case SortOrderRole: return l.sortOrder;
        case PendingCountRole: return 0;
        default: return QVariant();
    }
}

QHash<int, QByteArray> TaskListModel::roleNames() const {
    return {
        {IdRole, "id"},
        {DisplayNameRole, "displayName"},
        {IsSpecialRole, "isSpecial"},
        {SortOrderRole, "sortOrder"},
        {PendingCountRole, "pendingCount"}
    };
}

void TaskListModel::loadLists() {
    if (m_repo) {
        m_watcher.setFuture(m_repo->fetchLists());
    }
}

void TaskListModel::createList(const QString &name) {
    if (!m_repo || name.trimmed().isEmpty()) return;
    
    TaskList l;
    l.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    l.displayName = name.trimmed();
    l.isSpecial = false;
    l.sortOrder = 10;
    
    beginInsertRows(QModelIndex(), m_lists.count(), m_lists.count());
    m_lists.append(l);
    endInsertRows();

    m_repo->createList(l.displayName);

    if (m_graph) {
        m_graph->createList(l.displayName, [this, localId = l.id](bool ok, const QJsonObject &obj, const QString &) {
            if (ok && m_repo) {
                QString remoteId = obj.value("id").toString();
                m_repo->updateListRemoteId(localId, remoteId);
            }
        });
    }
    if (m_sync) {
        m_sync->scheduleSync(500);
    }
}

void TaskListModel::deleteList(const QString &id) {
    if (!m_repo || id.isEmpty()) return;
    for (int i = 0; i < m_lists.count(); ++i) {
        if (m_lists[i].id == id && !m_lists[i].isSpecial) {
            QString remoteId = m_lists[i].remoteId;
            beginRemoveRows(QModelIndex(), i, i);
            m_lists.removeAt(i);
            endRemoveRows();
            m_repo->deleteList(id);

            if (m_graph && !remoteId.isEmpty()) {
                m_graph->deleteList(remoteId, nullptr);
            }
            if (m_sync) {
                m_sync->scheduleSync(500);
            }
            break;
        }
    }
}

