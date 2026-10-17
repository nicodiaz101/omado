#include "TaskModel.h"
#include "GraphClient.h"
#include "../core/SyncEngine.h"
#include "../core/TaskInputParser.h"
#include <QUuid>
#include <QDebug>
#include <QSettings>
#include <algorithm>

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

static bool getTaskSortKey(const Task &t, QDateTime &outDt) {
    bool hasDue = t.dueDate.isValid();
    bool hasReminder = t.reminderAt.isValid();

    if (!hasDue && !hasReminder) {
        return false;
    }

    if (hasDue && hasReminder) {
        QDateTime dueDt(t.dueDate, QTime(23, 59, 59));
        QDateTime remDt = t.reminderAt.toLocalTime();
        outDt = std::min(dueDt, remDt);
    } else if (hasReminder) {
        outDt = t.reminderAt.toLocalTime();
    } else {
        outDt = QDateTime(t.dueDate, QTime(23, 59, 59));
    }
    return true;
}

static bool compareTasksByDueDate(const Task &t1, const Task &t2) {
    // 1. Incomplete tasks before completed tasks
    if (t1.isCompleted != t2.isCompleted) {
        return !t1.isCompleted;
    }

    // 2. Deadlines/reminders
    QDateTime dt1, dt2;
    bool has1 = getTaskSortKey(t1, dt1);
    bool has2 = getTaskSortKey(t2, dt2);

    if (has1 && has2) {
        if (dt1 != dt2) {
            return dt1 < dt2;
        }
        if (t1.importance != t2.importance) {
            return t1.importance == "high";
        }
        return false;
    }

    if (has1 && !has2) {
        return true;
    }
    if (!has1 && has2) {
        return false;
    }

    // Neither has date/reminder
    if (t1.importance != t2.importance) {
        return t1.importance == "high";
    }

    return false;
}

TaskModel::TaskModel(LocalRepository *repo, GraphClient *graph, SyncEngine *sync, QObject *parent)
    : QAbstractListModel(parent), m_repo(repo), m_graph(graph), m_sync(sync) {
    QSettings settings("omacom-io", "OmaDo");
    m_hideCompleted = settings.value("ui/hideCompleted", false).toBool();
    m_sortByDueDate = settings.value("ui/sortByDueDate", false).toBool();

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
    for (int i = 0; i < m_tasks.count(); ++i) {
        if (m_hideCompleted && m_tasks[i].isCompleted) {
            continue;
        }
        m_visibleIndices.append(i);
    }
    if (m_sortByDueDate) {
        std::stable_sort(m_visibleIndices.begin(), m_visibleIndices.end(), [this](int a, int b) {
            return compareTasksByDueDate(m_tasks[a], m_tasks[b]);
        });
    }
}

int TaskModel::actualIndex(int row) const {
    if (row >= 0 && row < m_visibleIndices.count()) {
        return m_visibleIndices.at(row);
    }
    return -1;
}

void TaskModel::setHideCompleted(bool hide) {
    if (m_hideCompleted == hide) return;

    QSettings settings("omacom-io", "OmaDo");
    settings.setValue("ui/hideCompleted", hide);

    QString selId;
    int actIdx = actualIndex(m_selectedIndex);
    if (actIdx >= 0 && actIdx < m_tasks.count()) {
        selId = m_tasks[actIdx].id;
    }

    beginResetModel();
    m_hideCompleted = hide;
    rebuildVisibleIndices();
    if (!selId.isEmpty()) {
        m_selectedIndex = -1;
        for (int i = 0; i < m_visibleIndices.count(); ++i) {
            if (m_tasks[m_visibleIndices[i]].id == selId) {
                m_selectedIndex = i;
                break;
            }
        }
    } else {
        m_selectedIndex = -1;
    }
    endResetModel();

    emit hideCompletedChanged();
    emit selectedIndexChanged();
    emit selectedTaskChanged();
    emit countChanged();
}

