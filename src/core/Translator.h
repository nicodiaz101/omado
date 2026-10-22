#pragma once

#include <QTranslator>
#include <QFileSystemWatcher>
#include <QHash>
#include <QString>

class QQmlEngine;

class Translator : public QTranslator {
    Q_OBJECT

public:
    explicit Translator(QQmlEngine *engine, QObject *parent = nullptr);
    ~Translator() override;

    QString translate(const char *context, const char *sourceText,
                      const char *disambiguation = nullptr, int n = -1) const override;

    bool isEmpty() const override;

    QString currentLanguage() const { return m_currentLanguage; }
    void loadLanguage(const QString &langCode);
    void setCustomLanguageFilePath(const QString &path);

signals:
    void languageChanged(const QString &langCode);

private slots:
    void onConfigFileChanged(const QString &path);

private:
    void loadTranslationsFromJson(const QString &langCode);
    QString detectSystemLanguage() const;

    QQmlEngine *m_engine = nullptr;
    QFileSystemWatcher m_watcher;
    QString m_configPath;
    QString m_currentLanguage;
    QHash<QString, QString> m_translations;
};
