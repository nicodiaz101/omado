#include <QtTest>
#include "core/TaskInputParser.h"

class tst_TaskInputParser : public QObject {
    Q_OBJECT

private slots:
    void testRelativeDaysOnlyDate() {
        QDateTime fixedNow(QDate(2026, 9, 9), QTime(10, 0)); // Miércoles

        // 1. Español: mañana
        {
            auto res = TaskInputParser::parse("Comprar leche mañana", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Comprar leche"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 10));
            QVERIFY(!res.hasReminder);
        }

        // 2. Inglés: tomorrow
        {
            auto res = TaskInputParser::parse("Buy groceries tomorrow", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Buy groceries"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 10));
            QVERIFY(!res.hasReminder);
        }

        // 3. Pasado mañana
        {
            auto res = TaskInputParser::parse("Pagar facturas pasado mañana", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Pagar facturas"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 11));
            QVERIFY(!res.hasReminder);
        }

        // 4. Day after tomorrow
        {
            auto res = TaskInputParser::parse("Deliver report day after tomorrow", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Deliver report"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 11));
            QVERIFY(!res.hasReminder);
        }

        // 5. Hoy
        {
            auto res = TaskInputParser::parse("Terminar tarea hoy", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Terminar tarea"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 9));
            QVERIFY(!res.hasReminder);
        }
    }

    void testDateAndTimeCombined() {
        QDateTime fixedNow(QDate(2026, 9, 9), QTime(10, 0));

        // 1. Español con "a las 18:00"
        {
            auto res = TaskInputParser::parse("Comprar pan mañana a las 18:00", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Comprar pan"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 10));
            QVERIFY(res.hasReminder);
            QCOMPARE(res.reminderAt, QDateTime(QDate(2026, 9, 10), QTime(18, 0)));
        }

        // 2. Inglés con "at 6pm"
        {
            auto res = TaskInputParser::parse("Call dentist tomorrow at 6pm", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Call dentist"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 10));
            QVERIFY(res.hasReminder);
            QCOMPARE(res.reminderAt, QDateTime(QDate(2026, 9, 10), QTime(18, 0)));
        }

        // 3. Formato 18hs
        {
            auto res = TaskInputParser::parse("Reunión de equipo pasado mañana 15hs", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Reunión de equipo"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 11));
            QVERIFY(res.hasReminder);
            QCOMPARE(res.reminderAt, QDateTime(QDate(2026, 9, 11), QTime(15, 0)));
        }
    }

    void testWeekdays() {
        QDateTime fixedNow(QDate(2026, 9, 9), QTime(10, 0)); // Miércoles (día 3)

        // 1. Viernes (día 5 -> +2 días = 11 de septiembre)
        {
            auto res = TaskInputParser::parse("Ir al gimnasio este viernes", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Ir al gimnasio"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 11));
            QVERIFY(!res.hasReminder);
        }

        // 2. Viernes con hora
        {
            auto res = TaskInputParser::parse("Cena con amigos el viernes a las 21:30", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Cena con amigos"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 11));
            QVERIFY(res.hasReminder);
            QCOMPARE(res.reminderAt, QDateTime(QDate(2026, 9, 11), QTime(21, 30)));
        }

        // 3. Next monday (Lunes siguiente -> +5 días = 14 de septiembre)
        {
            auto res = TaskInputParser::parse("Sprint review next monday at 10am", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Sprint review"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 14));
            QVERIFY(res.hasReminder);
            QCOMPARE(res.reminderAt, QDateTime(QDate(2026, 9, 14), QTime(10, 0)));
        }
    }

    void testNumericAndMonthDates() {
        QDateTime fixedNow(QDate(2026, 9, 9), QTime(10, 0));

        // 1. DD/MM
        {
            auto res = TaskInputParser::parse("Pagar tarjeta 25/10", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Pagar tarjeta"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 10, 25));
            QVERIFY(!res.hasReminder);
        }

        // 2. Textual mes en español con hora
        {
            auto res = TaskInputParser::parse("Cumpleaños de mamá 15 de octubre a las 20:00", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Cumpleaños de mamá"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 10, 15));
            QVERIFY(res.hasReminder);
            QCOMPARE(res.reminderAt, QDateTime(QDate(2026, 10, 15), QTime(20, 0)));
        }

        // 3. Textual mes en inglés
        {
            auto res = TaskInputParser::parse("Conference Dec 25 at 3pm", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Conference"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 12, 25));
            QVERIFY(res.hasReminder);
            QCOMPARE(res.reminderAt, QDateTime(QDate(2026, 12, 25), QTime(15, 0)));
        }
    }

    void testOnlyTimeInput() {
        QDateTime fixedNow(QDate(2026, 9, 9), QTime(10, 0));

        // 1. Hora futura en el mismo día (15:00 > 10:00) -> hoy
        {
            auto res = TaskInputParser::parse("Tomar remedio a las 15:00", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Tomar remedio"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 9));
            QVERIFY(res.hasReminder);
            QCOMPARE(res.reminderAt, QDateTime(QDate(2026, 9, 9), QTime(15, 0)));
        }

        // 2. Hora pasada en el mismo día (08:00 < 10:00) -> mañana
        {
            auto res = TaskInputParser::parse("Despertarse a las 8am", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Despertarse"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 10));
            QVERIFY(res.hasReminder);
            QCOMPARE(res.reminderAt, QDateTime(QDate(2026, 9, 10), QTime(8, 0)));
        }
    }

    void testFallbackWhenEntireTitleIsDate() {
        QDateTime fixedNow(QDate(2026, 9, 9), QTime(10, 0));

        // Si el usuario escribe solo "Mañana", no debe quedar un título en blanco
        {
            auto res = TaskInputParser::parse("Mañana", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Mañana"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 10));
        }

        {
            auto res = TaskInputParser::parse("Tomorrow at 6pm", fixedNow);
            QCOMPARE(res.cleanTitle, QStringLiteral("Tomorrow at 6pm"));
            QVERIFY(res.hasDueDate);
            QCOMPARE(res.dueDate, QDate(2026, 9, 10));
            QVERIFY(res.hasReminder);
        }
    }
};

QTEST_MAIN(tst_TaskInputParser)
#include "tst_TaskInputParser.moc"
