#include "Database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

Database::Database(QObject *parent) : QObject(parent) {
}

bool Database::initialize() {
    // 1. Asegurar que el directorio exista
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);
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
    
    // 3. Activar WAL mode y constrains de foreign keys
    executeSql("PRAGMA journal_mode=WAL;");
    executeSql("PRAGMA foreign_keys=ON;");
    executeSql("PRAGMA busy_timeout=5000;");
    
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
                remote_id       TEXT
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
    
    qDebug() << "[Database] Schema actualizado a versión:" << currentVersion;
    return true;
}
