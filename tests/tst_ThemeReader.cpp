#include <QtTest>
#include "core/ThemeReader.h"

class tst_ThemeReader : public QObject {
    Q_OBJECT

private slots:
    void testValidToml() {
        ThemeReader reader(QStringLiteral(TEST_FIXTURES_DIR "/valid.toml"));
        QCOMPARE(reader.background().name(), QStringLiteral("#1a1a2e"));
        QCOMPARE(reader.foreground().name(), QStringLiteral("#e0e0e0"));
        QCOMPARE(reader.accent().name(), QStringLiteral("#7aa2f7"));
        QCOMPARE(reader.surface().name(), QStringLiteral("#16213e"));
        QCOMPARE(reader.border().name(), QStringLiteral("#2a2a4a"));
        QCOMPARE(reader.error().name(), QStringLiteral("#f7768e"));
    }

    void testMissingSection() {
        ThemeReader reader(QStringLiteral(TEST_FIXTURES_DIR "/missing_section.toml"));
        QCOMPARE(reader.background().name(), QStringLiteral("#1a1a2e"));
        QCOMPARE(reader.foreground().name(), QStringLiteral("#e0e0e0"));
        // Otras retienen sus defaults
        QCOMPARE(reader.accent().name(), QStringLiteral("#7aa2f7"));
    }

    void testMissingKey() {
        ThemeReader reader(QStringLiteral(TEST_FIXTURES_DIR "/missing_key.toml"));
        QCOMPARE(reader.background().name(), QStringLiteral("#1a1a2e"));
        QCOMPARE(reader.accent().name(), QStringLiteral("#7aa2f7"));
        // foreground retiene default
        QCOMPARE(reader.foreground().name(), QStringLiteral("#e0e0e0"));
    }

    void testMalformedColor() {
        ThemeReader reader(QStringLiteral(TEST_FIXTURES_DIR "/malformed_color.toml"));
        // background es inválido en fixture, retiene default "#1a1a2e"
        QCOMPARE(reader.background().name(), QStringLiteral("#1a1a2e"));
        QCOMPARE(reader.foreground().name(), QStringLiteral("#e0e0e0"));
    }

    void testEmptyFile() {
        ThemeReader reader(QStringLiteral(TEST_FIXTURES_DIR "/empty.toml"));
        QCOMPARE(reader.background().name(), QStringLiteral("#1a1a2e"));
        QCOMPARE(reader.foreground().name(), QStringLiteral("#e0e0e0"));
        QCOMPARE(reader.accent().name(), QStringLiteral("#7aa2f7"));
    }

    void testNonExistentFileFallback() {
        ThemeReader reader(QStringLiteral("/path/does/not/exist/colors.toml"));
        QCOMPARE(reader.background().name(), QStringLiteral("#1a1a2e"));
        QCOMPARE(reader.foreground().name(), QStringLiteral("#e0e0e0"));
    }
};

QTEST_MAIN(tst_ThemeReader)
#include "tst_ThemeReader.moc"
