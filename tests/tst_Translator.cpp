#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include "core/Translator.h"

class tst_Translator : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;
    QString m_langFilePath;

private slots:
    void initTestCase() {
        QVERIFY(m_tempDir.isValid());
        m_langFilePath = m_tempDir.path() + "/language";
    }

    void testTranslationAndFallback() {
        Translator translator(nullptr, this);
        translator.setCustomLanguageFilePath(m_langFilePath);

        // Sin archivo, fallback en inglés
        QString myDayEn = translator.translate("", "My Day");
        QCOMPARE(myDayEn, QStringLiteral("My Day"));

        // Escribir 'es' en el archivo
        {
            QFile file(m_langFilePath);
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
            file.write("es\n");
            file.close();
        }

        // Simular cambio o recargar
        translator.setCustomLanguageFilePath(m_langFilePath);
        QCOMPARE(translator.currentLanguage(), QStringLiteral("es"));
        QString myDayEs = translator.translate("", "My Day");
        QCOMPARE(myDayEs, QStringLiteral("Mi día"));
        QCOMPARE(translator.translate("", "Add a task..."), QStringLiteral("Agregar una tarea..."));
    }

    void testLiveFileWatcher() {
        Translator translator(nullptr, this);
        translator.setCustomLanguageFilePath(m_langFilePath);

        QSignalSpy spy(&translator, &Translator::languageChanged);

        // Cambiar idioma a en
        {
            QFile file(m_langFilePath);
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
            file.write("en\n");
            file.close();
        }

        translator.loadLanguage("en");
        QTRY_COMPARE_WITH_TIMEOUT(translator.currentLanguage(), QStringLiteral("en"), 2000);
        QCOMPARE(translator.translate("", "My Day"), QStringLiteral("My Day"));
    }
};

QTEST_MAIN(tst_Translator)
#include "tst_Translator.moc"
