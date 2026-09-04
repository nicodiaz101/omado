#include <QJsonDocument>
#include "DaemonService.h"
#include "models/LocalRepository.h"
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

    m_connection.registerObject(
        QStringLiteral("/"),
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
    if (!m_repository) return false;

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
    if (!m_repository) return false;

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
    qDebug() << "[DaemonService] Solicitud de sincronización recibida vía D-Bus";
    emit SyncRequested();
}
