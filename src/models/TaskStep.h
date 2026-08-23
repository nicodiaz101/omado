#pragma once
#include <QString>
#include <QVariantMap>

struct TaskStep {
    QString id;
    QString taskId;
    QString title;
    bool    isCompleted = false;
    int     sortOrder   = 0;

    QVariantMap toVariantMap() const {
        return {
            {"id", id},
            {"taskId", taskId},
            {"title", title},
            {"isCompleted", isCompleted},
            {"sortOrder", sortOrder}
        };
    }
};
