#include "Database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

Database::Database(QObject *parent) : QObject(parent) {
}

bool Database::initialize() {
    // 1. Asegurar que el directorio exista y tenga permisos restringidos (0700)
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);
    QFile::setPermissions(dataDir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    QString dbPath = dataDir + "/omado.db";
    
    // 2. Configurar la conexión
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        m_db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
    }
    m_db.setDatabaseName(dbPath);
    
    if (!m_db.open()) {
        qWarning() << "[Database] No se pudo abrir la base de datos:" << m_db.lastError().text();
        return false;
    }

    // Asegurar permisos estrictos en el archivo SQLite (0600)
    QFile::setPermissions(dbPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    
    // 3. Activar WAL mode y constrains de foreign keys
    executeSql("PRAGMA journal_mode=WAL;");
    executeSql("PRAGMA synchronous=NORMAL;");
    executeSql("PRAGMA foreign_keys=ON;");
    executeSql("PRAGMA busy_timeout=5000;");
    executeSql("PRAGMA cache_size=-512;");
    executeSql("PRAGMA temp_store=MEMORY;");
    
    // 4. Migraciones
    return applyMigrations();
}

bool Database::executeSql(const QString &sql) {
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        qWarning() << "[Database] Error ejecutando SQL:" << query.lastError().text() << "\nQuery:" << sql;
        return false;
    }
    return true;
}

