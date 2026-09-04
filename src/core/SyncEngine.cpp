#include "SyncEngine.h"
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QDebug>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#include <memory>

SyncEngine::SyncEngine(AuthManager *auth, GraphClient *graph, LocalRepository *repo, QObject *parent)
    : QObject(parent), m_auth(auth), m_graph(graph), m_repo(repo), m_timer(new QTimer(this)), m_debounceTimer(new QTimer(this)) {
    
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, &SyncEngine::syncNow);
    connect(m_timer, &QTimer::timeout, this, &SyncEngine::syncNow);

    if (!m_graph) {
        // En modo cliente (GUI), conectarse a las señales D-Bus emitidas por el daemon
        QDBusConnection bus = QDBusConnection::sessionBus();
        bus.connect("io.omarchy.OmaDo", "/io/omarchy/OmaDo", "io.omarchy.OmaDo", "SyncStarted",
                    this, SLOT(onRemoteSyncStarted()));
        bus.connect("io.omarchy.OmaDo", "/io/omarchy/OmaDo", "io.omarchy.OmaDo", "SyncFinished",
                    this, SLOT(onRemoteSyncFinished(bool, QString)));
        bus.connect("io.omarchy.OmaDo", "/io/omarchy/OmaDo", "io.omarchy.OmaDo", "TasksChanged",
                    this, SLOT(onRemoteTasksChanged(QString)));
        bus.connect("io.omarchy.OmaDo", "/io/omarchy/OmaDo", "io.omarchy.OmaDo", "TodayTasksChanged",
                    this, SLOT(onRemoteTasksChanged(QString)));
    }

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

void SyncEngine::onRemoteSyncStarted() {
    if (!m_isSyncing) {
        m_isSyncing = true;
        emit syncStatusChanged();
        emit syncStarted();
    }
}

void SyncEngine::onRemoteSyncFinished(bool success, const QString &message) {
    m_isSyncing = false;
    if (success) {
        m_lastSyncedAt = QDateTime::currentDateTime();
        emit lastSyncedAtChanged();
    }
    emit syncStatusChanged();
    emit syncFinished(success, message);
}

void SyncEngine::onRemoteTasksChanged(const QString &) {
    emit syncFinished(true, QStringLiteral("Actualizado por daemon"));
}

void SyncEngine::startPeriodicSync(int intervalMs) {
    if (!m_graph) return; // En modo cliente, el daemon se encarga de los chequeos periódicos
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
    if (m_debounceTimer) {
        m_debounceTimer->stop();
        m_debounceTimer->start(delayMs);
    }
}

