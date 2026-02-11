#include <QTest>
#include <QSignalSpy>
#include "models/menumodel.h"

class TestMenuModel : public QObject {
    Q_OBJECT

private slots:
    // =========================================================================
    // parseMEDF — valid input
    // =========================================================================
    void testParseMEDF_validLine() {
        MenuModel model;
        QSignalSpy addedSpy(&model, &MenuModel::menuItemAdded);

        bool ok = model.parseMEDF("MEDF0007,AGC Hold Time,RX AGC,DEC,1,0,200,0,0,1;");
        QVERIFY(ok);
        QCOMPARE(addedSpy.count(), 1);
        QCOMPARE(addedSpy.at(0).at(0).toInt(), 7);

        MenuItem *item = model.getMenuItem(7);
        QVERIFY(item != nullptr);
        QCOMPARE(item->id, 7);
        QCOMPARE(item->name, QString("AGC Hold Time"));
        QCOMPARE(item->category, QString("RX AGC"));
        QCOMPARE(item->type, QString("DEC"));
        QCOMPARE(item->flag, 1);
        QCOMPARE(item->minValue, 0);
        QCOMPARE(item->maxValue, 200);
        QCOMPARE(item->defaultValue, 0);
        QCOMPARE(item->currentValue, 0);
        QCOMPARE(item->step, 1);
        QVERIFY(item->options.isEmpty());
    }

    void testParseMEDF_withOptions() {
        MenuModel model;

        bool ok = model.parseMEDF("MEDF0042,NB Mode,RX DSP,BIN,0,0,1,0,0,1,OFF,ON;");
        QVERIFY(ok);

        MenuItem *item = model.getMenuItem(42);
        QVERIFY(item != nullptr);
        QCOMPARE(item->type, QString("BIN"));
        QCOMPARE(item->options.size(), 2);
        QCOMPARE(item->options.at(0), QString("OFF"));
        QCOMPARE(item->options.at(1), QString("ON"));
    }

    void testParseMEDF_urlEncoding() {
        MenuModel model;

        // %2C should decode to comma
        bool ok = model.parseMEDF("MEDF0010,Name%2C with comma,CAT1,DEC,0,0,100,50,50,1;");
        QVERIFY(ok);

        MenuItem *item = model.getMenuItem(10);
        QVERIFY(item != nullptr);
        QCOMPARE(item->name, QString("Name, with comma"));
    }

    void testParseMEDF_noTrailingSemicolon() {
        MenuModel model;

        // Should work without trailing semicolon
        bool ok = model.parseMEDF("MEDF0001,Test,CAT,DEC,0,0,100,0,50,1");
        QVERIFY(ok);
        QVERIFY(model.getMenuItem(1) != nullptr);
    }

    // =========================================================================
    // parseMEDF — invalid input
    // =========================================================================
    void testParseMEDF_missingPrefix() {
        MenuModel model;
        QVERIFY(!model.parseMEDF("0007,AGC Hold Time,RX AGC,DEC,1,0,200,0,0,1;"));
    }

    void testParseMEDF_tooFewFields() {
        MenuModel model;
        QVERIFY(!model.parseMEDF("MEDF0007,AGC Hold Time,RX AGC;"));
    }

    void testParseMEDF_badID() {
        MenuModel model;
        QVERIFY(!model.parseMEDF("MEDFabcd,Test,CAT,DEC,0,0,100,0,0,1;"));
    }

    void testParseMEDF_emptyLine() {
        MenuModel model;
        QVERIFY(!model.parseMEDF(""));
    }

    // =========================================================================
    // parseME — valid input
    // =========================================================================
    void testParseME_updatesExistingItem() {
        MenuModel model;
        model.parseMEDF("MEDF0007,AGC Hold Time,RX AGC,DEC,1,0,200,0,0,1;");

        QSignalSpy changedSpy(&model, &MenuModel::menuValueChanged);

        bool ok = model.parseME("ME0007.0123;");
        QVERIFY(ok);
        QCOMPARE(changedSpy.count(), 1);
        QCOMPARE(changedSpy.at(0).at(0).toInt(), 7);
        QCOMPARE(changedSpy.at(0).at(1).toInt(), 123);

        QCOMPARE(model.getMenuItem(7)->currentValue, 123);
    }

