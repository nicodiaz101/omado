#include "TaskListModel.h"
#include "GraphClient.h"
#include "../core/SyncEngine.h"
#include <QUuid>
#include <QRegularExpression>

TaskListModel::TaskListModel(LocalRepository *repo, GraphClient *graph, SyncEngine *sync, QObject *parent)
    : QAbstractListModel(parent), m_repo(repo), m_graph(graph), m_sync(sync) {
    connect(&m_watcher, &QFutureWatcher<QList<TaskList>>::finished, this, [this]() {
        beginResetModel();
        m_lists = m_watcher.result();
        endResetModel();
    });
}

void TaskListModel::parseDisplayName(const QString &raw, QString &outEmoji, QString &outCleanName) {
    static const QRegularExpression emojiRegex(
        QString::fromUtf8(R"(^(\p{Extended_Pictographic}(?:[\x{FE0E}\x{FE0F}\x{1F3FB}-\x{1F3FF}]|\x{200D}\p{Extended_Pictographic})*)\s*(.*)$)")
    );

    QString trimmed = raw.trimmed();
    QRegularExpressionMatch match = emojiRegex.match(trimmed);
    if (match.hasMatch()) {
        outEmoji = match.captured(1);
        outCleanName = match.captured(2).trimmed();
    } else {
        outEmoji = QString();
        outCleanName = trimmed;
    }
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
        case NameRole: {
            QString emoji, name;
            parseDisplayName(l.displayName, emoji, name);
            return !name.isEmpty() ? name : (emoji.isEmpty() ? l.displayName : QString());
        }
        case IconRole: {
            QString emoji, name;
            parseDisplayName(l.displayName, emoji, name);
            return emoji;
        }
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
        {NameRole, "name"},
        {IconRole, "icon"},
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

void TaskListModel::createList(const QString &name, const QString &icon) {
    if (!m_repo || name.trimmed().isEmpty()) return;
    
    QString finalDisplayName;
    QString trimmedName = name.trimmed();
    QString trimmedIcon = icon.trimmed();

    if (!trimmedIcon.isEmpty()) {
        finalDisplayName = trimmedIcon + " " + trimmedName;
    } else {
        finalDisplayName = trimmedName;
    }

    TaskList l;
    l.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    l.displayName = finalDisplayName;
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

void TaskListModel::renameList(const QString &id, const QString &newName) {
    if (!m_repo || id.isEmpty()) return;
    QString trimmed = newName.trimmed();
    if (trimmed.isEmpty()) return;

    for (int i = 0; i < m_lists.count(); ++i) {
        if (m_lists[i].id == id && !m_lists[i].isSpecial) {
            QString curEmoji, curName;
            parseDisplayName(m_lists[i].displayName, curEmoji, curName);

            // Verificar si el nuevo texto incluye ya un emoji al principio
            QString inputEmoji, inputName;
            parseDisplayName(trimmed, inputEmoji, inputName);

            QString finalEmoji = !inputEmoji.isEmpty() ? inputEmoji : curEmoji;
            QString finalName = !inputEmoji.isEmpty() ? inputName : trimmed;

            QString fullDisplayName;
            if (!finalEmoji.isEmpty() && !finalName.isEmpty()) {
                fullDisplayName = finalEmoji + " " + finalName;
            } else if (!finalEmoji.isEmpty()) {
                fullDisplayName = finalEmoji;
            } else {
                fullDisplayName = finalName;
            }

            m_lists[i].displayName = fullDisplayName;
            emit dataChanged(index(i), index(i), {DisplayNameRole, NameRole, IconRole});

            m_repo->updateList(m_lists[i]);

            if (m_graph && !m_lists[i].remoteId.isEmpty()) {
                m_graph->updateList(m_lists[i].remoteId, fullDisplayName, nullptr);
            }
            if (m_sync) {
                m_sync->scheduleSync(500);
            }
            break;
        }
    }
}

void TaskListModel::setListIcon(const QString &id, const QString &emoji) {
    if (!m_repo || id.isEmpty()) return;

    for (int i = 0; i < m_lists.count(); ++i) {
        if (m_lists[i].id == id && !m_lists[i].isSpecial) {
            QString curEmoji, curName;
            parseDisplayName(m_lists[i].displayName, curEmoji, curName);

            QString pureEmoji, remainder;
            parseDisplayName(emoji.trimmed(), pureEmoji, remainder);
            if (pureEmoji.isEmpty() && !emoji.trimmed().isEmpty()) {
                pureEmoji = emoji.trimmed();
            }

            QString fullDisplayName;
            if (!pureEmoji.isEmpty()) {
                fullDisplayName = curName.isEmpty() ? pureEmoji : (pureEmoji + " " + curName);
            } else {
                // Remove icon -> clean name without emoji
                fullDisplayName = curName.isEmpty() ? tr("Tasks") : curName;
            }

            m_lists[i].displayName = fullDisplayName;
            emit dataChanged(index(i), index(i), {DisplayNameRole, NameRole, IconRole});

            m_repo->updateList(m_lists[i]);

            if (m_graph && !m_lists[i].remoteId.isEmpty()) {
                m_graph->updateList(m_lists[i].remoteId, fullDisplayName, nullptr);
            }
            if (m_sync) {
                m_sync->scheduleSync(500);
            }
            break;
        }
    }
}

QString TaskListModel::getListIcon(const QString &id) const {
    for (const auto &l : m_lists) {
        if (l.id == id) {
            QString emoji, name;
            parseDisplayName(l.displayName, emoji, name);
            return emoji;
        }
    }
    return QString();
}

QString TaskListModel::getListName(const QString &id) const {
    for (const auto &l : m_lists) {
        if (l.id == id) {
            QString emoji, name;
            parseDisplayName(l.displayName, emoji, name);
            return !name.isEmpty() ? name : (emoji.isEmpty() ? l.displayName : emoji);
        }
    }
    return QString();
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