bool Database::applyMigrations() {
    executeSql(R"(
        CREATE TABLE IF NOT EXISTS schema_version (
            version INTEGER PRIMARY KEY,
            applied_at TEXT NOT NULL
        )
    )");

    int currentVersion = 0;
    QSqlQuery query(m_db);
    if (query.exec("SELECT MAX(version) FROM schema_version") && query.next()) {
        currentVersion = query.value(0).toInt();
    }

    if (currentVersion < 1) {
        qDebug() << "[Database] Aplicando migración v1...";
        m_db.transaction();
        
        bool ok = executeSql(R"(
            CREATE TABLE task_lists (
                id              TEXT PRIMARY KEY,
                display_name    TEXT NOT NULL,
                is_special      INTEGER NOT NULL DEFAULT 0,
                sort_order      INTEGER NOT NULL DEFAULT 0,
                created_at      TEXT NOT NULL,
                synced_at       TEXT,
                remote_id       TEXT
            )
        )") && executeSql(R"(
            CREATE TABLE tasks (
                id              TEXT PRIMARY KEY,
                list_id         TEXT NOT NULL REFERENCES task_lists(id) ON DELETE CASCADE,
                title           TEXT NOT NULL,
                body            TEXT NOT NULL DEFAULT '',
                is_completed    INTEGER NOT NULL DEFAULT 0,
                is_my_day       INTEGER NOT NULL DEFAULT 0,
                importance      TEXT NOT NULL DEFAULT 'normal',
                due_date        TEXT,
                reminder_at     TEXT,
                reminded        INTEGER NOT NULL DEFAULT 0,
                recurrence      TEXT NOT NULL DEFAULT 'none',
                sort_order      INTEGER NOT NULL DEFAULT 0,
                created_at      TEXT NOT NULL,
                updated_at      TEXT,
                completed_at    TEXT,
                synced_at       TEXT,
                remote_id       TEXT
            )
        )") && executeSql(R"(
            CREATE TABLE task_steps (
                id              TEXT PRIMARY KEY,
                task_id         TEXT NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
                title           TEXT NOT NULL,
                is_completed    INTEGER NOT NULL DEFAULT 0,
                sort_order      INTEGER NOT NULL DEFAULT 0,
                remote_id       TEXT,
                updated_at      TEXT,
                synced_at       TEXT
            )
        )");

        if (ok) {
            // Datos semilla
            ok = executeSql(R"(
                INSERT INTO task_lists (id, display_name, is_special, sort_order, created_at)
                VALUES
                  ('special-myday',    'My Day',   1, 0, datetime('now')),
                  ('special-schedule', 'Schedule', 1, 1, datetime('now')),
                  ('special-tasks',    'Tasks',    1, 2, datetime('now')),
                  ('default-tasks',    'Tasks',    0, 3, datetime('now'))
            )");
        }
        
        if (ok) {
            executeSql("INSERT INTO schema_version (version, applied_at) VALUES (1, datetime('now'))");
            m_db.commit();
            currentVersion = 1;
        } else {
            m_db.rollback();
            qWarning() << "[Database] Fallo la migración v1";
            return false;
        }
    }
    
    if (currentVersion < 2) {
        qDebug() << "[Database] Aplicando migración v2 (reminders support)...";
        m_db.transaction();
        
        // Verificar si la columna 'reminded' ya existe
        bool hasReminded = false;
        QSqlQuery infoQuery(m_db);
        if (infoQuery.exec("PRAGMA table_info(tasks)")) {
            while (infoQuery.next()) {
                if (infoQuery.value("name").toString() == "reminded") {
                    hasReminded = true;
                    break;
                }
            }
        }
        
        bool ok = true;
        if (!hasReminded) {
            ok = executeSql("ALTER TABLE tasks ADD COLUMN reminded INTEGER NOT NULL DEFAULT 0;");
        }
        
        if (ok) {
            executeSql("INSERT INTO schema_version (version, applied_at) VALUES (2, datetime('now'))");
            m_db.commit();
            currentVersion = 2;
        } else {
            m_db.rollback();
            qWarning() << "[Database] Falló la migración v2";
            return false;
        }
    }

    if (currentVersion < 3) {
        qDebug() << "[Database] Aplicando migración v3 (tracking de actualizaciones y eliminaciones)...";
        m_db.transaction();

        bool hasUpdatedAt = false;
        QSqlQuery infoQuery(m_db);
        if (infoQuery.exec("PRAGMA table_info(tasks)")) {
            while (infoQuery.next()) {
                if (infoQuery.value("name").toString() == "updated_at") {
                    hasUpdatedAt = true;
                    break;
                }
            }
        }

        bool ok = true;
        if (!hasUpdatedAt) {
            ok = executeSql("ALTER TABLE tasks ADD COLUMN updated_at TEXT;");
            if (ok) {
                executeSql("UPDATE tasks SET updated_at = coalesce(completed_at, created_at, datetime('now')) WHERE updated_at IS NULL;");
            }
        }

        if (ok) {
            ok = executeSql(R"(
                CREATE TABLE IF NOT EXISTS deleted_tasks (
                    id              TEXT PRIMARY KEY,
                    remote_id       TEXT NOT NULL,
                    list_remote_id  TEXT,
                    deleted_at      TEXT NOT NULL
                );
            )");
        }

        if (ok) {
            executeSql("INSERT INTO schema_version (version, applied_at) VALUES (3, datetime('now'))");
            m_db.commit();
            currentVersion = 3;
        } else {
            m_db.rollback();
            qWarning() << "[Database] Falló la migración v3";
            return false;
        }
    }

    if (currentVersion < 4) {
        qDebug() << "[Database] Aplicando migración v4 (índices para optimización de queries)...";
        m_db.transaction();

        bool ok = executeSql("CREATE INDEX IF NOT EXISTS idx_tasks_list_completed ON tasks(list_id, is_completed);")
               && executeSql("CREATE INDEX IF NOT EXISTS idx_tasks_reminders ON tasks(reminder_at, is_completed, reminded);")
               && executeSql("CREATE INDEX IF NOT EXISTS idx_tasks_due_date ON tasks(due_date);")
               && executeSql("CREATE INDEX IF NOT EXISTS idx_tasks_remote_id ON tasks(remote_id);")
               && executeSql("CREATE INDEX IF NOT EXISTS idx_task_steps_task_id ON task_steps(task_id);")
               && executeSql("CREATE INDEX IF NOT EXISTS idx_task_lists_remote_id ON task_lists(remote_id);");

        if (ok) {
            executeSql("INSERT INTO schema_version (version, applied_at) VALUES (4, datetime('now'))");
            m_db.commit();
            currentVersion = 4;
        } else {
            m_db.rollback();
            qWarning() << "[Database] Falló la migración v4";
            return false;
        }
    }

    if (currentVersion < 5) {
        qDebug() << "[Database] Aplicando migración v5 (tracking de subtareas eliminadas)...";
        m_db.transaction();

        bool ok = executeSql(R"(
            CREATE TABLE IF NOT EXISTS deleted_steps (
                id              TEXT PRIMARY KEY,
                task_id         TEXT,
                remote_id       TEXT NOT NULL,
                task_remote_id  TEXT NOT NULL,
                list_remote_id  TEXT NOT NULL,
                deleted_at      TEXT NOT NULL
            );
        )") && executeSql("CREATE INDEX IF NOT EXISTS idx_deleted_steps_task_remote_id ON deleted_steps(task_remote_id);");

        if (ok) {
            executeSql("INSERT INTO schema_version (version, applied_at) VALUES (5, datetime('now'))");
            m_db.commit();
            currentVersion = 5;
        } else {
            m_db.rollback();
            qWarning() << "[Database] Falló la migración v5";
            return false;
        }
    }

    if (currentVersion < 6) {
        qDebug() << "[Database] Aplicando migración v6 (tracking de subtareas y sincronización granular)...";
        m_db.transaction();

        bool hasUpdatedAt = false;
        bool hasSyncedAt = false;
        QSqlQuery infoQuery(m_db);
        if (infoQuery.exec("PRAGMA table_info(task_steps)")) {
            while (infoQuery.next()) {
                QString colName = infoQuery.value("name").toString();
                if (colName == "updated_at") hasUpdatedAt = true;
                if (colName == "synced_at") hasSyncedAt = true;
            }
        }

        bool ok = true;
        if (!hasUpdatedAt) {
            ok = ok && executeSql("ALTER TABLE task_steps ADD COLUMN updated_at TEXT;");
        }
        if (!hasSyncedAt) {
            ok = ok && executeSql("ALTER TABLE task_steps ADD COLUMN synced_at TEXT;");
        }

        if (ok) {
            executeSql("UPDATE task_steps SET updated_at = datetime('now') WHERE updated_at IS NULL;");
            executeSql("UPDATE task_steps SET synced_at = datetime('now') WHERE remote_id IS NOT NULL AND synced_at IS NULL;");
            executeSql("CREATE INDEX IF NOT EXISTS idx_task_steps_remote_id ON task_steps(remote_id);");
            executeSql("INSERT INTO schema_version (version, applied_at) VALUES (6, datetime('now'))");
            m_db.commit();
            currentVersion = 6;
        } else {
            m_db.rollback();
            qWarning() << "[Database] Falló la migración v6";
            return false;
        }
    }
    
    // Asegurar que las listas especiales y la lista default-tasks siempre existan
    executeSql(R"(
        INSERT OR IGNORE INTO task_lists (id, display_name, is_special, sort_order, created_at)
        VALUES
          ('special-myday',    'My Day',   1, 0, datetime('now')),
          ('special-schedule', 'Schedule', 1, 1, datetime('now')),
          ('special-tasks',    'Tasks',    1, 2, datetime('now')),
          ('default-tasks',    'Tasks',    0, 3, datetime('now'));
    )");

    qDebug() << "[Database] Schema actualizado a versión:" << currentVersion;
    return true;
}
