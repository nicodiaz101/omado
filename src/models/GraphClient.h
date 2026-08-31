#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include "../core/AuthManager.h"
#include "Task.h"
#include "TaskList.h"
#include "TaskStep.h"

class GraphClient : public QObject {
    Q_OBJECT
public:
    explicit GraphClient(AuthManager *auth, QObject *parent = nullptr);

    // Listas
    void fetchLists(std::function<void(bool success, const QJsonArray &lists, const QString &error)> callback);
    void createList(const QString &displayName, std::function<void(bool success, const QJsonObject &listObj, const QString &error)> callback);
    void updateList(const QString &remoteListId, const QString &displayName, std::function<void(bool success, const QString &error)> callback);
    void deleteList(const QString &remoteListId, std::function<void(bool success, const QString &error)> callback);

    // Tareas
    void fetchTasks(const QString &remoteListId, std::function<void(bool success, const QJsonArray &tasks, const QString &error)> callback);
    void createTask(const QString &remoteListId, const Task &task, std::function<void(bool success, const QJsonObject &taskObj, const QString &error)> callback);
    void updateTask(const QString &remoteListId, const Task &task, std::function<void(bool success, const QString &error)> callback);
    void deleteTask(const QString &remoteListId, const QString &remoteTaskId, std::function<void(bool success, const QString &error)> callback);

    // Subtareas (Checklist Items)
    void fetchSteps(const QString &remoteListId, const QString &remoteTaskId, std::function<void(bool success, const QJsonArray &steps, const QString &error)> callback);
    void createStep(const QString &remoteListId, const QString &remoteTaskId, const TaskStep &step, std::function<void(bool success, const QJsonObject &stepObj, const QString &error)> callback);
    void updateStep(const QString &remoteListId, const QString &remoteTaskId, const QString &remoteStepId, const TaskStep &step, std::function<void(bool success, const QString &error)> callback);
    void deleteStep(const QString &remoteListId, const QString &remoteTaskId, const QString &remoteStepId, std::function<void(bool success, const QString &error)> callback);

    static QJsonObject taskToJson(const Task &task);

signals:
    void networkError(const QString &error);

private:
    void sendAuthorizedRequest(const QString &method, const QString &endpoint, const QByteArray &data,
                               std::function<void(bool success, int statusCode, const QByteArray &response, const QString &error)> callback);

    AuthManager            *m_auth = nullptr;
    QNetworkAccessManager  *m_nam = nullptr;

    static constexpr const char *kBaseUrl = "https://graph.microsoft.com/v1.0/me/todo";
};
