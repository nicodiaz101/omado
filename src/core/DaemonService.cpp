#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include "DaemonService.h"
#include "models/LocalRepository.h"
#include "TaskInputParser.h"
#include <QtDBus/QDBusMetaType>
#include <QDebug>

DaemonService::DaemonService(LocalRepository *repo, QObject *parent)
    : QObject(parent)
    , m_repository(repo)
    , m_connection(QDBusConnection::sessionBus())
{
    qDBusRegisterMetaType<OmaDoListEntry>();
    qDBusRegisterMetaType<QList<OmaDoListEntry>>();
    qDBusRegisterMetaType<QList<QVariantMap>>();
    qDBusRegisterMetaType<QVariantMap>();
}

DaemonService::DaemonService(LocalRepository *repo, const QDBusConnection &connection, QObject *parent)
    : QObject(parent)
    , m_repository(repo)
    , m_connection(connection)
{
    qDBusRegisterMetaType<OmaDoListEntry>();
    qDBusRegisterMetaType<QList<OmaDoListEntry>>();
    qDBusRegisterMetaType<QList<QVariantMap>>();
    qDBusRegisterMetaType<QVariantMap>();
}

bool DaemonService::registerService() {
    if (!m_connection.isConnected()) {
        qWarning() << "[DaemonService] No se puede conectar a D-Bus session bus";
        return false;
    }

    if (!m_connection.registerService(QStringLiteral("io.omarchy.OmaDo"))) {
        qWarning() << "[DaemonService] No se pudo registrar el servicio D-Bus io.omarchy.OmaDo:"
                   << m_connection.lastError().message();
        return false;
    }

    bool objRegistered = m_connection.registerObject(
        QStringLiteral("/io/omarchy/OmaDo"),
        this,
        QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals
    );

    if (!objRegistered) {
        qWarning() << "[DaemonService] No se pudo registrar el objeto D-Bus:"
                   << m_connection.lastError().message();
        return false;
    }

    qDebug() << "[DaemonService] Servicio D-Bus io.omarchy.OmaDo registrado exitosamente";
    return true;
}

QList<OmaDoListEntry> DaemonService::GetLists() {
    QList<OmaDoListEntry> result;
    if (!m_repository) return result;

    auto future = m_repository->fetchLists();
    QList<TaskList> lists = future.result();

    for (const TaskList &l : lists) {
        result.append({l.id, l.displayName});
    }
    return result;
}

QString DaemonService::GetTasksForToday() {
    QList<QVariant> result;
    if (!m_repository) return QStringLiteral("[]");

    auto future = m_repository->fetchMyDayTasks();
    QList<Task> tasks = future.result();

    for (const Task &t : tasks) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), t.id);
        map.insert(QStringLiteral("listId"), t.listId);
        map.insert(QStringLiteral("title"), t.title);
        map.insert(QStringLiteral("body"), t.body);
        map.insert(QStringLiteral("isCompleted"), t.isCompleted);
        map.insert(QStringLiteral("isMyDay"), t.isMyDay);
        map.insert(QStringLiteral("importance"), t.importance);
        map.insert(QStringLiteral("dueDate"), t.dueDate.isValid() ? t.dueDate.toString(Qt::ISODate) : QString());
        map.insert(QStringLiteral("reminderAt"), t.reminderAt.isValid() ? t.reminderAt.toString(Qt::ISODate) : QString());
        map.insert(QStringLiteral("recurrence"), t.recurrence);
        map.insert(QStringLiteral("sortOrder"), t.sortOrder);
        result.append(map);
    }
    return QString::fromUtf8(QJsonDocument::fromVariant(result).toJson(QJsonDocument::Compact));
}

int DaemonService::GetPendingCount(const QString &listId) {
    if (!m_repository) return 0;
    auto future = m_repository->getPendingCount(listId);
    return future.result();
}

int DaemonService::GetTotalPendingCount() {
    if (!m_repository) return 0;
    auto future = m_repository->getTotalPendingCount();
    return future.result();
}

bool DaemonService::ToggleTask(const QString &taskId, bool completed) {
    if (!m_repository || taskId.trimmed().isEmpty() || taskId.length() > 128) return false;

    auto taskFuture = m_repository->fetchTaskById(taskId);
    Task t = taskFuture.result();
    if (t.id.isEmpty()) {
        qWarning() << "[DaemonService] ToggleTask: Tarea no encontrada:" << taskId;
        return false;
    }

    auto toggleFuture = m_repository->toggleTask(taskId, completed);
    bool ok = toggleFuture.result();

    if (ok) {
        emit TasksChanged(t.listId);
        emit TodayTasksChanged();
    }
    return ok;
}

bool DaemonService::DeleteTask(const QString &taskId) {
    if (!m_repository || taskId.trimmed().isEmpty() || taskId.length() > 128) return false;

    auto taskFuture = m_repository->fetchTaskById(taskId);
    Task t = taskFuture.result();
    if (t.id.isEmpty()) {
        qWarning() << "[DaemonService] DeleteTask: Tarea no encontrada:" << taskId;
        return false;
    }

    auto deleteFuture = m_repository->deleteTask(taskId);
    bool ok = deleteFuture.result();

    if (ok) {
        emit TasksChanged(t.listId);
        emit TodayTasksChanged();
    }
    return ok;
}

void DaemonService::RequestSync() {
    if (m_syncRateLimiter.isValid() && m_syncRateLimiter.elapsed() < 5000) {
        qDebug() << "[DaemonService] RequestSync ignorado por limitación de tasa (<5s)";
        return;
    }
    m_syncRateLimiter.restart();
    qDebug() << "[DaemonService] Solicitud de sincronización recibida vía D-Bus";
    emit SyncRequested();
}

