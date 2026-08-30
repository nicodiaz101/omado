#include "ThemeReader.h"
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>

ThemeReader::ThemeReader(QObject *parent) : QObject(parent) {
    QString themeDir = QDir::homePath() + "/.local/state/omarchy/current/theme";
    m_themePath = themeDir + "/colors.toml";
    
    loadTheme();
    
    // Watch tanto el directorio (para detectar atomic renames) como el archivo
    if (QDir(themeDir).exists()) m_watcher.addPath(themeDir);
    if (QFile::exists(m_themePath)) m_watcher.addPath(m_themePath);
    
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &) {
        QTimer::singleShot(50, this, &ThemeReader::loadTheme);
    });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) {
        QTimer::singleShot(50, this, &ThemeReader::loadTheme);
    });
}

ThemeReader::ThemeReader(const QString &themePath, QObject *parent)
    : QObject(parent)
    , m_themePath(themePath)
{
    loadTheme();

    if (QFile::exists(m_themePath)) m_watcher.addPath(m_themePath);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &) {
        QTimer::singleShot(50, this, &ThemeReader::loadTheme);
    });
}

bool ThemeReader::loadFromFile(const QString &filePath) {
    m_themePath = filePath;
    loadTheme();
    return QFile::exists(filePath);
}

void ThemeReader::loadTheme() {
    QFile file(m_themePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[ThemeReader] No se pudo abrir" << m_themePath;
        return;
    }
    
    // Re-watch en caso de atomic rename
    if (!m_watcher.files().contains(m_themePath)) {
        m_watcher.addPath(m_themePath);
    }
    
    QTextStream in(&file);
    bool changed = false;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#") || line.startsWith("[")) {
            continue;
        }
        
        QRegularExpression re(R"(^(\w+)\s*=\s*['"]?(#[0-9a-fA-F]{6,8})['"]?)");
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            QString key = match.captured(1);
            QColor color(match.captured(2));
            
            if (color.isValid()) {
                if (key == "background" && m_background != color) { m_background = color; changed = true; }
                else if (key == "foreground" && m_foreground != color) { m_foreground = color; changed = true; }
                else if (key == "accent" && m_accent != color) { m_accent = color; changed = true; }
                else if ((key == "surface" || key == "lighter_background") && m_surface != color) { m_surface = color; changed = true; }
                else if ((key == "border" || key == "muted") && m_border != color) { m_border = color; changed = true; }
                else if ((key == "error" || key == "red") && m_error != color) { m_error = color; changed = true; }
            }
        }
    }
    
    if (changed) {
        emit themeChanged();
    }
}
