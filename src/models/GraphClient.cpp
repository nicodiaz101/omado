#include "GraphClient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

GraphClient::GraphClient(AuthManager *auth, QObject *parent)
    : QObject(parent), m_auth(auth), m_nam(new QNetworkAccessManager(this)) {}

void GraphClient::sendAuthorizedRequest(const QString &method, const QString &endpoint, const QByteArray &data,
                                        std::function<void(bool success, int statusCode, const QByteArray &response, const QString &error)> callback) {
    if (!m_auth) {
        if (callback) callback(false, 0, QByteArray(), "AuthManager no disponible");
        return;
    }

    m_auth->ensureValidToken([this, method, endpoint, data, callback](const QString &accessToken, bool okToken) {
        if (!okToken || accessToken.isEmpty()) {
            if (callback) callback(false, 401, QByteArray(), "No autenticado");
            emit networkError("No autenticado en Microsoft");
            return;
        }

        QUrl url(QString::fromLatin1(kBaseUrl) + endpoint);
        QNetworkRequest req(url);
        req.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = nullptr;
        if (method == "GET") {
            reply = m_nam->get(req);
        } else if (method == "POST") {
            reply = m_nam->post(req, data);
        } else if (method == "PATCH") {
            reply = m_nam->sendCustomRequest(req, "PATCH", data);
        } else if (method == "DELETE") {
            reply = m_nam->deleteResource(req);
        } else {
            if (callback) callback(false, 0, QByteArray(), "Método no soportado");
            return;
        }

        connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
            reply->deleteLater();
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray respData = reply->readAll();

            if (reply->error() != QNetworkReply::NoError && statusCode >= 400) {
                QString errStr = reply->errorString();
                qWarning() << "[GraphClient] Error HTTP" << statusCode << ":" << errStr << respData;
                emit networkError(errStr);
                if (callback) callback(false, statusCode, respData, errStr);
            } else {
                if (callback) callback(true, statusCode, respData, QString());
            }
        });
    });
}

QJsonObject GraphClient::taskToJson(const Task &task) {
    QJsonObject obj;
    obj["title"] = task.title;
    obj["status"] = task.isCompleted ? "completed" : "notStarted";
    obj["importance"] = task.importance.isEmpty() ? "normal" : task.importance;

    if (!task.body.isEmpty()) {
        QJsonObject bodyObj;
        bodyObj["content"] = task.body;
        bodyObj["contentType"] = "text";
        obj["body"] = bodyObj;
    }

    if (task.dueDate.isValid()) {
        QJsonObject dueObj;
        dueObj["dateTime"] = task.dueDate.toString(Qt::ISODate) + "T00:00:00.0000000";
        dueObj["timeZone"] = "UTC";
        obj["dueDateTime"] = dueObj;
    }

    if (task.reminderAt.isValid()) {
        obj["isReminderOn"] = true;
        QJsonObject reminderObj;
        reminderObj["dateTime"] = task.reminderAt.toUTC().toString("yyyy-MM-ddTHH:mm:ss.0000000");
        reminderObj["timeZone"] = "UTC";
        obj["reminderDateTime"] = reminderObj;
    } else {
        obj["isReminderOn"] = false;
    }

    return obj;
}

void GraphClient::fetchLists(std::function<void(bool success, const QJsonArray &lists, const QString &error)> callback) {
    sendAuthorizedRequest("GET", "/lists", QByteArray(), [callback](bool ok, int, const QByteArray &data, const QString &err) {
        if (!ok) {
            if (callback) callback(false, QJsonArray(), err);
            return;
        }
        QJsonObject root = QJsonDocument::fromJson(data).object();
        if (callback) callback(true, root.value("value").toArray(), QString());
    });
}

