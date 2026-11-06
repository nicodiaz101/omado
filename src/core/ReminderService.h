#pragma once

#include <QObject>
#include <QTimer>
#include <QHash>
#include "models/Task.h"

class LocalRepository;
class NotificationService;

class ReminderService : public QObject {
    Q_OBJECT
public:
    static constexpr int kDefaultCheckIntervalMs = 60'000;

    explicit ReminderService(LocalRepository *repo,
                            NotificationService *notifier,
                            QObject *parent = nullptr);

    void start(int intervalMs = kDefaultCheckIntervalMs);
    void stop();
    bool isRunning() const;

public slots:
    void checkReminders();

private slots:
    void handleActionInvoked(uint id, const QString &actionKey);
    void handleNotificationClosed(uint id, uint reason);

signals:
    void reminderTriggered(const Task &task);
    void reminderActionProcessed(const QString &taskId, const QString &actionKey);

private:
    QString formatNotificationBody(const Task &task) const;

    LocalRepository     *m_repository;
    NotificationService *m_notifier;
    QTimer              *m_timer;
    bool                 m_isChecking = false;

    // Mapeo de Notification ID -> Task ID
    QHash<uint, QString> m_activeNotificationTasks;
};
