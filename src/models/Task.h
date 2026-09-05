#pragma once
#include <QString>
#include <QDateTime>
#include <QDate>
#include <QList>
#include "TaskStep.h"

struct Task {
    QString         id;
    QString         listId;
    QString         title;
    QString         body;
    bool            isCompleted  = false;
    bool            isMyDay      = false;
    QString         importance   = "normal";
    QDate           dueDate;
    QDateTime       reminderAt;
    bool            reminded     = false;
    QString         recurrence   = "none";
    QList<TaskStep> steps;
    int             sortOrder    = 0;
    QDateTime       createdAt;
    QDateTime       updatedAt;
    QDateTime       completedAt;
    QDateTime       syncedAt;
    QString         remoteId;
};