void TaskModel::setSortByDueDate(bool sort) {
    if (m_sortByDueDate == sort) return;

    QSettings settings("omacom-io", "OmaDo");
    settings.setValue("ui/sortByDueDate", sort);

    QString selId;
    int actIdx = actualIndex(m_selectedIndex);
    if (actIdx >= 0 && actIdx < m_tasks.count()) {
        selId = m_tasks[actIdx].id;
    }

    beginResetModel();
    m_sortByDueDate = sort;
    rebuildVisibleIndices();
    if (!selId.isEmpty()) {
        m_selectedIndex = -1;
        for (int i = 0; i < m_visibleIndices.count(); ++i) {
            if (m_tasks[m_visibleIndices[i]].id == selId) {
                m_selectedIndex = i;
                break;
            }
        }
    } else {
        m_selectedIndex = -1;
    }
    endResetModel();

    emit sortByDueDateChanged();
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
    return m_visibleIndices.count();
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

QVariantMap TaskModel::parseInput(const QString &text) const {
    auto parsed = TaskInputParser::parse(text);
    QVariantMap map;
    map["cleanTitle"] = parsed.cleanTitle;
    map["hasDueDate"] = parsed.hasDueDate;
    map["dueDate"] = parsed.hasDueDate ? parsed.dueDate.toString(Qt::ISODate) : QString();
    map["hasReminder"] = parsed.hasReminder;
    map["reminderAt"] = parsed.hasReminder ? parsed.reminderAt.toString("yyyy-MM-dd HH:mm") : QString();
    map["reminderTime"] = parsed.hasReminder ? parsed.reminderAt.toString("HH:mm") : QString();
    return map;
}

void TaskModel::addTask(const QString &title, const QString &dueDate, const QString &reminderAt, const QString &recurrence) {
    addTaskWithSteps(title, dueDate, reminderAt, recurrence, QStringList());
}

void TaskModel::addTaskWithSteps(const QString &title, const QString &dueDate, const QString &reminderAt, const QString &recurrence, const QStringList &steps) {
    if (!m_repo || title.trimmed().isEmpty()) return;
    
    QString finalTitle = title.trimmed();
    QString finalDueDate = dueDate;
    QString finalReminderAt = reminderAt;

    if (finalDueDate.isEmpty() && finalReminderAt.isEmpty()) {
        auto parsed = TaskInputParser::parse(finalTitle);
        if (parsed.hasDueDate || parsed.hasReminder) {
            finalTitle = parsed.cleanTitle;
            if (parsed.hasDueDate) {
                finalDueDate = parsed.dueDate.toString(Qt::ISODate);
            }
            if (parsed.hasReminder) {
                finalReminderAt = parsed.reminderAt.toString("yyyy-MM-dd HH:mm");
            }
        }
    }

    Task t;
    t.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    t.title = finalTitle;
    t.body = "";
    t.isCompleted = false;
    t.importance = "normal";
    t.recurrence = recurrence.isEmpty() ? "none" : recurrence;
    t.createdAt = QDateTime::currentDateTime();

    if (!finalDueDate.isEmpty()) {
        t.dueDate = QDate::fromString(finalDueDate, Qt::ISODate);
    }
    if (!finalReminderAt.isEmpty()) {
        t.reminderAt = parseDateTimeLocal(finalReminderAt);
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
    
    if (m_sortByDueDate) {
        QString selId;
        int actIdx = actualIndex(m_selectedIndex);
        if (actIdx >= 0 && actIdx < m_tasks.count()) {
            selId = m_tasks[actIdx].id;
        }
        beginResetModel();
        m_tasks.insert(0, t);
        rebuildVisibleIndices();
        if (!selId.isEmpty()) {
            m_selectedIndex = -1;
            for (int i = 0; i < m_visibleIndices.count(); ++i) {
                if (m_tasks[m_visibleIndices[i]].id == selId) {
                    m_selectedIndex = i;
                    break;
                }
            }
        }
        endResetModel();
        emit countChanged();
        emit selectedIndexChanged();
        emit selectedTaskChanged();
    } else {
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
    }
    
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
    } else if (m_sortByDueDate) {
        QString selId;
        int currentAct = actualIndex(m_selectedIndex);
        if (currentAct >= 0 && currentAct < m_tasks.count()) {
            selId = m_tasks[currentAct].id;
        }
        beginResetModel();
        rebuildVisibleIndices();
        if (!selId.isEmpty()) {
            m_selectedIndex = -1;
            for (int i = 0; i < m_visibleIndices.count(); ++i) {
                if (m_tasks[m_visibleIndices[i]].id == selId) {
                    m_selectedIndex = i;
                    break;
                }
            }
        }
        endResetModel();
        emit selectedIndexChanged();
        emit selectedTaskChanged();
        emit countChanged();
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
    
    if (m_sortByDueDate) {
        QString selId;
        int currentAct = actualIndex(m_selectedIndex);
        if (currentAct >= 0 && currentAct < m_tasks.count()) {
            selId = m_tasks[currentAct].id;
        }
        beginResetModel();
        rebuildVisibleIndices();
        if (!selId.isEmpty()) {
            m_selectedIndex = -1;
            for (int i = 0; i < m_visibleIndices.count(); ++i) {
                if (m_tasks[m_visibleIndices[i]].id == selId) {
                    m_selectedIndex = i;
                    break;
                }
            }
        }
        endResetModel();
        emit selectedIndexChanged();
        emit selectedTaskChanged();
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

    if (m_sortByDueDate) {
        QString selId;
        int currentAct = actualIndex(m_selectedIndex);
        if (currentAct >= 0 && currentAct < m_tasks.count()) {
            selId = m_tasks[currentAct].id;
        }
        beginResetModel();
        rebuildVisibleIndices();
        if (!selId.isEmpty()) {
            m_selectedIndex = -1;
            for (int i = 0; i < m_visibleIndices.count(); ++i) {
                if (m_tasks[m_visibleIndices[i]].id == selId) {
                    m_selectedIndex = i;
                    break;
                }
            }
        }
        endResetModel();
        emit selectedIndexChanged();
        emit selectedTaskChanged();
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

void TaskModel::updateTaskReminder(int row, const QString &reminderAt) {
    int actIdx = actualIndex(row);
    if (actIdx < 0 || actIdx >= m_tasks.count() || !m_repo) return;
    if (reminderAt.isEmpty()) {
        m_tasks[actIdx].reminderAt = QDateTime();
    } else {
        m_tasks[actIdx].reminderAt = parseDateTimeLocal(reminderAt);
    }
    m_tasks[actIdx].reminded = false;

    if (m_sortByDueDate) {
        QString selId;
        int currentAct = actualIndex(m_selectedIndex);
        if (currentAct >= 0 && currentAct < m_tasks.count()) {
            selId = m_tasks[currentAct].id;
        }
        beginResetModel();
        rebuildVisibleIndices();
        if (!selId.isEmpty()) {
            m_selectedIndex = -1;
            for (int i = 0; i < m_visibleIndices.count(); ++i) {
                if (m_tasks[m_visibleIndices[i]].id == selId) {
                    m_selectedIndex = i;
                    break;
                }
            }
        }
        endResetModel();
        emit selectedIndexChanged();
        emit selectedTaskChanged();
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
