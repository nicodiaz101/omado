#pragma once

#include <QString>
#include <QDate>
#include <QTime>
#include <QDateTime>

struct TaskParseResult {
    QString cleanTitle;
    QDate dueDate;
    QDateTime reminderAt;
    bool hasDueDate = false;
    bool hasReminder = false;
};

class TaskInputParser {
public:
    static TaskParseResult parse(const QString &input, const QDateTime &now = QDateTime::currentDateTime());

private:
    struct MatchSpan {
        int start = -1;
        int length = 0;
        bool isValid() const { return start >= 0 && length > 0; }
    };

    static bool extractTime(const QString &text, QTime &time, MatchSpan &span);
    static bool extractDate(const QString &text, const QDateTime &now, const QTime &detectedTime, bool hasDetectedTime, QDate &date, MatchSpan &span);
    static QString cleanTitle(const QString &original, const QList<MatchSpan> &spans);
};
