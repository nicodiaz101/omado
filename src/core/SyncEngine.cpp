#include "SyncEngine.h"
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <memory>

SyncEngine::SyncEngine(AuthManager *auth, GraphClient *graph, LocalRepository *repo, QObject *parent)
    : QObject(parent), m_auth(auth), m_graph(graph), m_repo(repo), m_timer(new QTimer(this)), m_debounceTimer(new QTimer(this)) {
    
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, &SyncEngine::syncNow);
    connect(m_timer, &QTimer::timeout, this, &SyncEngine::syncNow);

    if (m_auth) {
        connect(m_auth, &AuthManager::authSuccess, this, [this]() {
            startPeriodicSync();
            scheduleSync(100);
        });
        connect(m_auth, &AuthManager::authStatusChanged, this, [this]() {
            if (m_auth->isAuthenticated()) {
                startPeriodicSync();
                scheduleSync(100);
            } else {
                stopPeriodicSync();
            }
        });
        if (m_auth->isAuthenticated()) {
            startPeriodicSync();
            scheduleSync(100);
        }
    }
}

void SyncEngine::startPeriodicSync(int intervalMs) {
    if (!m_timer->isActive()) {
        m_timer->start(intervalMs);
        qDebug() << "[SyncEngine] Sincronización periódica iniciada cada" << intervalMs / 1000 << "segundos";
    }
}

void SyncEngine::stopPeriodicSync() {
    if (m_timer->isActive()) {
        m_timer->stop();
        qDebug() << "[SyncEngine] Sincronización periódica detenida";
    }
}

void SyncEngine::scheduleSync(int delayMs) {
    if (!m_auth || !m_auth->isAuthenticated()) return;
    if (m_debounceTimer) {
        m_debounceTimer->stop();
        m_debounceTimer->start(delayMs);
    }
}

void SyncEngine::syncNow() {
    if (m_isSyncing) return;
    if (!m_auth || !m_auth->isAuthenticated()) return;

    m_isSyncing = true;
    emit syncStatusChanged();
    emit syncStarted();

    performSync();
}

