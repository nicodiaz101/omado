#pragma once
#include <QString>
#include <QtDBus/QDBusArgument>
#include <QMetaType>

struct TaskList {
    QString  id;
    QString  displayName;
    bool     isSpecial   = false;
    int      sortOrder   = 0;
    QString  remoteId;
    
    bool isSynced() const { return !remoteId.isEmpty(); }
};

struct OmaDoListEntry {
    QString id;
    QString displayName;
};
Q_DECLARE_METATYPE(OmaDoListEntry)

inline QDBusArgument &operator<<(QDBusArgument &argument, const OmaDoListEntry &entry) {
    argument.beginStructure();
    argument << entry.id << entry.displayName;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, OmaDoListEntry &entry) {
    argument.beginStructure();
    argument >> entry.id >> entry.displayName;
    argument.endStructure();
    return argument;
}