QString DaemonService::CreateTaskFromNaturalText(const QString &rawInput, bool addToMyDay) {
    if (!m_repository || rawInput.trimmed().isEmpty() || rawInput.length() > 1024) {
        return QString();
    }

    auto parsed = TaskInputParser::parse(rawInput.left(1024));

    // Determinar la lista destino
    QString targetListId = QStringLiteral("default-tasks");
    if (parsed.hasTargetList && !parsed.targetListName.isEmpty()) {
        auto lists = m_repository->fetchLists().result();
        bool found = false;
        for (const auto &l : lists) {
            if (l.displayName.compare(parsed.targetListName, Qt::CaseInsensitive) == 0) {
                targetListId = l.id;
                found = true;
                break;
            }
        }
        // Si no existe la lista, no la creamos automáticamente (evita que typos como #manana creen listas espurias)
        if (!found) {
            qInfo() << "[DaemonService] Lista destino no encontrada:" << parsed.targetListName << "usando lista por defecto";
            for (const auto &l : lists) {
                if (l.id == QLatin1String("default-tasks")) {
                    targetListId = l.id;
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto &l : lists) {
                    if (!l.isSpecial) {
                        targetListId = l.id;
                        found = true;
                        break;
                    }
                }
            }
            if (!found && !lists.isEmpty()) {
                targetListId = lists.first().id;
            }
        }
    }

    Task task;
    task.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.listId = targetListId;
    task.title = parsed.cleanTitle;
    task.isCompleted = false;
    task.isMyDay = addToMyDay;
    task.importance = parsed.hasImportance ? parsed.importance : QStringLiteral("normal");
    task.recurrence = parsed.hasRecurrence ? parsed.recurrence : QStringLiteral("none");
    if (parsed.hasDueDate) {
        task.dueDate = parsed.dueDate;
    }
    if (parsed.hasReminder) {
        task.reminderAt = parsed.reminderAt;
    }

    Task created = m_repository->createTask(task).result();
    if (!created.id.isEmpty()) {
        emit TasksChanged(targetListId);
        if (addToMyDay || task.dueDate == QDate::currentDate() || (task.reminderAt.isValid() && task.reminderAt.date() == QDate::currentDate())) {
            emit TodayTasksChanged();
        }
        return created.id;
    }

    return QString();
}

QString DaemonService::PreviewNaturalText(const QString &rawInput) {
    QString safeInput = rawInput.left(1024);
    auto parsed = TaskInputParser::parse(safeInput);
    QJsonObject obj;
    obj["cleanTitle"] = parsed.cleanTitle;
    obj["hasDueDate"] = parsed.hasDueDate;
    obj["dueDate"] = parsed.hasDueDate ? parsed.dueDate.toString(Qt::ISODate) : QString();
    obj["hasReminder"] = parsed.hasReminder;
    obj["reminderAt"] = parsed.hasReminder ? parsed.reminderAt.toString("yyyy-MM-dd HH:mm") : QString();
    obj["hasImportance"] = parsed.hasImportance;
    obj["importance"] = parsed.importance;
    obj["hasTargetList"] = parsed.hasTargetList;
    obj["targetListName"] = parsed.targetListName;
    obj["hasRecurrence"] = parsed.hasRecurrence;
    obj["recurrence"] = parsed.recurrence;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QString DaemonService::GetOverdueTasks() {
    if (!m_repository) return QStringLiteral("[]");
    auto future = m_repository->fetchOverdueTasks();
    QList<Task> tasks = future.result();

    QVariantList result;
    for (const Task &t : tasks) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), t.id);
        map.insert(QStringLiteral("listId"), t.listId);
        map.insert(QStringLiteral("title"), t.title);
        map.insert(QStringLiteral("body"), t.body);
        map.insert(QStringLiteral("isCompleted"), t.isCompleted);
        map.insert(QStringLiteral("isMyDay"), t.isMyDay);
        map.insert(QStringLiteral("importance"), t.importance);
        map.insert(QStringLiteral("dueDate"), t.dueDate.isValid() ? t.dueDate.toString(Qt::ISODate) : QString());
        map.insert(QStringLiteral("reminderAt"), t.reminderAt.isValid() ? t.reminderAt.toString(Qt::ISODate) : QString());
        map.insert(QStringLiteral("recurrence"), t.recurrence);
        result.append(map);
    }
    return QString::fromUtf8(QJsonDocument::fromVariant(result).toJson(QJsonDocument::Compact));
}

bool DaemonService::RescheduleTaskForMyDay(const QString &taskId) {
    if (!m_repository || taskId.trimmed().isEmpty() || taskId.length() > 128) return false;

    auto future = m_repository->rescheduleTaskForMyDay(taskId);
    bool ok = future.result();
    if (ok) {
        emit TodayTasksChanged();
        emit TasksChanged(QString());
    }
    return ok;
}

bool DaemonService::SnoozeTask(const QString &taskId, int minutes) {
    if (!m_repository || taskId.trimmed().isEmpty() || taskId.length() > 128) return false;

    // Validación de límites: entre 1 minuto y 1440 minutos (24 horas)
    int safeMinutes = minutes;
    if (safeMinutes < 1 || safeMinutes > 1440) {
        safeMinutes = 15;
    }

    auto future = m_repository->snoozeTaskReminder(taskId, safeMinutes);
    bool ok = future.result();
    if (ok) {
        emit TodayTasksChanged();
        emit TasksChanged(QString());
    }
    return ok;
}
