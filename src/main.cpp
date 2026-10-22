#include "core/Database.h"
#include "core/ThemeReader.h"
#include "core/NotificationService.h"
#include "core/ReminderService.h"
#include "core/DaemonService.h"
#include "core/KeychainStore.h"
#include "core/AuthManager.h"
#include "models/GraphClient.h"
#include "core/SyncEngine.h"
#include "models/LocalRepository.h"
#include "models/TaskListModel.h"
#include "models/TaskModel.h"
#include "core/Translator.h"
#include "core/CliRunner.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QFontDatabase>
#include <QIcon>
#include <QtDBus/QDBusConnection>
#include <QDebug>
#include <QTextStream>

int main(int argc, char *argv[])
{
    bool isDaemon = false;
    bool isQuickAdd = false;
    QStringList cliArgs;

    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);
        if (arg == "--daemon" || arg == "-d") {
            isDaemon = true;
        } else if (arg == "--quick-add") {
            isQuickAdd = true;
        } else if (arg == "--version" || arg == "-v") {
            QTextStream(stdout) << "OmaDo v2.0.0 (Omarchy 4.5)" << Qt::endl;
            return 0;
        } else if (!arg.startsWith("-")) {
            // Comandos CLI (add, list, done, reschedule)
            for (int j = i; j < argc; ++j) {
                cliArgs.append(QString::fromUtf8(argv[j]));
            }
            break;
        }
    }

    if (!cliArgs.isEmpty()) {
        QCoreApplication app(argc, argv);
        app.setOrganizationName("omacom-io");
        app.setApplicationName("OmaDo");
        app.setApplicationVersion("2.0.0");

        Database db;
        if (!db.initialize()) {
            return 1;
        }
        LocalRepository repo;
        return CliRunner::run(cliArgs, &repo);
    }

    if (isDaemon) {
        QCoreApplication app(argc, argv);
        app.setOrganizationName("omacom-io");
        app.setApplicationName("OmaDo");
        app.setApplicationVersion("2.0.0");

        Database db;
        if (!db.initialize()) {
            qWarning() << "[Daemon] No se pudo inicializar la base de datos";
            return 1;
        }

        LocalRepository repo;
        NotificationService notifier;
        ReminderService reminderService(&repo, &notifier);
        reminderService.start();

        KeychainStore keychain;
        AuthManager authManager(&keychain);
        GraphClient graphClient(&authManager);
        SyncEngine syncEngine(&authManager, &graphClient, &repo);

        DaemonService daemonService(&repo);
        QObject::connect(&reminderService, &ReminderService::reminderActionProcessed,
                         &daemonService, [&daemonService](const QString &taskId, const QString &actionKey) {
            Q_UNUSED(taskId);
            Q_UNUSED(actionKey);
            emit daemonService.TodayTasksChanged();
            emit daemonService.TasksChanged(QString());
        });
        QObject::connect(&daemonService, &DaemonService::SyncRequested, &syncEngine, &SyncEngine::syncNow);
        QObject::connect(&syncEngine, &SyncEngine::syncStarted, &daemonService, &DaemonService::SyncStarted);
        QObject::connect(&syncEngine, &SyncEngine::syncFinished, &daemonService, [&daemonService](bool success, const QString &message) {
            emit daemonService.SyncFinished(success, message);
            emit daemonService.TodayTasksChanged();
            emit daemonService.TasksChanged(QString());
        });

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
    app.setApplicationVersion("2.0.0");
    app.setDesktopFileName(QStringLiteral("omado"));
    app.setWindowIcon(QIcon(":/qt/qml/OmaDo/icons/omado.svg"));

    // Cargar fuentes
    int fontId = QFontDatabase::addApplicationFont(":/qt/qml/OmaDo/fonts/iAWriterMono-Regular.ttf");
    if (fontId == -1) {
        fontId = QFontDatabase::addApplicationFont(":/OmaDo/fonts/iAWriterMono-Regular.ttf"); // fallback Qt < 6.5
        if (fontId == -1) {
            qWarning() << "No se pudo cargar iAWriterMono-Regular.ttf";
        }
    }
    int boldFontId = QFontDatabase::addApplicationFont(":/qt/qml/OmaDo/fonts/iAWriterMono-Bold.ttf");
    if (boldFontId == -1) {
        boldFontId = QFontDatabase::addApplicationFont(":/OmaDo/fonts/iAWriterMono-Bold.ttf"); // fallback Qt < 6.5
        if (boldFontId == -1) {
            qWarning() << "No se pudo cargar iAWriterMono-Bold.ttf";
        }
    }

    Database *db = new Database(&app);
    if (!db->initialize()) {
        qWarning() << "No se pudo inicializar la base de datos";
        return 1;
    }

    LocalRepository *repo = new LocalRepository(&app);
    
    KeychainStore *keychain = new KeychainStore(&app);
    AuthManager *authManager = new AuthManager(keychain, &app);
    SyncEngine *syncEngine = new SyncEngine(authManager, nullptr, repo, &app);

    TaskListModel *listModel = new TaskListModel(repo, nullptr, syncEngine, &app);
    TaskModel *taskModel = new TaskModel(repo, nullptr, syncEngine, &app);

    QObject::connect(syncEngine, &SyncEngine::syncFinished, listModel, [listModel, taskModel](bool ok, const QString &) {
        if (ok) {
            listModel->loadLists();
            taskModel->loadTasks();
        }
    });

    // Escuchar cambios D-Bus desde el daemon u otros procesos de Omarchy para refresco instantáneo
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected()) {
        bus.connect(
            QString(),
            QStringLiteral("/io/omarchy/OmaDo"),
            QStringLiteral("io.omarchy.OmaDo"),
            QStringLiteral("TodayTasksChanged"),
            taskModel,
            SLOT(loadTasks())
        );
        bus.connect(
            QString(),
            QStringLiteral("/io/omarchy/OmaDo"),
            QStringLiteral("io.omarchy.OmaDo"),
            QStringLiteral("TasksChanged"),
            taskModel,
            SLOT(loadTasks())
        );
        bus.connect(
            QString(),
            QStringLiteral("/io/omarchy/OmaDo"),
            QStringLiteral("io.omarchy.OmaDo"),
            QStringLiteral("TasksChanged"),
            listModel,
            SLOT(loadLists())
        );
    }

    qmlRegisterSingletonInstance("OmaDo.Models", 1, 0, "TaskListModel", listModel);
    qmlRegisterSingletonInstance("OmaDo.Models", 1, 0, "TaskModel", taskModel);
    qmlRegisterSingletonInstance("OmaDo.Auth", 1, 0, "AuthManager", authManager);
    qmlRegisterSingletonInstance("OmaDo.Sync", 1, 0, "SyncEngine", syncEngine);

    ThemeReader *themeReader = new ThemeReader(&app);
    qmlRegisterSingletonInstance("OmaDo.Theme", 1, 0, "Theme", themeReader);

    QQmlApplicationEngine engine;
    engine.addImportPath(":/");
    engine.addImportPath(":/qt/qml");

    // Instalar traductor multilenguaje con observación en vivo de Omarchy
    Translator *translator = new Translator(&engine, &app);
    app.installTranslator(translator);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    
    if (isQuickAdd) {
        engine.loadFromModule("OmaDo", "QuickAddWindow");
    } else {
        engine.loadFromModule("OmaDo", "Main");
    }

    return app.exec();
}