    void testParseME_noChangeNoSignal() {
        MenuModel model;
        model.parseMEDF("MEDF0007,Test,CAT,DEC,0,0,200,0,50,1;");

        QSignalSpy changedSpy(&model, &MenuModel::menuValueChanged);

        // Set to same value as current (50)
        model.parseME("ME0007.0050;");
        QCOMPARE(changedSpy.count(), 0);
    }

    void testParseME_unknownID() {
        MenuModel model;
        // No items added — parseME should succeed but do nothing
        bool ok = model.parseME("ME9999.0050;");
        QVERIFY(ok); // Parsing succeeds
        // No crash, no item created
        QVERIFY(model.getMenuItem(9999) == nullptr);
    }

    // =========================================================================
    // parseME — invalid input
    // =========================================================================
    void testParseME_noDotSeparator() {
        MenuModel model;
        QVERIFY(!model.parseME("ME00070123;"));
    }

    void testParseME_badFormat() {
        MenuModel model;
        QVERIFY(!model.parseME("XY0007.0123;"));
    }

    void testParseME_medfPrefixRejected() {
        MenuModel model;
        // parseME should reject lines starting with MEDF
        QVERIFY(!model.parseME("MEDF0007,stuff;"));
    }

    // =========================================================================
    // getMenuItem
    // =========================================================================
    void testGetMenuItem_exists() {
        MenuModel model;
        model.parseMEDF("MEDF0005,Test,CAT,DEC,0,0,100,0,0,1;");

        QVERIFY(model.getMenuItem(5) != nullptr);
        QCOMPARE(model.getMenuItem(5)->name, QString("Test"));
    }

    void testGetMenuItem_notExists() {
        MenuModel model;
        QVERIFY(model.getMenuItem(999) == nullptr);
    }

    // =========================================================================
    // getMenuItemByName
    // =========================================================================
    void testGetMenuItemByName_exists() {
        MenuModel model;
        model.parseMEDF("MEDF0001,AGC Hold Time,RX AGC,DEC,0,0,200,0,0,1;");

        MenuItem *item = model.getMenuItemByName("AGC Hold Time");
        QVERIFY(item != nullptr);
        QCOMPARE(item->id, 1);
    }

    void testGetMenuItemByName_notExists() {
        MenuModel model;
        QVERIFY(model.getMenuItemByName("Nonexistent") == nullptr);
    }

    // =========================================================================
    // getAllItems — sorted by name (case-insensitive)
    // =========================================================================
    void testGetAllItems_sortedByName() {
        MenuModel model;
        model.parseMEDF("MEDF0001,Zebra,CAT,DEC,0,0,1,0,0,1;");
        model.parseMEDF("MEDF0002,apple,CAT,DEC,0,0,1,0,0,1;");
        model.parseMEDF("MEDF0003,Banana,CAT,DEC,0,0,1,0,0,1;");

        auto items = model.getAllItems();
        QCOMPARE(items.size(), 3);
        QCOMPARE(items[0]->name, QString("apple"));
        QCOMPARE(items[1]->name, QString("Banana"));
        QCOMPARE(items[2]->name, QString("Zebra"));
    }

    // =========================================================================
    // getItemsByCategory
    // =========================================================================
    void testGetItemsByCategory() {
        MenuModel model;
        model.parseMEDF("MEDF0001,Item A,RX AGC,DEC,0,0,1,0,0,1;");
        model.parseMEDF("MEDF0002,Item B,TX,DEC,0,0,1,0,0,1;");
        model.parseMEDF("MEDF0003,Item C,RX AGC,DEC,0,0,1,0,0,1;");

        auto items = model.getItemsByCategory("RX AGC");
        QCOMPARE(items.size(), 2);
        // Should be sorted by name
        QCOMPARE(items[0]->name, QString("Item A"));
        QCOMPARE(items[1]->name, QString("Item C"));
    }

