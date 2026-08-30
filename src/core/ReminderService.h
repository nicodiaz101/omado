#pragma once

#include <QObject>
#include <QTimer>
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

signals:
    void reminderTriggered(const Task &task);

private:
    LocalRepository     *m_repository;
    NotificationService *m_notifier;
    QTimer              *m_timer;
    bool                 m_isChecking = false;
};
