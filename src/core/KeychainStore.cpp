#include "KeychainStore.h"
#include <qt6keychain/keychain.h>
#include <QDebug>

KeychainStore::KeychainStore(QObject *parent) : QObject(parent) {}

void KeychainStore::writeKey(const QString &key, const QString &value, std::function<void(bool success, const QString &error)> callback) {
    auto *job = new QKeychain::WritePasswordJob(QLatin1String(kServiceName), this);
    job->setAutoDelete(true);
    job->setKey(key);
    job->setTextData(value);
    
    connect(job, &QKeychain::Job::finished, this, [job, callback]() {
        if (job->error()) {
            qWarning() << "[KeychainStore] Error writing key:" << job->key() << job->errorString();
            if (callback) callback(false, job->errorString());
        } else {
            if (callback) callback(true, QString());
        }
    });
    job->start();
}

void KeychainStore::readKey(const QString &key, std::function<void(const QString &value, bool success)> callback) {
    auto *job = new QKeychain::ReadPasswordJob(QLatin1String(kServiceName), this);
    job->setAutoDelete(true);
    job->setKey(key);
    
    connect(job, &QKeychain::Job::finished, this, [job, callback]() {
        if (job->error()) {
            if (job->error() != QKeychain::EntryNotFound) {
                qWarning() << "[KeychainStore] Error reading key:" << job->key() << job->errorString();
            }
            if (callback) callback(QString(), false);
        } else {
            if (callback) callback(job->textData(), true);
        }
    });
    job->start();
}

void KeychainStore::deleteKey(const QString &key, std::function<void(bool success)> callback) {
    auto *job = new QKeychain::DeletePasswordJob(QLatin1String(kServiceName), this);
    job->setAutoDelete(true);
    job->setKey(key);
    
    connect(job, &QKeychain::Job::finished, this, [job, callback]() {
        if (job->error() && job->error() != QKeychain::EntryNotFound) {
            qWarning() << "[KeychainStore] Error deleting key:" << job->key() << job->errorString();
            if (callback) callback(false);
        } else {
            if (callback) callback(true);
        }
    });
    job->start();
}

void KeychainStore::clearAll(std::function<void()> callback) {
    deleteKey(QLatin1String(kKeyAccessToken), [this, callback](bool) {
        deleteKey(QLatin1String(kKeyRefreshToken), [this, callback](bool) {
            deleteKey(QLatin1String(kKeyTokenExpiry), [this, callback](bool) {
                deleteKey(QLatin1String(kKeyUserEmail), [this, callback](bool) {
                    deleteKey(QLatin1String(kKeyUserName), [callback](bool) {
                        if (callback) callback();
                    });
                });
            });
        });
    });
}
