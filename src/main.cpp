#include "core/Database.h"
#include "core/ThemeReader.h"
#include "core/NotificationService.h"
#include "core/ReminderService.h"
#include "core/DaemonService.h"
#include "models/LocalRepository.h"
#include "models/TaskListModel.h"
#include "models/TaskModel.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QFontDatabase>
#include <QDebug>

int main(int argc, char *argv[])
{
    bool isDaemon = false;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);
        if (arg == "--daemon" || arg == "-d") {
            isDaemon = true;
            break;
        }
    }

    if (isDaemon) {
        QCoreApplication app(argc, argv);
        app.setOrganizationName("omacom-io");
        app.setApplicationName("OmaDo");

        Database db;
        if (!db.initialize()) {
            qWarning() << "[Daemon] No se pudo inicializar la base de datos";
            return 1;
        }

        LocalRepository repo;
        NotificationService notifier;
        ReminderService reminderService(&repo, &notifier);
        reminderService.start();

        DaemonService daemonService(&repo);
        if (!daemonService.registerService()) {
            qWarning() << "[Daemon] No se pudo registrar el servicio D-Bus";
            return 1;
        }

        qDebug() << "[Daemon] OmaDo daemon iniciado correctamente";
        return app.exec();
    }

    // Modo GUI
    QGuiApplication app(argc, argv);
    app.setOrganizationName("omacom-io");
    app.setApplicationName("OmaDo");

    // Cargar fuente
    int fontId = QFontDatabase::addApplicationFont(":/qt/qml/OmaDo/fonts/iAWriterMono-Regular.ttf");
    if (fontId == -1) {
        fontId = QFontDatabase::addApplicationFont(":/OmaDo/fonts/iAWriterMono-Regular.ttf"); // fallback Qt < 6.5
        if (fontId == -1) {
            qWarning() << "No se pudo cargar iAWriterMono-Regular.ttf";
        }
    }

    Database *db = new Database(&app);
    if (!db->initialize()) {
        qWarning() << "No se pudo inicializar la base de datos";
        return 1;
    }

    LocalRepository *repo = new LocalRepository(&app);
    NotificationService *notifier = new NotificationService(&app);
    ReminderService *reminderService = new ReminderService(repo, notifier, &app);
    reminderService->start();

    TaskListModel *listModel = new TaskListModel(repo, &app);
    TaskModel *taskModel = new TaskModel(repo, &app);
    qmlRegisterSingletonInstance("OmaDo.Models", 1, 0, "TaskListModel", listModel);
    qmlRegisterSingletonInstance("OmaDo.Models", 1, 0, "TaskModel", taskModel);

    ThemeReader *themeReader = new ThemeReader(&app);
    qmlRegisterSingletonInstance("OmaDo.Theme", 1, 0, "Theme", themeReader);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    
    // Qt 6.5+ recomendado: loadFromModule
    engine.loadFromModule("OmaDo", "Main");

    return app.exec();
}