    void testGetItemsByCategory_empty() {
        MenuModel model;
        model.parseMEDF("MEDF0001,Test,CAT1,DEC,0,0,1,0,0,1;");

        auto items = model.getItemsByCategory("NONEXISTENT");
        QCOMPARE(items.size(), 0);
    }

    // =========================================================================
    // filterByName
    // =========================================================================
    void testFilterByName_caseInsensitive() {
        MenuModel model;
        model.parseMEDF("MEDF0001,AGC Hold Time,RX,DEC,0,0,200,0,0,1;");
        model.parseMEDF("MEDF0002,NB Level,RX,DEC,0,0,15,0,0,1;");
        model.parseMEDF("MEDF0003,agc speed,RX,DEC,0,0,2,0,0,1;");

        auto items = model.filterByName("agc");
        QCOMPARE(items.size(), 2);
        // Sorted: "AGC Hold Time", "agc speed"
        QCOMPARE(items[0]->name, QString("AGC Hold Time"));
        QCOMPARE(items[1]->name, QString("agc speed"));
    }

    void testFilterByName_emptyPattern() {
        MenuModel model;
        model.parseMEDF("MEDF0001,A,CAT,DEC,0,0,1,0,0,1;");
        model.parseMEDF("MEDF0002,B,CAT,DEC,0,0,1,0,0,1;");

        // Empty pattern returns all items
        auto items = model.filterByName("");
        QCOMPARE(items.size(), 2);
    }

    // =========================================================================
    // getCategories
    // =========================================================================
    void testGetCategories_uniqueSorted() {
        MenuModel model;
        model.parseMEDF("MEDF0001,A,TX,DEC,0,0,1,0,0,1;");
        model.parseMEDF("MEDF0002,B,RX AGC,DEC,0,0,1,0,0,1;");
        model.parseMEDF("MEDF0003,C,TX,DEC,0,0,1,0,0,1;");
        model.parseMEDF("MEDF0004,D,AUDIO,DEC,0,0,1,0,0,1;");

        QStringList cats = model.getCategories();
        QCOMPARE(cats.size(), 3);
        QCOMPARE(cats[0], QString("AUDIO"));
        QCOMPARE(cats[1], QString("RX AGC"));
        QCOMPARE(cats[2], QString("TX"));
    }

    // =========================================================================
    // MenuItem helpers
    // =========================================================================
    void testMenuItem_isBinary() {
        MenuItem item;
        item.type = "BIN";
        QVERIFY(item.isBinary());
        item.type = "DEC";
        QVERIFY(!item.isBinary());
    }

    void testMenuItem_isReadOnly() {
        MenuItem item;
        item.flag = 2;
        QVERIFY(item.isReadOnly());
        item.flag = 0;
        QVERIFY(!item.isReadOnly());
        item.flag = 1;
        QVERIFY(!item.isReadOnly());
    }

    void testMenuItem_displayValue_withOptions() {
        MenuItem item;
        item.options = {"OFF", "ON", "AUTO"};
        item.currentValue = 1;
        QCOMPARE(item.displayValue(), QString("ON"));

        item.currentValue = 0;
        QCOMPARE(item.displayValue(), QString("OFF"));

        item.currentValue = 2;
        QCOMPARE(item.displayValue(), QString("AUTO"));
    }

    void testMenuItem_displayValue_withoutOptions() {
        MenuItem item;
        item.currentValue = 42;
        QCOMPARE(item.displayValue(), QString("42"));
    }

    void testMenuItem_displayValue_outOfRange() {
        MenuItem item;
        item.options = {"OFF", "ON"};
        item.currentValue = 5; // Out of range
        // Falls through to number display
        QCOMPARE(item.displayValue(), QString("5"));
    }

