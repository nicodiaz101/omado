#include "GraphClient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QTimer>
#include <QDebug>
#include <memory>

GraphClient::GraphClient(AuthManager *auth, QObject *parent)
    : QObject(parent), m_auth(auth), m_nam(nullptr) {}

QNetworkAccessManager *GraphClient::networkAccessManager() {
    if (m_auth) {
        return m_auth->networkAccessManager();
    }
    if (!m_nam) {
        m_nam = new QNetworkAccessManager(this);
    }
    return m_nam;
}

void GraphClient::sendAuthorizedRequest(const QString &method, const QString &endpoint, const QByteArray &data,
                                        std::function<void(bool success, int statusCode, const QByteArray &response, const QString &error)> callback,
                                        int retryCount) {
    if (!m_auth) {
        if (callback) callback(false, 0, QByteArray(), QStringLiteral("AuthManager no disponible"));
        return;
    }

    m_auth->ensureValidToken([this, method, endpoint, data, callback, retryCount](const QString &accessToken, bool okToken) {
        if (!okToken || accessToken.isEmpty()) {
            if (callback) callback(false, 401, QByteArray(), QStringLiteral("No autenticado"));
            emit networkError(QStringLiteral("No autenticado en Microsoft"));
            return;
        }

        QUrl url;
        if (endpoint.startsWith(QLatin1String("https://"))) {
            url = QUrl(endpoint);
            if (url.scheme() != QLatin1String("https") || url.host().toLower() != QLatin1String("graph.microsoft.com") || !url.path().startsWith(QLatin1String("/v1.0/"))) {
                qWarning() << "[GraphClient] Rechazando URL no confiable para Bearer Token:" << endpoint;
                if (callback) callback(false, 0, QByteArray(), QStringLiteral("URL no confiable rechazada por seguridad"));
                return;
            }
        } else {
            url = QUrl(QString::fromLatin1(kBaseUrl) + endpoint);
        }

        QNetworkRequest req(url);
        req.setTransferTimeout(15000); // 15 segundos timeout (OMA-A05)
        req.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Prefer", "outlook.timezone=\"UTC\"");

        QNetworkAccessManager *nam = networkAccessManager();
        QNetworkReply *reply = nullptr;
        if (method == QLatin1String("GET")) {
            reply = nam->get(req);
        } else if (method == QLatin1String("POST")) {
            reply = nam->post(req, data);
        } else if (method == QLatin1String("PATCH")) {
            reply = nam->sendCustomRequest(req, "PATCH", data);
        } else if (method == QLatin1String("DELETE")) {
            reply = nam->deleteResource(req);
        } else {
            if (callback) callback(false, 0, QByteArray(), QStringLiteral("Método no soportado"));
            return;
        }

        connect(reply, &QNetworkReply::finished, this, [this, reply, method, endpoint, data, callback, retryCount]() {
            reply->deleteLater();
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray respData = reply->readAll();

            // Handle 401 Unauthorized retry once with refreshed token
            if (statusCode == 401 && retryCount < 1) {
                qDebug() << "[GraphClient] Token expirado (401), forzando refresco...";
                m_auth->refreshAccessToken([this, method, endpoint, data, callback, retryCount](bool refreshed) {
                    if (refreshed) {
                        sendAuthorizedRequest(method, endpoint, data, callback, retryCount + 1);
                    } else {
                        if (callback) callback(false, 401, QByteArray(), QStringLiteral("Error refrescando token tras 401"));
                    }
                });
                return;
            }

            // Handle 429 Too Many Requests with Retry-After
            if (statusCode == 429 && retryCount < 2) {
                int retryAfterSec = 5;
                if (reply->hasRawHeader("Retry-After")) {
                    bool ok = false;
                    int hSec = reply->rawHeader("Retry-After").toInt(&ok);
                    if (ok && hSec > 0 && hSec <= 60) {
                        retryAfterSec = hSec;
                    }
                }
                qWarning() << "[GraphClient] Rate limit (429), reintentando en" << retryAfterSec << "segundos...";
                QTimer::singleShot(retryAfterSec * 1000, this, [this, method, endpoint, data, callback, retryCount]() {
                    sendAuthorizedRequest(method, endpoint, data, callback, retryCount + 1);
                });
                return;
            }

            // OMA-C01: Corregir condición de error a || (no &&) para atrapar fallos de red offline donde statusCode es 0
            if (reply->error() != QNetworkReply::NoError || statusCode < 200 || statusCode >= 300) {
                QString errStr = reply->errorString();
                if (errStr.isEmpty() || errStr == QLatin1String("Unknown error")) {
                    errStr = QStringLiteral("HTTP error %1").arg(statusCode);
                }
                qWarning() << "[GraphClient] Error HTTP" << statusCode << ":" << errStr;
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
    obj["status"] = task.isCompleted ? QStringLiteral("completed") : QStringLiteral("notStarted");
    obj["importance"] = task.importance.isEmpty() ? QStringLiteral("normal") : task.importance;

    QJsonObject bodyObj;
    bodyObj["content"] = task.body.isNull() ? QStringLiteral("") : task.body;
    bodyObj["contentType"] = QStringLiteral("text");
    obj["body"] = bodyObj;

    if (task.dueDate.isValid()) {
        QJsonObject dueObj;
        dueObj["dateTime"] = task.dueDate.toString(Qt::ISODate) + QStringLiteral("T00:00:00.0000000");
        dueObj["timeZone"] = QStringLiteral("UTC");
        obj["dueDateTime"] = dueObj;
    } else if (!task.remoteId.isEmpty()) {
        // Enviar null explícito para limpiar fecha remota
        obj["dueDateTime"] = QJsonValue::Null;
    }

    if (task.reminderAt.isValid()) {
        obj["isReminderOn"] = true;
        QJsonObject reminderObj;
        reminderObj["dateTime"] = task.reminderAt.toUTC().toString("yyyy-MM-ddTHH:mm:ss.0000000");
        reminderObj["timeZone"] = QStringLiteral("UTC");
        obj["reminderDateTime"] = reminderObj;
    } else {
        obj["isReminderOn"] = false;
        if (!task.remoteId.isEmpty()) {
            obj["reminderDateTime"] = QJsonValue::Null;
        }
    }

    if (!task.recurrence.isEmpty() && task.recurrence != "none") {
        QJsonObject recObj;
        QJsonObject pattern;
        QJsonObject range;

        if (task.recurrence == "daily") {
            pattern["type"] = QStringLiteral("daily");
            pattern["interval"] = 1;
        } else if (task.recurrence == "weekly") {
            pattern["type"] = QStringLiteral("weekly");
            pattern["interval"] = 1;
        } else if (task.recurrence == "workdays") {
            pattern["type"] = QStringLiteral("weekly");
            pattern["interval"] = 1;
            QJsonArray days;
            days.append(QStringLiteral("monday"));
            days.append(QStringLiteral("tuesday"));
            days.append(QStringLiteral("wednesday"));
            days.append(QStringLiteral("thursday"));
            days.append(QStringLiteral("friday"));
            pattern["daysOfWeek"] = days;
        } else if (task.recurrence == "monthly") {
            pattern["type"] = QStringLiteral("absoluteMonthly");
            pattern["interval"] = 1;
            pattern["dayOfMonth"] = task.dueDate.isValid() ? task.dueDate.day() : 1;
        }

        range["type"] = QStringLiteral("noEnd");
        range["startDate"] = task.dueDate.isValid() ? task.dueDate.toString(Qt::ISODate) : QDate::currentDate().toString(Qt::ISODate);
        range["recurrenceTimeZone"] = QStringLiteral("UTC");

        recObj["pattern"] = pattern;
        recObj["range"] = range;
        obj["recurrence"] = recObj;
    }

    return obj;
}

void GraphClient::fetchLists(std::function<void(bool success, const QJsonArray &lists, const QString &error)> callback) {
    sendAuthorizedRequest("GET", QStringLiteral("/lists?$top=100"), QByteArray(), [callback](bool ok, int, const QByteArray &data, const QString &err) {
        if (!ok) {
            if (callback) callback(false, QJsonArray(), err);
            return;
        }
        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            if (callback) callback(false, QJsonArray(), QStringLiteral("Error parseando listas JSON"));
            return;
        }
        QJsonObject root = doc.object();
        if (callback) callback(true, root.value("value").toArray(), QString());
    });
}

void GraphClient::createList(const QString &displayName, std::function<void(bool success, const QJsonObject &listObj, const QString &error)> callback) {
    QJsonObject body;
    body["displayName"] = displayName;
    QByteArray jsonData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    sendAuthorizedRequest("POST", QStringLiteral("/lists"), jsonData, [callback](bool ok, int, const QByteArray &data, const QString &err) {
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

    QString encodedListId = QString::fromUtf8(QUrl::toPercentEncoding(remoteListId));
    sendAuthorizedRequest("PATCH", QStringLiteral("/lists/%1").arg(encodedListId), jsonData, [callback](bool ok, int, const QByteArray &, const QString &err) {
        if (callback) callback(ok, err);
    });
}

void GraphClient::deleteList(const QString &remoteListId, std::function<void(bool success, const QString &error)> callback) {
    QString encodedListId = QString::fromUtf8(QUrl::toPercentEncoding(remoteListId));
    sendAuthorizedRequest("DELETE", QStringLiteral("/lists/%1").arg(encodedListId), QByteArray(), [callback](bool ok, int, const QByteArray &, const QString &err) {
        if (callback) callback(ok, err);
    });
}

void GraphClient::fetchTasks(const QString &remoteListId, std::function<void(bool success, const QJsonArray &tasks, const QString &error)> callback) {
    QString encodedListId = QString::fromUtf8(QUrl::toPercentEncoding(remoteListId));
    QString initialEndpoint = QStringLiteral("/lists/%1/tasks?$top=1000&$expand=checklistItems").arg(encodedListId);

    auto allTasks = std::make_shared<QJsonArray>();
    auto fetchPage = std::make_shared<std::function<void(const QString &)>>();
    std::weak_ptr<std::function<void(const QString &)>> weakFetchPage = fetchPage;

    *fetchPage = [this, allTasks, callback, weakFetchPage](const QString &endpointOrUrl) {
        sendAuthorizedRequest("GET", endpointOrUrl, QByteArray(), [allTasks, callback, weakFetchPage](bool ok, int, const QByteArray &data, const QString &err) {
            if (!ok) {
                if (callback) callback(false, *allTasks, err);
                return;
            }
            QJsonParseError parseErr;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
            if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
                if (callback) callback(false, *allTasks, QStringLiteral("Error parseando tareas JSON"));
                return;
            }
            QJsonObject root = doc.object();
            QJsonArray pageItems = root.value("value").toArray();
            for (const auto &item : pageItems) {
                allTasks->append(item);
            }

            QString nextLink = root.value("@odata.nextLink").toString();
            if (!nextLink.isEmpty()) {
                if (auto fp = weakFetchPage.lock()) {
                    (*fp)(nextLink);
                }
            } else {
                if (callback) callback(true, *allTasks, QString());
            }
        });
    };

    (*fetchPage)(initialEndpoint);
}

void GraphClient::createTask(const QString &remoteListId, const Task &task, std::function<void(bool success, const QJsonObject &taskObj, const QString &error)> callback) {
    QJsonObject body = taskToJson(task);
    QByteArray jsonData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QString encodedListId = QString::fromUtf8(QUrl::toPercentEncoding(remoteListId));
    sendAuthorizedRequest("POST", QStringLiteral("/lists/%1/tasks").arg(encodedListId), jsonData, [callback](bool ok, int, const QByteArray &data, const QString &err) {
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
        if (callback) callback(false, QStringLiteral("Tarea sin remoteId"));
        return;
    }
    QJsonObject body = taskToJson(task);
    QByteArray jsonData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QString encodedListId = QString::fromUtf8(QUrl::toPercentEncoding(remoteListId));
    QString encodedTaskId = QString::fromUtf8(QUrl::toPercentEncoding(task.remoteId));
    sendAuthorizedRequest("PATCH", QStringLiteral("/lists/%1/tasks/%2").arg(encodedListId, encodedTaskId), jsonData, [callback](bool ok, int, const QByteArray &, const QString &err) {
        if (callback) callback(ok, err);
    });
}

void GraphClient::deleteTask(const QString &remoteListId, const QString &remoteTaskId, std::function<void(bool success, int statusCode, const QString &error)> callback) {
    QString encodedListId = QString::fromUtf8(QUrl::toPercentEncoding(remoteListId));
    QString encodedTaskId = QString::fromUtf8(QUrl::toPercentEncoding(remoteTaskId));
    sendAuthorizedRequest("DELETE", QStringLiteral("/lists/%1/tasks/%2").arg(encodedListId, encodedTaskId), QByteArray(), [callback](bool ok, int statusCode, const QByteArray &, const QString &err) {
        if (callback) callback(ok, statusCode, err);
    });
}

void GraphClient::fetchSteps(const QString &remoteListId, const QString &remoteTaskId, std::function<void(bool success, const QJsonArray &steps, const QString &error)> callback) {
    QString encodedListId = QString::fromUtf8(QUrl::toPercentEncoding(remoteListId));
    QString encodedTaskId = QString::fromUtf8(QUrl::toPercentEncoding(remoteTaskId));
    sendAuthorizedRequest("GET", QStringLiteral("/lists/%1/tasks/%2/checklistItems").arg(encodedListId, encodedTaskId), QByteArray(), [callback](bool ok, int, const QByteArray &data, const QString &err) {
        if (!ok) {
            if (callback) callback(false, QJsonArray(), err);
            return;
        }
        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            if (callback) callback(false, QJsonArray(), QStringLiteral("Error parseando subtareas JSON"));
            return;
        }
        QJsonObject root = doc.object();
        if (callback) callback(true, root.value("value").toArray(), QString());
    });
}

void GraphClient::createStep(const QString &remoteListId, const QString &remoteTaskId, const TaskStep &step, std::function<void(bool success, const QJsonObject &stepObj, const QString &error)> callback) {
    QJsonObject body;
    body["displayName"] = step.title;
    body["isChecked"] = step.isCompleted;
    QByteArray jsonData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QString encodedListId = QString::fromUtf8(QUrl::toPercentEncoding(remoteListId));
    QString encodedTaskId = QString::fromUtf8(QUrl::toPercentEncoding(remoteTaskId));
    sendAuthorizedRequest("POST", QStringLiteral("/lists/%1/tasks/%2/checklistItems").arg(encodedListId, encodedTaskId), jsonData, [callback](bool ok, int, const QByteArray &data, const QString &err) {
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

    QString encodedListId = QString::fromUtf8(QUrl::toPercentEncoding(remoteListId));
    QString encodedTaskId = QString::fromUtf8(QUrl::toPercentEncoding(remoteTaskId));
    QString encodedStepId = QString::fromUtf8(QUrl::toPercentEncoding(remoteStepId));
    sendAuthorizedRequest("PATCH", QStringLiteral("/lists/%1/tasks/%2/checklistItems/%3").arg(encodedListId, encodedTaskId, encodedStepId), jsonData, [callback](bool ok, int, const QByteArray &, const QString &err) {
        if (callback) callback(ok, err);
    });
}

void GraphClient::deleteStep(const QString &remoteListId, const QString &remoteTaskId, const QString &remoteStepId, std::function<void(bool success, int statusCode, const QString &error)> callback) {
    QString encodedListId = QString::fromUtf8(QUrl::toPercentEncoding(remoteListId));
    QString encodedTaskId = QString::fromUtf8(QUrl::toPercentEncoding(remoteTaskId));
    QString encodedStepId = QString::fromUtf8(QUrl::toPercentEncoding(remoteStepId));
    sendAuthorizedRequest("DELETE", QStringLiteral("/lists/%1/tasks/%2/checklistItems/%3").arg(encodedListId, encodedTaskId, encodedStepId), QByteArray(), [callback](bool ok, int statusCode, const QByteArray &, const QString &err) {
        if (callback) callback(ok, statusCode, err);
    });
}
