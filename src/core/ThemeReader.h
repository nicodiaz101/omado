#pragma once

#include <QObject>
#include <QColor>
#include <QFileSystemWatcher>

class ThemeReader : public QObject {
    Q_OBJECT
    Q_PROPERTY(QColor background READ background NOTIFY themeChanged)
    Q_PROPERTY(QColor foreground READ foreground NOTIFY themeChanged)
    Q_PROPERTY(QColor accent READ accent NOTIFY themeChanged)
    Q_PROPERTY(QColor surface READ surface NOTIFY themeChanged)
    Q_PROPERTY(QColor border READ border NOTIFY themeChanged)
    Q_PROPERTY(QColor error READ error NOTIFY themeChanged)

public:
    explicit ThemeReader(QObject *parent = nullptr);

    QColor background() const { return m_background; }
    QColor foreground() const { return m_foreground; }
    QColor accent() const { return m_accent; }
    QColor surface() const { return m_surface; }
    QColor border() const { return m_border; }
    QColor error() const { return m_error; }

signals:
    void themeChanged();

private:
    void loadTheme();
    void parseTomlLine(const QString &line);
    
    QColor m_background{"#1a1a2e"};
    QColor m_foreground{"#e0e0e0"};
    QColor m_accent{"#7aa2f7"};
    QColor m_surface{"#16213e"};
    QColor m_border{"#2a2a4a"};
    QColor m_error{"#f7768e"};
    
    QFileSystemWatcher m_watcher;
    QString m_themePath;
};
