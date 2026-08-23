#include "models/LocalRepository.h"
#include "models/TaskListModel.h"
#include "models/TaskModel.h"
#include "core/Database.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QFontDatabase>
#include <QDebug>
#include "core/ThemeReader.h"

int main(int argc, char *argv[])
{
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
        qFatal("No se pudo inicializar la base de datos");
    }

    LocalRepository *repo = new LocalRepository(&app);
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
