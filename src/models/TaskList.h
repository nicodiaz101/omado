#pragma once
#include <QString>

struct TaskList {
    QString  id;
    QString  displayName;
    bool     isSpecial   = false;
    int      sortOrder   = 0;
    QString  remoteId;
    
    bool isSynced() const { return !remoteId.isEmpty(); }
};