void GraphClient::createList(const QString &displayName, std::function<void(bool success, const QJsonObject &listObj, const QString &error)> callback) {
    QJsonObject body;
    body["displayName"] = displayName;
    QByteArray jsonData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    sendAuthorizedRequest("POST", "/lists", jsonData, [callback](bool ok, int, const QByteArray &data, const QString &err) {
        if (!ok) {
            if (callback) callback(false, QJsonObject(), err);
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(data).object();
        if (callback) callback(true, obj, QString());
    });
}

void GraphClient::updateList(const QString &remoteListId, const QString &displayName, std::function<void(bool success, const QString &error)> callback) {
    QJsonObject body;
    body["displayName"] = displayName;
    QByteArray jsonData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    sendAuthorizedRequest("PATCH", QStringLiteral("/lists/%1").arg(remoteListId), jsonData, [callback](bool ok, int, const QByteArray &, const QString &err) {
        if (callback) callback(ok, err);
    });
}

void GraphClient::deleteList(const QString &remoteListId, std::function<void(bool success, const QString &error)> callback) {
    sendAuthorizedRequest("DELETE", QStringLiteral("/lists/%1").arg(remoteListId), QByteArray(), [callback](bool ok, int, const QByteArray &, const QString &err) {
        if (callback) callback(ok, err);
    });
}

void GraphClient::fetchTasks(const QString &remoteListId, std::function<void(bool success, const QJsonArray &tasks, const QString &error)> callback) {
    sendAuthorizedRequest("GET", QStringLiteral("/lists/%1/tasks").arg(remoteListId), QByteArray(), [callback](bool ok, int, const QByteArray &data, const QString &err) {
        if (!ok) {
            if (callback) callback(false, QJsonArray(), err);
            return;
        }
        QJsonObject root = QJsonDocument::fromJson(data).object();
        if (callback) callback(true, root.value("value").toArray(), QString());
    });
}

void GraphClient::createTask(const QString &remoteListId, const Task &task, std::function<void(bool success, const QJsonObject &taskObj, const QString &error)> callback) {
    QJsonObject body = taskToJson(task);
    QByteArray jsonData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    sendAuthorizedRequest("POST", QStringLiteral("/lists/%1/tasks").arg(remoteListId), jsonData, [callback](bool ok, int, const QByteArray &data, const QString &err) {
        if (!ok) {
            if (callback) callback(false, QJsonObject(), err);
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(data).object();
        if (callback) callback(true, obj, QString());
    });
}

void GraphClient::updateTask(const QString &remoteListId, const Task &task, std::function<void(bool success, const QString &error)> callback) {
    if (task.remoteId.isEmpty()) {
        if (callback) callback(false, "Tarea sin remoteId");
        return;
    }
    QJsonObject body = taskToJson(task);
    QByteArray jsonData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    sendAuthorizedRequest("PATCH", QStringLiteral("/lists/%1/tasks/%2").arg(remoteListId, task.remoteId), jsonData, [callback](bool ok, int, const QByteArray &, const QString &err) {
        if (callback) callback(ok, err);
    });
}

void GraphClient::deleteTask(const QString &remoteListId, const QString &remoteTaskId, std::function<void(bool success, const QString &error)> callback) {
    sendAuthorizedRequest("DELETE", QStringLiteral("/lists/%1/tasks/%2").arg(remoteListId, remoteTaskId), QByteArray(), [callback](bool ok, int, const QByteArray &, const QString &err) {
        if (callback) callback(ok, err);
    });
}

void GraphClient::fetchSteps(const QString &remoteListId, const QString &remoteTaskId, std::function<void(bool success, const QJsonArray &steps, const QString &error)> callback) {
    sendAuthorizedRequest("GET", QStringLiteral("/lists/%1/tasks/%2/checklistItems").arg(remoteListId, remoteTaskId), QByteArray(), [callback](bool ok, int, const QByteArray &data, const QString &err) {
        if (!ok) {
            if (callback) callback(false, QJsonArray(), err);
            return;
        }
        QJsonObject root = QJsonDocument::fromJson(data).object();
        if (callback) callback(true, root.value("value").toArray(), QString());
    });
}

void GraphClient::createStep(const QString &remoteListId, const QString &remoteTaskId, const TaskStep &step, std::function<void(bool success, const QJsonObject &stepObj, const QString &error)> callback) {
    QJsonObject body;
    body["displayName"] = step.title;
    body["isChecked"] = step.isCompleted;
    QByteArray jsonData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    sendAuthorizedRequest("POST", QStringLiteral("/lists/%1/tasks/%2/checklistItems").arg(remoteListId, remoteTaskId), jsonData, [callback](bool ok, int, const QByteArray &data, const QString &err) {
        if (!ok) {
            if (callback) callback(false, QJsonObject(), err);
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(data).object();
        if (callback) callback(true, obj, QString());
    });
}

void GraphClient::updateStep(const QString &remoteListId, const QString &remoteTaskId, const QString &remoteStepId, const TaskStep &step, std::function<void(bool success, const QString &error)> callback) {
    QJsonObject body;
    body["displayName"] = step.title;
    body["isChecked"] = step.isCompleted;
    QByteArray jsonData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    sendAuthorizedRequest("PATCH", QStringLiteral("/lists/%1/tasks/%2/checklistItems/%3").arg(remoteListId, remoteTaskId, remoteStepId), jsonData, [callback](bool ok, int, const QByteArray &, const QString &err) {
        if (callback) callback(ok, err);
    });
}

void GraphClient::deleteStep(const QString &remoteListId, const QString &remoteTaskId, const QString &remoteStepId, std::function<void(bool success, const QString &error)> callback) {
    sendAuthorizedRequest("DELETE", QStringLiteral("/lists/%1/tasks/%2/checklistItems/%3").arg(remoteListId, remoteTaskId, remoteStepId), QByteArray(), [callback](bool ok, int, const QByteArray &, const QString &err) {
        if (callback) callback(ok, err);
    });
}
