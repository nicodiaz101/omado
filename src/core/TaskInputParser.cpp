#include "TaskInputParser.h"
#include <QRegularExpression>
#include <QMap>
#include <algorithm>

static int parseMonthName(const QString &str) {
    QString m = str.toLower().trimmed();
    if (m == "enero" || m == "january" || m == "jan") return 1;
    if (m == "febrero" || m == "february" || m == "feb") return 2;
    if (m == "marzo" || m == "march" || m == "mar") return 3;
    if (m == "abril" || m == "april" || m == "apr") return 4;
    if (m == "mayo" || m == "may") return 5;
    if (m == "junio" || m == "june" || m == "jun") return 6;
    if (m == "julio" || m == "july" || m == "jul") return 7;
    if (m == "agosto" || m == "august" || m == "aug") return 8;
    if (m == "septiembre" || m == "setiembre" || m == "september" || m == "sep" || m == "sept") return 9;
    if (m == "octubre" || m == "october" || m == "oct") return 10;
    if (m == "noviembre" || m == "november" || m == "nov") return 11;
    if (m == "diciembre" || m == "december" || m == "dec") return 12;
    return -1;
}

static int parseWeekdayName(const QString &str) {
    QString w = str.toLower().trimmed();
    if (w == "lunes" || w == "monday") return 1;
    if (w == "martes" || w == "tuesday") return 2;
    if (w == "miercoles" || w == "miércoles" || w == "wednesday") return 3;
    if (w == "jueves" || w == "thursday") return 4;
    if (w == "viernes" || w == "friday") return 5;
    if (w == "sabado" || w == "sábado" || w == "saturday") return 6;
    if (w == "domingo" || w == "sunday") return 7;
    return -1;
}

