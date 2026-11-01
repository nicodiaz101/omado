#include "CliRunner.h"
#include "DaemonService.h"
#include "models/LocalRepository.h"
#include "TaskInputParser.h"
#include <QTextStream>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <iostream>

int CliRunner::run(const QStringList &args, LocalRepository *repo) {
    if (args.isEmpty() || !repo) {
        return 1;
    }

    QString subCmd = args.at(0).toLower();
    bool useDBus = false;
    std::unique_ptr<QDBusInterface> daemonIface;

    if (!QStandardPaths::isTestModeEnabled()) {
        daemonIface = std::make_unique<QDBusInterface>(
            QStringLiteral("io.omarchy.OmaDo"),
            QStringLiteral("/io/omarchy/OmaDo"),
            QStringLiteral("io.omarchy.OmaDo"),
            QDBusConnection::sessionBus()
        );
        useDBus = daemonIface->isValid();
    }

    if (subCmd == QLatin1String("add")) {
        if (args.count() < 2) {
            std::cerr << "Uso: omado add \"<texto natural>\" [--today]" << std::endl;
            return 1;
        }

        bool addToMyDay = false;
        QStringList textParts;
        for (int i = 1; i < args.count(); ++i) {
            if (args.at(i) == QLatin1String("--today")) {
                addToMyDay = true;
            } else {
                textParts.append(args.at(i));
            }
        }

        QString rawInput = textParts.join(QLatin1Char(' '));
        QString taskId;

        if (useDBus) {
            QDBusReply<QString> reply = daemonIface->call(QStringLiteral("CreateTaskFromNaturalText"), rawInput, addToMyDay);
            if (reply.isValid()) {
                taskId = reply.value();
            }
        }
        if (taskId.isEmpty()) {
            DaemonService daemon(repo);
            taskId = daemon.CreateTaskFromNaturalText(rawInput, addToMyDay);
        }

        if (!taskId.isEmpty()) {
            std::cout << "Tarea creada [" << taskId.toStdString() << "]" << std::endl;
            return 0;
        } else {
            std::cerr << "Error al crear la tarea" << std::endl;
            return 1;
        }
    } else if (subCmd == QLatin1String("list")) {
        bool jsonOutput = args.contains(QLatin1String("--json"));
        bool todayOnly = args.contains(QLatin1String("--today"));

        if (todayOnly) {
            QString jsonStr;
            if (useDBus) {
                QDBusReply<QString> reply = daemonIface->call(QStringLiteral("GetTasksForToday"));
                if (reply.isValid()) {
                    jsonStr = reply.value();
                }
            }
            if (jsonStr.isEmpty()) {
                DaemonService daemon(repo);
                jsonStr = daemon.GetTasksForToday();
            }

            if (jsonOutput) {
                std::cout << jsonStr.toStdString() << std::endl;
            } else {
                QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
                QJsonArray arr = doc.array();
                if (arr.isEmpty()) {
                    std::cout << "No hay tareas para hoy." << std::endl;
                } else {
                    for (const auto &val : arr) {
                        QJsonObject o = val.toObject();
                        std::cout << (o.value("isCompleted").toBool() ? "[✓] " : "[ ] ")
                                  << o.value("title").toString().toStdString()
                                  << " (" << o.value("id").toString().toStdString() << ")"
                                  << std::endl;
                    }
                }
            }
            return 0;
        } else {
            DaemonService daemon(repo);
            auto lists = daemon.GetLists();
            if (jsonOutput) {
                QJsonArray arr;
                for (const auto &l : lists) {
                    QJsonObject o;
                    o["id"] = l.id;
                    o["displayName"] = l.displayName;
                    arr.append(o);
                }
                std::cout << QJsonDocument(arr).toJson(QJsonDocument::Compact).toStdString() << std::endl;
            } else {
                for (const auto &l : lists) {
                    std::cout << "• " << l.displayName.toStdString() << " [" << l.id.toStdString() << "]" << std::endl;
                }
            }
            return 0;
        }
    } else if (subCmd == QLatin1String("done")) {
        if (args.count() < 2) {
            std::cerr << "Uso: omado done <task-id>" << std::endl;
            return 1;
        }
        QString taskId = args.at(1);
        bool ok = false;
        if (useDBus) {
            QDBusReply<bool> reply = daemonIface->call(QStringLiteral("ToggleTask"), taskId, true);
            if (reply.isValid()) {
                ok = reply.value();
            }
        }
        if (!ok) {
            DaemonService daemon(repo);
            ok = daemon.ToggleTask(taskId, true);
        }

        if (ok) {
            std::cout << "Tarea completada: " << taskId.toStdString() << std::endl;
            return 0;
        } else {
            std::cerr << "Error completando tarea: " << taskId.toStdString() << std::endl;
            return 1;
        }
    } else if (subCmd == QLatin1String("reschedule")) {
        if (args.count() < 2) {
            std::cerr << "Uso: omado reschedule <task-id>" << std::endl;
            return 1;
        }
        QString taskId = args.at(1);
        bool ok = false;
        if (useDBus) {
            QDBusReply<bool> reply = daemonIface->call(QStringLiteral("RescheduleTaskForMyDay"), taskId);
            if (reply.isValid()) {
                ok = reply.value();
            }
        }
        if (!ok) {
            DaemonService daemon(repo);
            ok = daemon.RescheduleTaskForMyDay(taskId);
        }

        if (ok) {
            std::cout << "Tarea reprogramada para Mi Día: " << taskId.toStdString() << std::endl;
            return 0;
        } else {
            std::cerr << "Error reprogramando tarea: " << taskId.toStdString() << std::endl;
            return 1;
        }
    }

    std::cerr << "Subcomando desconocido: " << subCmd.toStdString() << std::endl;
    std::cerr << "Subcomandos disponibles: add, list, done, reschedule" << std::endl;
    return 1;
}
