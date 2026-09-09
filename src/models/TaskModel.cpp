#include "TaskModel.h"
#include "GraphClient.h"
#include "../core/SyncEngine.h"
#include <QUuid>
#include <QDebug>
#include <QSettings>

static QDateTime parseDateTimeLocal(const QString &str) {
    if (str.trimmed().isEmpty()) return QDateTime();
    QDateTime dt = QDateTime::fromString(str, Qt::ISODate);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(str, "yyyy-MM-dd HH:mm");
    }
    if (!dt.isValid()) {
        dt = QDateTime::fromString(str, "yyyy-MM-dd HH:mm:ss");
    }
    if (dt.isValid()) {
        return dt.toLocalTime();
    }
    return QDateTime();
}

TaskModel::TaskModel(LocalRepository *repo, GraphClient *graph, SyncEngine *sync, QObject *parent)
    : QAbstractListModel(parent), m_repo(repo), m_graph(graph), m_sync(sync) {
    QSettings settings("omacom-io", "OmaDo");
    m_hideCompleted = settings.value("ui/hideCompleted", false).toBool();

    connect(&m_watcher, &QFutureWatcher<QList<Task>>::finished, this, [this]() {
        beginResetModel();
        m_tasks = m_watcher.result();
        rebuildVisibleIndices();
        m_selectedIndex = -1;
        endResetModel();
        emit selectedIndexChanged();
        emit selectedTaskChanged();
        emit countChanged();
    });
}

void TaskModel::rebuildVisibleIndices() {
    m_visibleIndices.clear();
    if (!m_hideCompleted) {
        return;
    }
    for (int i = 0; i < m_tasks.count(); ++i) {
        if (!m_tasks[i].isCompleted) {
            m_visibleIndices.append(i);
        }
    }
}

int TaskModel::actualIndex(int row) const {
    if (m_hideCompleted) {
        if (row >= 0 && row < m_visibleIndices.count()) {
            return m_visibleIndices.at(row);
        }
        return -1;
    }
    if (row >= 0 && row < m_tasks.count()) {
        return row;
    }
    return -1;
}

void TaskModel::setHideCompleted(bool hide) {
    if (m_hideCompleted == hide) return;

    QSettings settings("omacom-io", "OmaDo");
    settings.setValue("ui/hideCompleted", hide);

    beginResetModel();
    m_hideCompleted = hide;
    rebuildVisibleIndices();
    m_selectedIndex = -1;
    endResetModel();

    emit hideCompletedChanged();
    emit selectedIndexChanged();
    emit selectedTaskChanged();
    emit countChanged();
}

void TaskModel::setCurrentListId(const QString &id) {
    if (m_currentListId == id) return;
    m_currentListId = id;
    emit currentListIdChanged();
    loadTasks();
}

void TaskModel::setSelectedIndex(int idx) {
    if (idx >= rowCount()) idx = -1;
    if (m_selectedIndex == idx) return;
    m_selectedIndex = idx;
    emit selectedIndexChanged();
    emit selectedTaskChanged();
}

QVariantMap TaskModel::selectedTask() const {
    int actIdx = actualIndex(m_selectedIndex);
    if (actIdx < 0 || actIdx >= m_tasks.count()) {
        return QVariantMap();
    }
    const Task &t = m_tasks.at(actIdx);
    QVariantList stepList;
    for (const auto &s : t.steps) {
        stepList.append(s.toVariantMap());
    }

    return {
        {"id", t.id},
        {"listId", t.listId},
        {"title", t.title},
        {"body", t.body},
        {"isCompleted", t.isCompleted},
        {"isMyDay", t.isMyDay},
        {"importance", t.importance},
        {"dueDate", t.dueDate.isValid() ? t.dueDate.toString(Qt::ISODate) : ""},
        {"reminderAt", t.reminderAt.isValid() ? t.reminderAt.toLocalTime().toString("yyyy-MM-dd HH:mm") : ""},
        {"recurrence", t.recurrence},
        {"steps", stepList}
    };
}

int TaskModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_hideCompleted ? m_visibleIndices.count() : m_tasks.count();
}

