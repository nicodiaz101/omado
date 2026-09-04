#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "AuthManager.h"
#include "../models/GraphClient.h"
#include "../models/LocalRepository.h"

class SyncEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY syncStatusChanged)
    Q_PROPERTY(QDateTime lastSyncedAt READ lastSyncedAt NOTIFY lastSyncedAtChanged)

public:
    explicit SyncEngine(AuthManager *auth, GraphClient *graph, LocalRepository *repo, QObject *parent = nullptr);

    bool isSyncing() const { return m_isSyncing; }
    QDateTime lastSyncedAt() const { return m_lastSyncedAt; }

    Q_INVOKABLE void syncNow();
    Q_INVOKABLE void scheduleSync(int delayMs = 500);
    void startPeriodicSync(int intervalMs = 300000); // 5 minutos por defecto
    void stopPeriodicSync();

signals:
    void syncStatusChanged();
    void lastSyncedAtChanged();
    void syncStarted();
    void syncFinished(bool success, const QString &message);

private slots:
    void onRemoteSyncStarted();
    void onRemoteSyncFinished(bool success, const QString &message);
    void onRemoteTasksChanged(const QString &listId);

private:
    void performSync();
    void syncTasksForList(const QString &localListId, const QString &remoteListId, std::function<void()> onDone);

    AuthManager     *m_auth = nullptr;
    GraphClient     *m_graph = nullptr;
    LocalRepository *m_repo = nullptr;
    QTimer          *m_timer = nullptr;
    QTimer          *m_debounceTimer = nullptr;

    bool             m_isSyncing = false;
    QDateTime        m_lastSyncedAt;
};