void SyncEngine::syncNow() {
    if (m_isSyncing) return;

    if (!m_graph) {
        // Modo cliente: Solicitar al daemon vía D-Bus
        m_isSyncing = true;
        emit syncStatusChanged();
        emit syncStarted();

        QDBusMessage msg = QDBusMessage::createMethodCall("io.omarchy.OmaDo", "/io/omarchy/OmaDo", "io.omarchy.OmaDo", "RequestSync");
        bool sent = QDBusConnection::sessionBus().send(msg);
        if (!sent) {
            qWarning() << "[SyncEngine] Error enviando RequestSync por D-Bus al daemon";
            m_isSyncing = false;
            emit syncStatusChanged();
            emit syncFinished(false, "No se pudo conectar al daemon de OmaDo");
            return;
        }

        // Timeout de seguridad en caso de que el daemon esté colgado o no responda
        QTimer::singleShot(20000, this, [this]() {
            if (m_isSyncing) {
                m_isSyncing = false;
                emit syncStatusChanged();
                emit syncFinished(false, "Timeout esperando respuesta del daemon");
            }
        });
        return;
    }

    if (!m_auth) return;

    // En modo daemon, si aún no está autenticado, intentar leer credenciales de inmediato
    if (!m_auth->isAuthenticated()) {
        qDebug() << "[SyncEngine] Daemon no autenticado al solicitar sync, reintentando credenciales...";
        m_auth->checkSavedCredentials([this](bool restored) {
            if (restored) {
                syncNow();
            } else {
                qWarning() << "[SyncEngine] Sincronización omitida: usuario no autenticado";
                emit syncFinished(false, "No autenticado");
            }
        });
        return;
    }

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
    m_graph->fetchTasks(remoteListId, [this, localListId, remoteListId, onDone](bool ok, const QJsonArray &remoteTasks, const QString &err) {
        if (!ok) {
            qWarning() << "[SyncEngine] Error al obtener tareas remotas para lista:" << remoteListId << err;
            if (onDone) onDone();
            return;
        }

        QSet<QString> remoteTaskIds;

        // Descargar e insertar/actualizar tareas remotas en SQLite
        for (const auto &val : remoteTasks) {
            QJsonObject obj = val.toObject();
            Task t;
            t.remoteId = obj.value("id").toString();
            remoteTaskIds.insert(t.remoteId);

            t.title = obj.value("title").toString();
            t.isCompleted = (obj.value("status").toString() == "completed");
            t.importance = obj.value("importance").toString("normal");

            QJsonObject bodyObj = obj.value("body").toObject();
            t.body = bodyObj.value("content").toString();

            QJsonObject dueObj = obj.value("dueDateTime").toObject();
            QString dueStr = dueObj.value("dateTime").toString();
            if (!dueStr.isEmpty()) {
                // Microsoft To Do almacena la fecha de vencimiento como un valor de solo-fecha fijado a medianoche UTC.
                // No se debe aplicar conversión de zona horaria para evitar desfasar el día en husos horarios negativos (UTC-X).
                QString datePart = dueStr.split('T').value(0);
                t.dueDate = QDate::fromString(datePart, Qt::ISODate);
            }

            if (obj.value("isReminderOn").toBool()) {
                QJsonObject remObj = obj.value("reminderDateTime").toObject();
                QString remStr = remObj.value("dateTime").toString();
                if (!remStr.isEmpty()) {
                    QString iso = remStr.endsWith('Z') ? remStr : remStr + "Z";
                    QDateTime dt = QDateTime::fromString(iso, Qt::ISODate);
                    t.reminderAt = dt.isValid() ? dt.toLocalTime() : QDateTime::fromString(remStr, Qt::ISODate).toLocalTime();
                }
            }

            if (obj.contains("recurrence") && !obj.value("recurrence").isNull()) {
                QJsonObject recObj = obj.value("recurrence").toObject();
                QJsonObject patObj = recObj.value("pattern").toObject();
                QString type = patObj.value("type").toString();
                if (type == "daily") {
                    t.recurrence = "daily";
                } else if (type == "weekly") {
                    QJsonArray days = patObj.value("daysOfWeek").toArray();
                    if (days.size() == 5 && !days.contains("saturday") && !days.contains("sunday")) {
                        t.recurrence = "workdays";
                    } else {
                        t.recurrence = "weekly";
                    }
                } else if (type == "absoluteMonthly" || type == "relativeMonthly") {
                    t.recurrence = "monthly";
                } else {
                    t.recurrence = "none";
                }
            } else {
                t.recurrence = "none";
            }

            m_repo->upsertRemoteTask(localListId, t);
        }

        // 2. Procesar tareas locales
        auto futureTasks = m_repo->fetchTasks(localListId);
        auto *watcher = new QFutureWatcher<QList<Task>>(this);
        connect(watcher, &QFutureWatcher<QList<Task>>::finished, this, [this, watcher, localListId, remoteListId, remoteTaskIds, onDone]() {
            QList<Task> localTasks = watcher->result();
            watcher->deleteLater();

            auto pendingOps = std::make_shared<int>(1); // Base 1
            auto checkDone = [onDone, pendingOps]() {
                (*pendingOps)--;
                if (*pendingOps <= 0 && onDone) {
                    onDone();
                }
            };

            for (const auto &locTask : localTasks) {
                if (locTask.remoteId.isEmpty()) {
                    // Tarea local nueva sin remoteId -> Subir a Graph
                    (*pendingOps)++;
                    m_graph->createTask(remoteListId, locTask, [this, locTask, checkDone](bool okCreate, const QJsonObject &taskObj, const QString &err) {
                        if (okCreate) {
                            QString newRemoteId = taskObj.value("id").toString();
                            m_repo->updateTaskRemoteId(locTask.id, newRemoteId);
                        } else {
                            qWarning() << "[SyncEngine] Error creando tarea en Graph:" << locTask.title << err;
                        }
                        checkDone();
                    });
                } else if (locTask.updatedAt.isValid() && locTask.syncedAt.isValid() && (locTask.updatedAt > locTask.syncedAt)) {
                    // Tarea local con modificaciones pendientes (ej. marcada completada) -> Actualizar en Graph
                    (*pendingOps)++;
                    m_graph->updateTask(remoteListId, locTask, [this, locTask, checkDone](bool okUpdate, const QString &err) {
                        if (okUpdate) {
                            m_repo->markTaskSynced(locTask.id);
                        } else {
                            qWarning() << "[SyncEngine] Error actualizando tarea en Graph:" << locTask.title << err;
                        }
                        checkDone();
                    });
                } else if (!locTask.remoteId.isEmpty() && !remoteTaskIds.contains(locTask.remoteId) && locTask.syncedAt.isValid()) {
                    // Tarea eliminada remotamente (en celular / Samsung Reminders) -> Eliminar localmente sin reencolar
                    m_repo->deleteTask(locTask.id, false);
                }
            }

            // 3. Procesar eliminaciones locales pendientes de sincronizar
            auto deletedFuture = m_repo->fetchDeletedTasks();
            auto *delWatcher = new QFutureWatcher<QList<LocalRepository::DeletedTaskRecord>>(this);
            connect(delWatcher, &QFutureWatcher<QList<LocalRepository::DeletedTaskRecord>>::finished, this, [this, delWatcher, remoteListId, pendingOps, checkDone]() {
                QList<LocalRepository::DeletedTaskRecord> deletedRecords = delWatcher->result();
                delWatcher->deleteLater();

                for (const auto &rec : deletedRecords) {
                    if (rec.listRemoteId.isEmpty() || rec.listRemoteId == remoteListId) {
                        (*pendingOps)++;
                        m_graph->deleteTask(remoteListId, rec.remoteId, [this, rec, checkDone](bool, const QString &) {
                            m_repo->removeDeletedTaskRecord(rec.id);
                            checkDone();
                        });
                    }
                }
                checkDone(); // Decrementa el base 1
            });
            delWatcher->setFuture(deletedFuture);
        });
        watcher->setFuture(futureTasks);
    });
}