QVariant TaskModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    int actIdx = actualIndex(index.row());
    if (actIdx < 0 || actIdx >= m_tasks.count()) return QVariant();
    const Task &t = m_tasks.at(actIdx);

    int completedSteps = 0;
    for (const auto &s : t.steps) {
        if (s.isCompleted) completedSteps++;
    }

    switch(role) {
        case IdRole: return t.id;
        case ListIdRole: return t.listId;
        case TitleRole: return t.title;
        case BodyRole: return t.body;
        case IsCompletedRole: return t.isCompleted;
        case IsMyDayRole: return t.isMyDay;
        case DueDateRole: return t.dueDate.isValid() ? t.dueDate.toString(Qt::ISODate) : "";
        case ImportanceRole: return t.importance;
        case ReminderAtRole: return t.reminderAt.isValid() ? t.reminderAt.toLocalTime().toString("yyyy-MM-dd HH:mm") : "";
        case RecurrenceRole: return t.recurrence;
        case StepsRole: {
            QVariantList list;
            for (const auto &s : t.steps) list.append(s.toVariantMap());
            return list;
        }
        case HasStepsRole: return !t.steps.isEmpty();
        case StepsCountRole: return t.steps.count();
        case CompletedStepsCountRole: return completedSteps;
        default: return QVariant();
    }
}

QHash<int, QByteArray> TaskModel::roleNames() const {
    return {
        {IdRole, "id"},
        {ListIdRole, "listId"},
        {TitleRole, "title"},
        {BodyRole, "body"},
        {IsCompletedRole, "isCompleted"},
        {IsMyDayRole, "isMyDay"},
        {DueDateRole, "dueDate"},
        {ImportanceRole, "importance"},
        {ReminderAtRole, "reminderAt"},
        {RecurrenceRole, "recurrence"},
        {StepsRole, "steps"},
        {HasStepsRole, "hasSteps"},
        {StepsCountRole, "stepsCount"},
        {CompletedStepsCountRole, "completedStepsCount"}
    };
}

void TaskModel::loadTasks() {
    if (!m_repo) return;
    
    if (m_currentListId == "special-myday") {
        m_watcher.setFuture(m_repo->fetchMyDayTasks());
    } else if (m_currentListId == "special-schedule") {
        m_watcher.setFuture(m_repo->fetchScheduleTasks());
    } else if (m_currentListId == "special-tasks") {
        // En MS To Do, Tasks es su propia lista por defecto (default-tasks), no todas las listas
        m_watcher.setFuture(m_repo->fetchTasks("default-tasks"));
    } else if (!m_currentListId.isEmpty()) {
        m_watcher.setFuture(m_repo->fetchTasks(m_currentListId));
    } else {
        beginResetModel();
        m_tasks.clear();
        rebuildVisibleIndices();
        m_selectedIndex = -1;
        endResetModel();
        emit selectedIndexChanged();
        emit selectedTaskChanged();
        emit countChanged();
    }
}

void TaskModel::notifyRowChanged(int row) {
    if (row < 0 || row >= rowCount()) return;
    emit dataChanged(index(row), index(row));
    if (row == m_selectedIndex) {
        emit selectedTaskChanged();
    }
}

void TaskModel::addTask(const QString &title, const QString &dueDate, const QString &reminderAt, const QString &recurrence) {
    addTaskWithSteps(title, dueDate, reminderAt, recurrence, QStringList());
}

void TaskModel::addTaskWithSteps(const QString &title, const QString &dueDate, const QString &reminderAt, const QString &recurrence, const QStringList &steps) {
    if (!m_repo || title.trimmed().isEmpty()) return;
    
    Task t;
    t.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    t.title = title.trimmed();
    t.body = "";
    t.isCompleted = false;
    t.importance = "normal";
    t.recurrence = recurrence.isEmpty() ? "none" : recurrence;
    t.createdAt = QDateTime::currentDateTime();

    if (!dueDate.isEmpty()) {
        t.dueDate = QDate::fromString(dueDate, Qt::ISODate);
    }
    if (!reminderAt.isEmpty()) {
        t.reminderAt = parseDateTimeLocal(reminderAt);
    }

    if (m_currentListId == "special-myday") {
        t.listId = "default-tasks";
        t.isMyDay = true;
    } else if (m_currentListId == "special-schedule") {
        t.listId = "default-tasks";
        if (!t.dueDate.isValid()) t.dueDate = QDate::currentDate();
    } else if (m_currentListId == "special-tasks" || m_currentListId.isEmpty()) {
        t.listId = "default-tasks";
    } else {
        t.listId = m_currentListId;
    }

    for (int i = 0; i < steps.count(); ++i) {
        if (!steps[i].trimmed().isEmpty()) {
            TaskStep s;
            s.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            s.taskId = t.id;
            s.title = steps[i].trimmed();
            s.isCompleted = false;
            s.sortOrder = i;
            t.steps.append(s);
        }
    }
    
    beginInsertRows(QModelIndex(), 0, 0);
    m_tasks.insert(0, t);
    rebuildVisibleIndices();
    if (m_selectedIndex >= 0) {
        m_selectedIndex++;
    }
    endInsertRows();
    emit countChanged();
    emit selectedIndexChanged();
    emit selectedTaskChanged();
    
    m_repo->createTask(t);
    if (m_sync) {
        m_sync->scheduleSync(300);
    }
}

