#include "ReminderService.h"
#include "models/LocalRepository.h"
#include "NotificationService.h"
#include <QFutureWatcher>
#include <QVariantMap>
#include <QStringList>
#include <QDebug>

ReminderService::ReminderService(LocalRepository *repo,
                                 NotificationService *notifier,
                                 QObject *parent)
    : QObject(parent)
    , m_repository(repo)
    , m_notifier(notifier)
    , m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, &ReminderService::checkReminders);

    if (m_notifier) {
        connect(m_notifier, &NotificationService::actionInvoked,
                this, &ReminderService::handleActionInvoked);
        connect(m_notifier, &NotificationService::notificationClosed,
                this, &ReminderService::handleNotificationClosed);
    }
}

void ReminderService::start(int intervalMs) {
    if (!m_timer->isActive()) {
        m_timer->start(intervalMs);
        qDebug() << "[ReminderService] Iniciado con intervalo de" << intervalMs << "ms";
        checkReminders();
    }
}

void ReminderService::stop() {
    if (m_timer->isActive()) {
        m_timer->stop();
        qDebug() << "[ReminderService] Detenido";
    }
}

bool ReminderService::isRunning() const {
    return m_timer->isActive();
}

QString ReminderService::formatNotificationBody(const Task &task) const {
    QStringList sections;

    if (!task.body.trimmed().isEmpty()) {
        sections.append(task.body.trimmed());
    }

    // Agregar hasta 3 subtareas pendientes si existen
    QStringList pendingSteps;
    int count = 0;
    for (const TaskStep &step : task.steps) {
        if (!step.isCompleted) {
            if (count < 3) {
                pendingSteps.append(QStringLiteral("• ") + step.title);
            }
            count++;
        }
    }

    if (!pendingSteps.isEmpty()) {
        QString stepText = pendingSteps.join(QLatin1Char('\n'));
        if (count > 3) {
            stepText += QStringLiteral("\n(+%1 más...)").arg(count - 3);
        }
        sections.append(stepText);
    }

    if (sections.isEmpty()) {
        return QStringLiteral("OmaDo");
    }

    return sections.join(QStringLiteral("\n\n"));
}

void ReminderService::checkReminders() {
    if (!m_repository || !m_notifier || m_isChecking) {
        return;
    }

    m_isChecking = true;
    auto future = m_repository->getPendingReminders();
    auto *watcher = new QFutureWatcher<QList<Task>>(this);

    connect(watcher, &QFutureWatcher<QList<Task>>::finished, this, [this, watcher]() {
        QList<Task> pendingTasks = watcher->result();
        watcher->deleteLater();
        m_isChecking = false;

        for (const Task &task : pendingTasks) {
            QString title = QStringLiteral("Recordatorio: ") + task.title;
            QString body = formatNotificationBody(task);

            // Acciones para Omarchy 4.5 / Omapager (pares: id, texto)
            QStringList actions;
            actions << QStringLiteral("complete") << QStringLiteral("Completar")
                    << QStringLiteral("snooze-15") << QStringLiteral("Posponer 15m");

            QVariantMap hints;
            hints.insert(QStringLiteral("desktop-entry"), QStringLiteral("omado"));
            if (task.importance == QLatin1String("high")) {
                hints.insert(QStringLiteral("urgency"), static_cast<uchar>(2)); // Urgencia crítica
            } else {
                hints.insert(QStringLiteral("urgency"), static_cast<uchar>(1)); // Normal
            }

            uint notificationId = m_notifier->sendNotification(
                title,
                body,
                actions,
                hints,
                QStringLiteral("omado"),
                task.importance == QLatin1String("high") ? 0 : 5000 // 0 = no expira automáticamente si es high
            );

            if (notificationId > 0) {
                m_activeNotificationTasks.insert(notificationId, task.id);
                qDebug() << "[ReminderService] Notificación ID" << notificationId << "enviada para tarea:" << task.id << task.title;
            } else {
                qWarning() << "[ReminderService] No se pudo enviar notificación para tarea:" << task.id;
            }

            // Marcar tarea como recordada en SQLite
            auto markFuture = m_repository->markReminded(task.id, true);
            auto *markWatcher = new QFutureWatcher<bool>(this);
            connect(markWatcher, &QFutureWatcher<bool>::finished, this, [markWatcher]() {
                markWatcher->deleteLater();
            });
            markWatcher->setFuture(markFuture);

            emit reminderTriggered(task);
        }
    });

    watcher->setFuture(future);
}

void ReminderService::handleActionInvoked(uint id, const QString &actionKey) {
    if (!m_activeNotificationTasks.contains(id)) {
        return;
    }

    QString taskId = m_activeNotificationTasks.value(id);
    qDebug() << "[ReminderService] Acción recibida para ID" << id << "tarea" << taskId << ":" << actionKey;

    if (actionKey == QLatin1String("complete")) {
        auto future = m_repository->toggleTask(taskId, true);
        auto *watcher = new QFutureWatcher<bool>(this);
        connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, taskId, actionKey]() {
            watcher->deleteLater();
            emit reminderActionProcessed(taskId, actionKey);
        });
        watcher->setFuture(future);
    } else if (actionKey == QLatin1String("snooze-15")) {
        auto future = m_repository->snoozeTaskReminder(taskId, 15);
        auto *watcher = new QFutureWatcher<bool>(this);
        connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, taskId, actionKey]() {
            watcher->deleteLater();
            emit reminderActionProcessed(taskId, actionKey);
        });
        watcher->setFuture(future);
    }
}

void ReminderService::handleNotificationClosed(uint id, uint reason) {
    Q_UNUSED(reason);
    m_activeNotificationTasks.remove(id);
}
