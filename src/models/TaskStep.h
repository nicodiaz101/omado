#pragma once
#include <QString>
#include <QVariantMap>

struct TaskStep {
    QString id;
    QString taskId;
    QString title;
    bool    isCompleted = false;
    int     sortOrder   = 0;
    QString remoteId;

    QVariantMap toVariantMap() const {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("taskId"), taskId},
            {QStringLiteral("title"), title},
            {QStringLiteral("isCompleted"), isCompleted},
            {QStringLiteral("sortOrder"), sortOrder},
            {QStringLiteral("remoteId"), remoteId}
        };
    }
};