void TaskModel::toggleTaskCompletion(int row) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo) return;
    
    m_tasks[actIdx].isCompleted = !m_tasks[actIdx].isCompleted;
    if (m_tasks[actIdx].isCompleted) {
        m_tasks[actIdx].completedAt = QDateTime::currentDateTime();
    } else {
        m_tasks[actIdx].completedAt = QDateTime();
    }

    if (m_hideCompleted) {
        if (m_tasks[actIdx].isCompleted) {
            beginRemoveRows(QModelIndex(), row, row);
            rebuildVisibleIndices();
            if (m_selectedIndex == row) {
                m_selectedIndex = -1;
            } else if (m_selectedIndex > row) {
                m_selectedIndex--;
            }
            endRemoveRows();
            emit selectedIndexChanged();
            emit selectedTaskChanged();
            emit countChanged();
        } else {
            beginResetModel();
            rebuildVisibleIndices();
            endResetModel();
            emit selectedIndexChanged();
            emit selectedTaskChanged();
            emit countChanged();
        }
    } else {
        notifyRowChanged(row);
    }

    m_repo->updateTask(m_tasks[actIdx]);

    if (m_graph && !m_tasks[actIdx].remoteId.isEmpty() && m_repo) {
        QString remoteListId = m_repo->getListRemoteId(m_tasks[actIdx].listId);
        if (!remoteListId.isEmpty()) {
            m_graph->updateTask(remoteListId, m_tasks[actIdx], nullptr);
        }
    }
    if (m_sync) {
        m_sync->scheduleSync(800);
    }
}

void TaskModel::toggleTaskImportance(int row) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo) return;
    m_tasks[actIdx].importance = (m_tasks[actIdx].importance == "high") ? "normal" : "high";
    notifyRowChanged(row);
    m_repo->updateTask(m_tasks[actIdx]);

    if (m_graph && !m_tasks[actIdx].remoteId.isEmpty() && m_repo) {
        QString remoteListId = m_repo->getListRemoteId(m_tasks[actIdx].listId);
        if (!remoteListId.isEmpty()) {
            m_graph->updateTask(remoteListId, m_tasks[actIdx], nullptr);
        }
    }
    if (m_sync) {
        m_sync->scheduleSync(800);
    }
}

void TaskModel::toggleTaskMyDay(int row) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo) return;
    m_tasks[actIdx].isMyDay = !m_tasks[actIdx].isMyDay;
    notifyRowChanged(row);
    m_repo->updateTask(m_tasks[actIdx]);
    if (m_sync) {
        m_sync->scheduleSync(800);
    }
}

void TaskModel::updateTaskTitle(int row, const QString &title) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo || title.trimmed().isEmpty()) return;
    m_tasks[actIdx].title = title.trimmed();
    notifyRowChanged(row);
    m_repo->updateTask(m_tasks[actIdx]);

    if (m_graph && !m_tasks[actIdx].remoteId.isEmpty() && m_repo) {
        QString remoteListId = m_repo->getListRemoteId(m_tasks[actIdx].listId);
        if (!remoteListId.isEmpty()) {
            m_graph->updateTask(remoteListId, m_tasks[actIdx], nullptr);
        }
    }
    if (m_sync) {
        m_sync->scheduleSync(800);
    }
}

void TaskModel::updateTaskBody(int row, const QString &body) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo) return;
    m_tasks[actIdx].body = body;
    notifyRowChanged(row);
    m_repo->updateTask(m_tasks[actIdx]);

    if (m_graph && !m_tasks[actIdx].remoteId.isEmpty() && m_repo) {
        QString remoteListId = m_repo->getListRemoteId(m_tasks[actIdx].listId);
        if (!remoteListId.isEmpty()) {
            m_graph->updateTask(remoteListId, m_tasks[actIdx], nullptr);
        }
    }
    if (m_sync) {
        m_sync->scheduleSync(800);
    }
}

