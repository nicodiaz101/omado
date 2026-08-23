#pragma once

#include <QObject>
#include <QSqlDatabase>

class Database : public QObject {
    Q_OBJECT
public:
    explicit Database(QObject *parent = nullptr);
    
    // Inicializa la conexión y aplica migraciones.
    // Retorna true si todo fue exitoso.
    bool initialize();

private:
    bool applyMigrations();
    bool executeSql(const QString &sql);
    
    QSqlDatabase m_db;
};
