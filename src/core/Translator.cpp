#include "Translator.h"
#include <QQmlEngine>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>

Translator::Translator(QQmlEngine *engine, QObject *parent)
    : QTranslator(parent)
    , m_engine(engine)
{
    QString configDir = QDir::homePath() + QStringLiteral("/.config/omarchy");
    m_configPath = configDir + QStringLiteral("/language");

    // Observar directorio o archivo de configuración
    if (QFile::exists(m_configPath)) {
        m_watcher.addPath(m_configPath);
    } else {
        if (QDir(configDir).exists()) {
            m_watcher.addPath(configDir);
        }
    }

    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &Translator::onConfigFileChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &Translator::onConfigFileChanged);

    // Cargar idioma inicial
    QString initialLang = detectSystemLanguage();
    loadLanguage(initialLang);
}

Translator::~Translator() = default;

void Translator::setCustomLanguageFilePath(const QString &path) {
    if (!m_watcher.files().isEmpty()) {
        m_watcher.removePaths(m_watcher.files());
    }
    if (!m_watcher.directories().isEmpty()) {
        m_watcher.removePaths(m_watcher.directories());
    }

    m_configPath = path;
    if (QFile::exists(m_configPath)) {
        m_watcher.addPath(m_configPath);
    } else {
        QFileInfo fi(path);
        if (fi.dir().exists()) {
            m_watcher.addPath(fi.dir().absolutePath());
        }
    }

    QString lang = detectSystemLanguage();
    loadLanguage(lang);
}

QString Translator::detectSystemLanguage() const {
    if (QFile::exists(m_configPath)) {
        QFile file(m_configPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(file.readAll()).trimmed();
            if (!content.isEmpty()) {
                return content;
            }
        }
    }

    // Fallback: locale del sistema
    QString sysLocale = QLocale::system().name(); // ej: "es_AR", "en_US"
    if (sysLocale.startsWith(QLatin1String("es"), Qt::CaseInsensitive)) {
        return QStringLiteral("es");
    }
    return QStringLiteral("en");
}

void Translator::onConfigFileChanged(const QString &path) {
    Q_UNUSED(path);
    // Si el archivo fue recreado, asegurarnos de volver a vigilarlo
    if (QFile::exists(m_configPath) && !m_watcher.files().contains(m_configPath)) {
        m_watcher.addPath(m_configPath);
    }

    QString newLang = detectSystemLanguage();
    if (newLang != m_currentLanguage) {
        loadLanguage(newLang);
    }
}

void Translator::loadLanguage(const QString &langCode) {
    m_currentLanguage = langCode;
    m_translations.clear();

    loadTranslationsFromJson(langCode);

    if (m_engine) {
        m_engine->retranslate();
    }
    emit languageChanged(langCode);
}

void Translator::loadTranslationsFromJson(const QString &langCode) {
    // Intentar código exacto (ej: "es-AR" o "es"), luego fallback a idioma base (ej: "es"), luego "en"
    QStringList candidates;
    candidates.append(langCode);
    if (langCode.contains(QLatin1Char('-'))) {
        candidates.append(langCode.split(QLatin1Char('-')).first());
    } else if (langCode.contains(QLatin1Char('_'))) {
        candidates.append(langCode.split(QLatin1Char('_')).first());
    }
    if (!candidates.contains(QStringLiteral("en"))) {
        candidates.append(QStringLiteral("en"));
    }

    for (const QString &code : candidates) {
        QStringList pathsToTry = {
            QStringLiteral(":/qt/qml/OmaDo/i18n/src/i18n/") + code + QStringLiteral(".json"),
            QStringLiteral(":/qt/qml/OmaDo/i18n/") + code + QStringLiteral(".json"),
            QStringLiteral(":/OmaDo/src/i18n/") + code + QStringLiteral(".json"),
            QStringLiteral(":/OmaDo/i18n/") + code + QStringLiteral(".json")
        };
        for (const QString &resPath : pathsToTry) {
            QFile file(resPath);
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    for (auto it = obj.begin(); it != obj.end(); ++it) {
                        if (!m_translations.contains(it.key())) {
                            m_translations.insert(it.key(), it.value().toString());
                        }
                    }
                    break;
                }
            }
        }
    }
}

QString Translator::translate(const char *context, const char *sourceText,
                             const char *disambiguation, int n) const {
    Q_UNUSED(context);
    Q_UNUSED(disambiguation);
    Q_UNUSED(n);

    QString key = QString::fromUtf8(sourceText);
    if (m_translations.contains(key)) {
        return m_translations.value(key);
    }

    return QString();
}

bool Translator::isEmpty() const {
    return m_translations.isEmpty();
}