void TaskModel::updateTaskDueDate(int row, const QString &dueDate) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo) return;
    if (dueDate.isEmpty()) {
        m_tasks[actIdx].dueDate = QDate();
    } else {
        m_tasks[actIdx].dueDate = QDate::fromString(dueDate, Qt::ISODate);
    }
    notifyRowChanged(row);
    m_repo->updateTask(m_tasks[actIdx]);

    if (m_graph && !m_tasks[actIdx].remoteId.isEmpty() && m_repo) {
        QString remoteListId = m_repo->getListRemoteId(m_tasks[actIdx].listId);
        if (!remoteListId.isEmpty()) {
            m_graph->updateTask(remoteListId, m_tasks[actIdx], nullptr);
        }
    }
    if (m_sync) {
        m_sync->scheduleSync(800);
    }
}

void TaskModel::updateTaskReminder(int row, const QString &reminderAt) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo) return;
    if (reminderAt.isEmpty()) {
        m_tasks[actIdx].reminderAt = QDateTime();
    } else {
        m_tasks[actIdx].reminderAt = parseDateTimeLocal(reminderAt);
    }
    m_tasks[actIdx].reminded = false;
    notifyRowChanged(row);
    m_repo->updateTask(m_tasks[actIdx]);

    if (m_graph && !m_tasks[actIdx].remoteId.isEmpty() && m_repo) {
        QString remoteListId = m_repo->getListRemoteId(m_tasks[actIdx].listId);
        if (!remoteListId.isEmpty()) {
            m_graph->updateTask(remoteListId, m_tasks[actIdx], nullptr);
        }
    }
    if (m_sync) {
        m_sync->scheduleSync(800);
    }
}

void TaskModel::deleteTask(int row) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo) return;
    
    QString taskId = m_tasks[actIdx].id;
    QString remoteId = m_tasks[actIdx].remoteId;
    QString listId = m_tasks[actIdx].listId;

    beginRemoveRows(QModelIndex(), row, row);
    m_tasks.removeAt(actIdx);
    rebuildVisibleIndices();
    if (m_selectedIndex == row) {
        m_selectedIndex = -1;
    } else if (m_selectedIndex > row) {
        m_selectedIndex--;
    }
    endRemoveRows();
    
    emit selectedIndexChanged();
    emit selectedTaskChanged();
    emit countChanged();

    m_repo->deleteTask(taskId);

    if (m_graph && !remoteId.isEmpty() && m_repo) {
        QString remoteListId = m_repo->getListRemoteId(listId);
        if (!remoteListId.isEmpty()) {
            m_graph->deleteTask(remoteListId, remoteId, nullptr);
        }
    }
    if (m_sync) {
        m_sync->scheduleSync(800);
    }
}

void TaskModel::addStep(int row, const QString &stepTitle) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo || stepTitle.trimmed().isEmpty()) return;
    
    TaskStep s;
    s.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    s.taskId = m_tasks[actIdx].id;
    s.title = stepTitle.trimmed();
    s.isCompleted = false;
    s.sortOrder = m_tasks[actIdx].steps.count();

    m_tasks[actIdx].steps.append(s);
    notifyRowChanged(row);
    m_repo->addStep(s.taskId, s.title);
}

void TaskModel::toggleStep(int row, int stepIndex) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo) return;
    if (stepIndex < 0 || stepIndex >= m_tasks[actIdx].steps.count()) return;

    m_tasks[actIdx].steps[stepIndex].isCompleted = !m_tasks[actIdx].steps[stepIndex].isCompleted;
    notifyRowChanged(row);
    m_repo->updateStep(m_tasks[actIdx].steps[stepIndex]);
}

void TaskModel::deleteStep(int row, int stepIndex) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo) return;
    if (stepIndex < 0 || stepIndex >= m_tasks[actIdx].steps.count()) return;

    QString stepId = m_tasks[actIdx].steps[stepIndex].id;
    m_tasks[actIdx].steps.removeAt(stepIndex);
    notifyRowChanged(row);
    m_repo->deleteStep(stepId);
}
