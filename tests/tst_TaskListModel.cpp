#include <QtTest>
#include <QSignalSpy>
#include "core/Database.h"
#include "models/LocalRepository.h"
#include "models/TaskListModel.h"

class tst_TaskListModel : public QObject {
    Q_OBJECT

private:
    Database *m_db = nullptr;
    LocalRepository *m_repo = nullptr;

private slots:
    void initTestCase() {
        m_db = new Database(this);
        QVERIFY(m_db->initialize());
        m_repo = new LocalRepository(this);
    }

    void cleanupTestCase() {
        QThreadPool::globalInstance()->waitForDone(5000);
    }

    void testParseDisplayName() {
        QString emoji, clean;

        // Normal list without emoji
        TaskListModel::parseDisplayName("Trabajo", emoji, clean);
        QCOMPARE(emoji, QString(""));
        QCOMPARE(clean, QString("Trabajo"));

        // Single code point emoji
        TaskListModel::parseDisplayName("🛒 Compras", emoji, clean);
        QCOMPARE(emoji, QString("🛒"));
        QCOMPARE(clean, QString("Compras"));

        // Emoji without space
        TaskListModel::parseDisplayName("💼Proyectos", emoji, clean);
        QCOMPARE(emoji, QString("💼"));
        QCOMPARE(clean, QString("Proyectos"));

        // Emoji with variation selector (FE0F)
        TaskListModel::parseDisplayName("✈️ Viajes", emoji, clean);
        QCOMPARE(emoji, QString("✈️"));
        QCOMPARE(clean, QString("Viajes"));

        // Emoji ZWJ sequence
        TaskListModel::parseDisplayName("👨‍💻 Desarrollo", emoji, clean);
        QCOMPARE(emoji, QString("👨‍💻"));
        QCOMPARE(clean, QString("Desarrollo"));

        // Only emoji
        TaskListModel::parseDisplayName("⭐", emoji, clean);
        QCOMPARE(emoji, QString("⭐"));
        QCOMPARE(clean, QString(""));
    }

    void testCreateListAndRoles() {
        TaskListModel model(m_repo);
        model.loadLists();
        QTRY_VERIFY(model.rowCount() >= 0);

        int initialCount = model.rowCount();
        model.createList("Supermercado", "🛒");
        QCOMPARE(model.rowCount(), initialCount + 1);

        QModelIndex lastIdx = model.index(model.rowCount() - 1, 0);
        QCOMPARE(model.data(lastIdx, TaskListModel::DisplayNameRole).toString(), QString("🛒 Supermercado"));
        QCOMPARE(model.data(lastIdx, TaskListModel::NameRole).toString(), QString("Supermercado"));
        QCOMPARE(model.data(lastIdx, TaskListModel::IconRole).toString(), QString("🛒"));

        QString listId = model.data(lastIdx, TaskListModel::IdRole).toString();
        QCOMPARE(model.getListName(listId), QString("Supermercado"));
        QCOMPARE(model.getListIcon(listId), QString("🛒"));
    }

    void testRenameListPreservingIcon() {
        TaskListModel model(m_repo);
        model.createList("Original", "💼");
        QModelIndex idx = model.index(model.rowCount() - 1, 0);
        QString listId = model.data(idx, TaskListModel::IdRole).toString();

        // Rename without changing emoji
        model.renameList(listId, "Renombrado");
        QCOMPARE(model.data(idx, TaskListModel::DisplayNameRole).toString(), QString("💼 Renombrado"));
        QCOMPARE(model.data(idx, TaskListModel::NameRole).toString(), QString("Renombrado"));
        QCOMPARE(model.data(idx, TaskListModel::IconRole).toString(), QString("💼"));

        // Rename with a new emoji entered in text
        model.renameList(listId, "🚀 Proyecto Alfa");
        QCOMPARE(model.data(idx, TaskListModel::DisplayNameRole).toString(), QString("🚀 Proyecto Alfa"));
        QCOMPARE(model.data(idx, TaskListModel::NameRole).toString(), QString("Proyecto Alfa"));
        QCOMPARE(model.data(idx, TaskListModel::IconRole).toString(), QString("🚀"));
    }

    void testSetAndRemoveListIcon() {
        TaskListModel model(m_repo);
        model.createList("Gimnasio");
        QModelIndex idx = model.index(model.rowCount() - 1, 0);
        QString listId = model.data(idx, TaskListModel::IdRole).toString();

        // Initially no icon
        QCOMPARE(model.data(idx, TaskListModel::IconRole).toString(), QString(""));
        QCOMPARE(model.data(idx, TaskListModel::NameRole).toString(), QString("Gimnasio"));

        // Set icon
        model.setListIcon(listId, "🏋️");
        QCOMPARE(model.data(idx, TaskListModel::IconRole).toString(), QString("🏋️"));
        QCOMPARE(model.data(idx, TaskListModel::NameRole).toString(), QString("Gimnasio"));
        QCOMPARE(model.data(idx, TaskListModel::DisplayNameRole).toString(), QString("🏋️ Gimnasio"));

        // Change icon
        model.setListIcon(listId, "🏃");
        QCOMPARE(model.data(idx, TaskListModel::IconRole).toString(), QString("🏃"));
        QCOMPARE(model.data(idx, TaskListModel::DisplayNameRole).toString(), QString("🏃 Gimnasio"));

        // Remove icon (empty string -> default)
        model.setListIcon(listId, "");
        QCOMPARE(model.data(idx, TaskListModel::IconRole).toString(), QString(""));
        QCOMPARE(model.data(idx, TaskListModel::NameRole).toString(), QString("Gimnasio"));
        QCOMPARE(model.data(idx, TaskListModel::DisplayNameRole).toString(), QString("Gimnasio"));
    }
};

QTEST_MAIN(tst_TaskListModel)
#include "tst_TaskListModel.moc"