void SyncEngine::performSync() {
    qDebug() << "[SyncEngine] Iniciando ciclo de sincronización...";

    // 1. Obtener listas remotas de Microsoft Graph
    m_graph->fetchLists([this](bool ok, const QJsonArray &remoteLists, const QString &err) {
        if (!ok) {
            qWarning() << "[SyncEngine] Error al obtener listas de Graph:" << err;
            m_isSyncing = false;
            emit syncStatusChanged();
            emit syncFinished(false, err);
            return;
        }

        // Obtener listas locales de SQLite
        auto futureLists = m_repo->fetchLists();
        auto *watcher = new QFutureWatcher<QList<TaskList>>(this);
        connect(watcher, &QFutureWatcher<QList<TaskList>>::finished, this, [this, watcher, remoteLists]() {
            QList<TaskList> localLists = watcher->result();
            watcher->deleteLater();

            // Mapear listas
            QString defaultRemoteId;
            QMap<QString, QString> localToRemoteMap; // localListId -> remoteListId

            for (const auto &val : remoteLists) {
                QJsonObject obj = val.toObject();
                QString rId = obj.value("id").toString();
                QString name = obj.value("displayName").toString();
                QString wellKnown = obj.value("wellknownListName").toString();

                if (wellKnown == "defaultList" || name == "Tasks") {
                    defaultRemoteId = rId;
                    m_repo->updateListRemoteId("default-tasks", rId);
                    localToRemoteMap["default-tasks"] = rId;
                } else {
                    // Buscar si existe localmente por remoteId o crearla
                    bool found = false;
                    for (const auto &loc : localLists) {
                        if (loc.remoteId == rId || loc.displayName == name) {
                            localToRemoteMap[loc.id] = rId;
                            if (loc.remoteId != rId) {
                                m_repo->updateListRemoteId(loc.id, rId);
                            }
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        auto f = m_repo->upsertRemoteList(rId, name);
                        TaskList newLoc = f.result();
                        localToRemoteMap[newLoc.id] = rId;
                    }
                }
            }

            // Subir listas locales que no tengan remoteId
            for (const auto &loc : localLists) {
                if (loc.isSpecial) continue;
                if (loc.remoteId.isEmpty() && !localToRemoteMap.contains(loc.id)) {
                    m_graph->createList(loc.displayName, [this, loc](bool okCreate, const QJsonObject &newObj, const QString &) {
                        if (okCreate) {
                            m_repo->updateListRemoteId(loc.id, newObj.value("id").toString());
                        }
                    });
                }
            }

            // Sincronizar tareas de cada lista
            if (localToRemoteMap.isEmpty()) {
                m_isSyncing = false;
                m_lastSyncedAt = QDateTime::currentDateTime();
                emit syncStatusChanged();
                emit lastSyncedAtChanged();
                emit syncFinished(true, "Sincronizado");
                return;
            }

            auto remainingCount = std::make_shared<int>(localToRemoteMap.count());
            for (auto it = localToRemoteMap.begin(); it != localToRemoteMap.end(); ++it) {
                QString locId = it.key();
                QString remId = it.value();

                syncTasksForList(locId, remId, [this, remainingCount]() {
                    (*remainingCount)--;
                    if (*remainingCount <= 0) {
                        m_isSyncing = false;
                        m_lastSyncedAt = QDateTime::currentDateTime();
                        emit syncStatusChanged();
                        emit lastSyncedAtChanged();
                        emit syncFinished(true, "Sincronización completada");
                        qDebug() << "[SyncEngine] Ciclo de sincronización finalizado con éxito";
                    }
                });
            }
        });
        watcher->setFuture(futureLists);
    });
}

void SyncEngine::syncTasksForList(const QString &localListId, const QString &remoteListId, std::function<void()> onDone) {
    // 1. Obtener tareas remotas de Graph
    m_graph->fetchTasks(remoteListId, [this, localListId, remoteListId, onDone](bool ok, const QJsonArray &remoteTasks, const QString &) {
        if (!ok) {
            if (onDone) onDone();
            return;
        }

        // Descargar e insertar/actualizar tareas remotas en SQLite
        for (const auto &val : remoteTasks) {
            QJsonObject obj = val.toObject();
            Task t;
            t.remoteId = obj.value("id").toString();
            t.title = obj.value("title").toString();
            t.isCompleted = (obj.value("status").toString() == "completed");
            t.importance = obj.value("importance").toString("normal");

            QJsonObject bodyObj = obj.value("body").toObject();
            t.body = bodyObj.value("content").toString();

            QJsonObject dueObj = obj.value("dueDateTime").toObject();
            QString dueStr = dueObj.value("dateTime").toString();
            if (!dueStr.isEmpty()) {
                t.dueDate = QDate::fromString(dueStr.split('T').value(0), Qt::ISODate);
            }

            if (obj.value("isReminderOn").toBool()) {
                QJsonObject remObj = obj.value("reminderDateTime").toObject();
                QString remStr = remObj.value("dateTime").toString();
                if (!remStr.isEmpty()) {
                    t.reminderAt = QDateTime::fromString(remStr, Qt::ISODate).toLocalTime();
                }
            }

            m_repo->upsertRemoteTask(localListId, t);
        }

        // 2. Subir tareas locales sin remoteId
        auto futureTasks = m_repo->fetchTasks(localListId);
        auto *watcher = new QFutureWatcher<QList<Task>>(this);
        connect(watcher, &QFutureWatcher<QList<Task>>::finished, this, [this, watcher, remoteListId, onDone]() {
            QList<Task> localTasks = watcher->result();
            watcher->deleteLater();

            for (const auto &locTask : localTasks) {
                if (locTask.remoteId.isEmpty()) {
                    m_graph->createTask(remoteListId, locTask, [this, locTask](bool okCreate, const QJsonObject &taskObj, const QString &) {
                        if (okCreate) {
                            QString newRemoteId = taskObj.value("id").toString();
                            m_repo->updateTaskRemoteId(locTask.id, newRemoteId);
                        }
                    });
                }
            }

            if (onDone) onDone();
        });
        watcher->setFuture(futureTasks);
    });
}
