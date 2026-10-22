#include "CliRunner.h"
#include "DaemonService.h"
#include "models/LocalRepository.h"
#include "TaskInputParser.h"
#include <QTextStream>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <iostream>

int CliRunner::run(const QStringList &args, LocalRepository *repo) {
    if (args.isEmpty() || !repo) {
        return 1;
    }

    QString subCmd = args.at(0).toLower();

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
        DaemonService daemon(repo);
        QString taskId = daemon.CreateTaskFromNaturalText(rawInput, addToMyDay);

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

        DaemonService daemon(repo);
        if (todayOnly) {
            QString jsonStr = daemon.GetTasksForToday();
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
        DaemonService daemon(repo);
        bool ok = daemon.ToggleTask(taskId, true);
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
        DaemonService daemon(repo);
        bool ok = daemon.RescheduleTaskForMyDay(taskId);
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