bool TaskInputParser::extractTime(const QString &text, QTime &time, MatchSpan &span) {
    // 1. Named times: mediodía / noon, medianoche / midnight
    {
        static const QRegularExpression rxNoon(QStringLiteral(R"(\b(?:al\s+)?(?:mediod[ií]a|noon)\b)"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = rxNoon.match(text);
        if (m.hasMatch()) {
            time = QTime(12, 0);
            span.start = m.capturedStart();
            span.length = m.capturedLength();
            return true;
        }

        static const QRegularExpression rxMidnight(QStringLiteral(R"(\b(?:a\s+la\s+)?(?:medianoche|midnight)\b)"), QRegularExpression::CaseInsensitiveOption);
        m = rxMidnight.match(text);
        if (m.hasMatch()) {
            time = QTime(0, 0);
            span.start = m.capturedStart();
            span.length = m.capturedLength();
            return true;
        }
    }

    // 2. Standard time HH:mm with optional am/pm/hs/hrs
    {
        static const QRegularExpression rxTimeCol(
            QStringLiteral(R"(\b(?:(?:a\s+las?|at)\s+)?(\d{1,2})[:.](\d{2})(?:\s*(am|pm|hs?|hrs?))?\b)"),
            QRegularExpression::CaseInsensitiveOption
        );
        QRegularExpressionMatch m = rxTimeCol.match(text);
        if (m.hasMatch()) {
            int h = m.captured(1).toInt();
            int min = m.captured(2).toInt();
            QString ampm = m.captured(3).toLower();

            if (ampm == "pm" && h < 12) h += 12;
            else if (ampm == "am" && h == 12) h = 0;

            if (h >= 0 && h <= 23 && min >= 0 && min <= 59) {
                time = QTime(h, min);
                span.start = m.capturedStart();
                span.length = m.capturedLength();
                return true;
            }
        }
    }

    // 3. 12h time with am/pm (e.g. "at 6pm", "6pm", "a las 6pm", "6 am")
    {
        static const QRegularExpression rxAmPm(
            QStringLiteral(R"(\b(?:(?:a\s+las?|at)\s+)?(\d{1,2})\s*(am|pm)\b)"),
            QRegularExpression::CaseInsensitiveOption
        );
        QRegularExpressionMatch m = rxAmPm.match(text);
        if (m.hasMatch()) {
            int h = m.captured(1).toInt();
            QString ampm = m.captured(2).toLower();
            if (ampm == "pm" && h < 12) h += 12;
            else if (ampm == "am" && h == 12) h = 0;

            if (h >= 0 && h <= 23) {
                time = QTime(h, 0);
                span.start = m.capturedStart();
                span.length = m.capturedLength();
                return true;
            }
        }
    }

    // 4. Time with "a las HH" / "at HH" or "HHhs"
    {
        static const QRegularExpression rxAtHour(
            QStringLiteral(R"(\b(?:a\s+las?|at)\s+(\d{1,2})(?:\s*(?:hs?|hrs?))?\b)"),
            QRegularExpression::CaseInsensitiveOption
        );
        QRegularExpressionMatch m = rxAtHour.match(text);
        if (m.hasMatch()) {
            int h = m.captured(1).toInt();
            if (h >= 0 && h <= 23) {
                time = QTime(h, 0);
                span.start = m.capturedStart();
                span.length = m.capturedLength();
                return true;
            }
        }

        static const QRegularExpression rxHourHs(
            QStringLiteral(R"(\b(\d{1,2})\s*(?:hs|hrs)\b)"),
            QRegularExpression::CaseInsensitiveOption
        );
        m = rxHourHs.match(text);
        if (m.hasMatch()) {
            int h = m.captured(1).toInt();
            if (h >= 0 && h <= 23) {
                time = QTime(h, 0);
                span.start = m.capturedStart();
                span.length = m.capturedLength();
                return true;
            }
        }
    }

    return false;
}

bool TaskInputParser::extractDate(const QString &text, const QDateTime &now, const QTime &detectedTime, bool hasDetectedTime, QDate &date, MatchSpan &span) {
    // 1. Relative days
    {
        static const QRegularExpression rxDayAfter(
            QStringLiteral(R"(\b(?:(?:para|for)\s+)?(?:pasado\s+ma[ñn]ana|day\s+after\s+tomorrow)\b)"),
            QRegularExpression::CaseInsensitiveOption
        );
        QRegularExpressionMatch m = rxDayAfter.match(text);
        if (m.hasMatch()) {
            date = now.date().addDays(2);
            span.start = m.capturedStart();
            span.length = m.capturedLength();
            return true;
        }

        static const QRegularExpression rxTomorrow(
            QStringLiteral(R"(\b(?:(?:para|for)\s+)?(?:ma[ñn]ana|tomorrow)\b)"),
            QRegularExpression::CaseInsensitiveOption
        );
        m = rxTomorrow.match(text);
        if (m.hasMatch()) {
            date = now.date().addDays(1);
            span.start = m.capturedStart();
            span.length = m.capturedLength();
            return true;
        }

        static const QRegularExpression rxToday(
            QStringLiteral(R"(\b(?:(?:para|for)\s+)?(?:hoy|today)\b)"),
            QRegularExpression::CaseInsensitiveOption
        );
        m = rxToday.match(text);
        if (m.hasMatch()) {
            date = now.date();
            span.start = m.capturedStart();
            span.length = m.capturedLength();
            return true;
        }
    }

    // 2. Weekdays (e.g. "el viernes", "próximo lunes", "next monday", "friday")
    {
        static const QRegularExpression rxWeekday(
            QStringLiteral(R"(\b(?:(?:para|for)\s+)?(?:(?:el|este|pr[oó]ximo|proximo|on|this|next)\s+)?(lunes|martes|mi[eé]rcoles|miercoles|jueves|viernes|s[aá]bado|sabado|domingo|monday|tuesday|wednesday|thursday|friday|saturday|sunday)\b)"),
            QRegularExpression::CaseInsensitiveOption
        );
        QRegularExpressionMatch m = rxWeekday.match(text);
        if (m.hasMatch()) {
            int targetDow = parseWeekdayName(m.captured(1));
            if (targetDow > 0) {
                QString matchedFull = m.captured(0).toLower();
                bool isNext = matchedFull.contains("próximo") || matchedFull.contains("proximo") || matchedFull.contains("next");
                int todayDow = now.date().dayOfWeek();
                int diff = targetDow - todayDow;

                if (diff < 0) {
                    diff += 7;
                } else if (diff == 0) {
                    if (isNext) {
                        diff = 7;
                    } else if (hasDetectedTime) {
                        if (detectedTime <= now.time()) {
                            diff = 7;
                        } else {
                            diff = 0; // Hoy más tarde
                        }
                    } else {
                        diff = 7; // Es el mismo día, sin hora -> próxima semana
                    }
                }

                date = now.date().addDays(diff);
                span.start = m.capturedStart();
                span.length = m.capturedLength();
                return true;
            }
        }
    }

    // 3. Numeric dates: DD/MM or DD/MM/YYYY or DD-MM-YYYY
    {
        static const QRegularExpression rxNumDate(
            QStringLiteral(R"(\b(?:(?:para|for)\s+)?(?:(?:el|on|by)\s+)?(\d{1,2})[\/\-](\d{1,2})(?:[\/\-](\d{2,4}))?\b)"),
            QRegularExpression::CaseInsensitiveOption
        );
        QRegularExpressionMatch m = rxNumDate.match(text);
        if (m.hasMatch()) {
            int d = m.captured(1).toInt();
            int mon = m.captured(2).toInt();
            int y = m.captured(3).isEmpty() ? now.date().year() : m.captured(3).toInt();
            if (y < 100) y += 2000;

            if (mon >= 1 && mon <= 12 && d >= 1 && d <= 31) {
                QDate candidate(y, mon, d);
                if (candidate.isValid()) {
                    if (m.captured(3).isEmpty() && candidate < now.date()) {
                        candidate = candidate.addYears(1);
                    }
                    date = candidate;
                    span.start = m.capturedStart();
                    span.length = m.capturedLength();
                    return true;
                }
            }
        }
    }

    // 4. Textual month dates: "15 de mayo", "15 may", "may 15th"
    {
        static const QString monthWords = QStringLiteral(
            R"(enero|febrero|marzo|abril|mayo|junio|julio|agosto|septiembre|setiembre|octubre|noviembre|diciembre|january|february|march|april|may|june|july|august|september|october|november|december|jan|feb|mar|apr|jun|jul|aug|sep|sept|oct|nov|dec)"
        );

        // Format A: 15 [de] mayo [2026]
        static const QRegularExpression rxDayMonth(
            QStringLiteral(R"(\b(?:(?:para|for)\s+)?(?:(?:el|on|by)\s+)?(\d{1,2})(?:st|nd|rd|th)?(?:\s+de)?\s+()") + monthWords + QStringLiteral(R"()(?:\s+(?:de\s+)?(\d{4}))?\b)"),
            QRegularExpression::CaseInsensitiveOption
        );
        QRegularExpressionMatch m = rxDayMonth.match(text);
        if (m.hasMatch()) {
            int d = m.captured(1).toInt();
            int mon = parseMonthName(m.captured(2));
            int y = m.captured(3).isEmpty() ? now.date().year() : m.captured(3).toInt();

            if (mon > 0 && d >= 1 && d <= 31) {
                QDate candidate(y, mon, d);
                if (candidate.isValid()) {
                    if (m.captured(3).isEmpty() && candidate < now.date()) {
                        candidate = candidate.addYears(1);
                    }
                    date = candidate;
                    span.start = m.capturedStart();
                    span.length = m.capturedLength();
                    return true;
                }
            }
        }

        // Format B: May 15[th] [2026]
        static const QRegularExpression rxMonthDay(
            QStringLiteral(R"(\b(?:(?:para|for)\s+)?(?:(?:el|on|by)\s+)?()") + monthWords + QStringLiteral(R"()\s+(\d{1,2})(?:st|nd|rd|th)?(?:\s+(?:de\s+)?(\d{4}))?\b)"),
            QRegularExpression::CaseInsensitiveOption
        );
        m = rxMonthDay.match(text);
        if (m.hasMatch()) {
            int mon = parseMonthName(m.captured(1));
            int d = m.captured(2).toInt();
            int y = m.captured(3).isEmpty() ? now.date().year() : m.captured(3).toInt();

            if (mon > 0 && d >= 1 && d <= 31) {
                QDate candidate(y, mon, d);
                if (candidate.isValid()) {
                    if (m.captured(3).isEmpty() && candidate < now.date()) {
                        candidate = candidate.addYears(1);
                    }
                    date = candidate;
                    span.start = m.capturedStart();
                    span.length = m.capturedLength();
                    return true;
                }
            }
        }
    }

    return false;
}

QString TaskInputParser::cleanTitle(const QString &original, const QList<MatchSpan> &spans) {
    if (spans.isEmpty()) return original.trimmed();

    QList<MatchSpan> sortedSpans = spans;
    // Filtrar inválidos
    sortedSpans.erase(std::remove_if(sortedSpans.begin(), sortedSpans.end(), [](const MatchSpan &s) {
        return !s.isValid();
    }), sortedSpans.end());

    if (sortedSpans.isEmpty()) return original.trimmed();

    // Expandir cada span para absorber preposiciones anteriores inmediatas (p. ej. "para el", "para", "a las", "at", "on", "by")
    static const QRegularExpression rxLeadingPrep(
        QStringLiteral(R"(\b(?:para\s+el|para\s+la|para\s+las?|para|a\s+las?|a\s+la|at|on|by|for)\s*$)"),
        QRegularExpression::CaseInsensitiveOption
    );

    for (MatchSpan &s : sortedSpans) {
        QString before = original.left(s.start);
        QRegularExpressionMatch pm = rxLeadingPrep.match(before);
        if (pm.hasMatch() && pm.capturedEnd() == before.length()) {
            int prepLen = pm.capturedLength();
            s.start -= prepLen;
            s.length += prepLen;
        }
    }

    // Ordenar de mayor a menor índice para borrar sin desfasar posiciones
    std::sort(sortedSpans.begin(), sortedSpans.end(), [](const MatchSpan &a, const MatchSpan &b) {
        return a.start > b.start;
    });

    QString cleaned = original;
    for (const MatchSpan &s : sortedSpans) {
        if (s.start >= 0 && s.start + s.length <= cleaned.length()) {
            cleaned.remove(s.start, s.length);
        }
    }

    // Limpiar preposiciones colgantes al final (ej. "Comprar pan para" -> "Comprar pan")
    static const QRegularExpression rxTrailingPrep(
        QStringLiteral(R"(\s+\b(?:para\s+el|para\s+la|para\s+las?|para|a\s+las?|a\s+la|at|on|by|for)$)"),
        QRegularExpression::CaseInsensitiveOption
    );
    cleaned.remove(rxTrailingPrep);

    // Colapsar espacios múltiples y trim
    static const QRegularExpression rxSpaces(QStringLiteral(R"(\s+)"));
    cleaned = cleaned.replace(rxSpaces, QStringLiteral(" ")).trimmed();

    // Si quedó vacío (porque el usuario puso únicamente la fecha u hora), devolvemos el texto original
    if (cleaned.isEmpty()) {
        return original.trimmed();
    }

    return cleaned;
}

TaskParseResult TaskInputParser::parse(const QString &input, const QDateTime &now) {
    TaskParseResult result;
    if (input.trimmed().isEmpty()) return result;

    QTime detectedTime;
    MatchSpan timeSpan;
    bool hasTime = extractTime(input, detectedTime, timeSpan);

    QDate detectedDate;
    MatchSpan dateSpan;
    bool hasDate = extractDate(input, now, detectedTime, hasTime, detectedDate, dateSpan);

    QList<MatchSpan> spans;
    if (timeSpan.isValid()) spans.append(timeSpan);
    if (dateSpan.isValid()) spans.append(dateSpan);

    result.cleanTitle = cleanTitle(input, spans);

    if (hasDate && !hasTime) {
        // Solo día -> fecha de vencimiento
        result.hasDueDate = true;
        result.dueDate = detectedDate;
        result.hasReminder = false;
    } else if (hasDate && hasTime) {
        // Día y hora -> fecha de vencimiento y recordatorio
        result.hasDueDate = true;
        result.dueDate = detectedDate;
        result.hasReminder = true;
        result.reminderAt = QDateTime(detectedDate, detectedTime);
    } else if (!hasDate && hasTime) {
        // Solo hora -> fecha de vencimiento hoy (o mañana si ya pasó la hora) y recordatorio
        QDate targetDate = now.date();
        if (detectedTime <= now.time()) {
            targetDate = targetDate.addDays(1);
        }
        result.hasDueDate = true;
        result.dueDate = targetDate;
        result.hasReminder = true;
        result.reminderAt = QDateTime(targetDate, detectedTime);
    }

    return result;
}
