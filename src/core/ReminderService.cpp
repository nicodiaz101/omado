#include "ReminderService.h"
#include "models/LocalRepository.h"
#include "NotificationService.h"
#include <QFutureWatcher>
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
            QString body = task.body.isEmpty() ? QStringLiteral("OmaDo") : task.body;
            
            bool sent = m_notifier->send(title, body, QStringLiteral("checkbox-checked"), 5000);
            if (sent) {
                qDebug() << "[ReminderService] Notificación enviada para tarea:" << task.id << task.title;
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