    // =========================================================================
    // Signals: modelCleared
    // =========================================================================
    void testClear() {
        MenuModel model;
        model.parseMEDF("MEDF0001,Test,CAT,DEC,0,0,1,0,0,1;");
        QCOMPARE(model.count(), 1);

        QSignalSpy clearedSpy(&model, &MenuModel::modelCleared);
        model.clear();

        QCOMPARE(model.count(), 0);
        QCOMPARE(clearedSpy.count(), 1);
        QVERIFY(model.getMenuItem(1) == nullptr);
    }

    // =========================================================================
    // count
    // =========================================================================
    void testCount() {
        MenuModel model;
        QCOMPARE(model.count(), 0);

        model.parseMEDF("MEDF0001,A,CAT,DEC,0,0,1,0,0,1;");
        QCOMPARE(model.count(), 1);

        model.parseMEDF("MEDF0002,B,CAT,DEC,0,0,1,0,0,1;");
        QCOMPARE(model.count(), 2);
    }

    // =========================================================================
    // addSyntheticDisplayFpsItem
    // =========================================================================
    void testSyntheticDisplayFpsItem() {
        MenuModel model;
        QSignalSpy addedSpy(&model, &MenuModel::menuItemAdded);

        model.addSyntheticDisplayFpsItem(24);
        QCOMPARE(addedSpy.count(), 1);
        QCOMPARE(addedSpy.at(0).at(0).toInt(), MenuModel::SYNTHETIC_DISPLAY_FPS_ID);

        MenuItem *item = model.getMenuItem(MenuModel::SYNTHETIC_DISPLAY_FPS_ID);
        QVERIFY(item != nullptr);
        QCOMPARE(item->name, QString("Display FPS"));
        QCOMPARE(item->category, QString("APP SETTINGS"));
        QCOMPARE(item->currentValue, 24);
        QCOMPARE(item->minValue, 12);
        QCOMPARE(item->maxValue, 30);
    }

    // =========================================================================
    // updateValue directly
    // =========================================================================
    void testUpdateValue_emitsSignal() {
        MenuModel model;
        model.parseMEDF("MEDF0001,Test,CAT,DEC,0,0,100,0,50,1;");

        QSignalSpy spy(&model, &MenuModel::menuValueChanged);
        model.updateValue(1, 75);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(model.getMenuItem(1)->currentValue, 75);
    }

    void testUpdateValue_sameValueNoSignal() {
        MenuModel model;
        model.parseMEDF("MEDF0001,Test,CAT,DEC,0,0,100,0,50,1;");

        QSignalSpy spy(&model, &MenuModel::menuValueChanged);
        model.updateValue(1, 50); // Same as current

        QCOMPARE(spy.count(), 0);
    }

    void testUpdateValue_nonexistentId() {
        MenuModel model;
        QSignalSpy spy(&model, &MenuModel::menuValueChanged);
        model.updateValue(999, 42); // No such ID
        QCOMPARE(spy.count(), 0); // No crash, no signal
    }

    // =========================================================================
    // const getMenuItem overload
    // =========================================================================
    void testGetMenuItem_const() {
        MenuModel model;
        model.parseMEDF("MEDF0001,Test,CAT,DEC,0,0,100,0,50,1;");

        const MenuModel &constModel = model;
        const MenuItem *item = constModel.getMenuItem(1);
        QVERIFY(item != nullptr);
        QCOMPARE(item->name, QString("Test"));

        QVERIFY(constModel.getMenuItem(999) == nullptr);
    }

    void testGetMenuItemByName_const() {
        MenuModel model;
        model.parseMEDF("MEDF0001,Test Item,CAT,DEC,0,0,100,0,50,1;");

        const MenuModel &constModel = model;
        const MenuItem *item = constModel.getMenuItemByName("Test Item");
        QVERIFY(item != nullptr);
        QCOMPARE(item->id, 1);

        QVERIFY(constModel.getMenuItemByName("Nope") == nullptr);
    }
};

QTEST_MAIN(TestMenuModel)
#include "test_menumodel.moc"
