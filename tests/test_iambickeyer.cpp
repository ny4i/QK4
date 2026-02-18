#include <QTest>
#include <QSignalSpy>
#include "keyer/iambickeyer.h"

class TestIambicKeyer : public QObject {
    Q_OBJECT

private slots:
    void init();

    // Basic element tests
    void ditPressRelease();
    void dahPressRelease();

    // Repetition (same paddle held)
    void ditRepeat();
    void dahRepeat();

    // Squeeze alternation
    void squeezeAlternation();

    // Mode B: extra element after squeeze release
    void modeBSqueezeRelease();

    // Mode A: no extra element after squeeze release
    void modeASqueezeRelease();

    // Both paddles from idle → dit first
    void bothPaddlesFromIdle();

    // Stop resets state
    void stopResetsState();

    // WPM affects timing
    void wpmAffectsTiming();

private:
    void processEventsFor(int ms);
};

void TestIambicKeyer::init() {
    // Nothing — each test creates its own keyer
}

void TestIambicKeyer::processEventsFor(int ms) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        QThread::msleep(1);
    }
}

void TestIambicKeyer::ditPressRelease() {
    IambicKeyer keyer;
    keyer.setWpm(25); // dit = 48ms

    QSignalSpy keyDownSpy(&keyer, &IambicKeyer::keyDown);
    QSignalSpy keyUpSpy(&keyer, &IambicKeyer::keyUp);

    // Press dit
    keyer.updatePaddleState(true, false);
    QCOMPARE(keyDownSpy.count(), 1);
    QCOMPARE(keyDownSpy.at(0).at(0).toBool(), true); // isDit = true

    // Release dit
    keyer.updatePaddleState(false, false);

    // Wait for element + space timers to expire
    processEventsFor(120);

    QCOMPARE(keyUpSpy.count(), 1);
    // Should go idle — no more elements
    QCOMPARE(keyDownSpy.count(), 1);
}

void TestIambicKeyer::dahPressRelease() {
    IambicKeyer keyer;
    keyer.setWpm(25);

    QSignalSpy keyDownSpy(&keyer, &IambicKeyer::keyDown);
    QSignalSpy keyUpSpy(&keyer, &IambicKeyer::keyUp);

    keyer.updatePaddleState(false, true);
    QCOMPARE(keyDownSpy.count(), 1);
    QCOMPARE(keyDownSpy.at(0).at(0).toBool(), false); // isDit = false (dah)

    keyer.updatePaddleState(false, false);

    processEventsFor(250);

    QCOMPARE(keyUpSpy.count(), 1);
    QCOMPARE(keyDownSpy.count(), 1);
}

void TestIambicKeyer::ditRepeat() {
    IambicKeyer keyer;
    keyer.setWpm(25); // dit = 48ms

    QSignalSpy keyDownSpy(&keyer, &IambicKeyer::keyDown);

    // Press and hold dit
    keyer.updatePaddleState(true, false);
    QCOMPARE(keyDownSpy.count(), 1);

    // Wait for element + space → should repeat
    processEventsFor(120);

    QVERIFY(keyDownSpy.count() >= 2);
    // All should be dits
    for (int i = 0; i < keyDownSpy.count(); ++i) {
        QCOMPARE(keyDownSpy.at(i).at(0).toBool(), true);
    }
}

void TestIambicKeyer::dahRepeat() {
    IambicKeyer keyer;
    keyer.setWpm(25); // dah = 144ms

    QSignalSpy keyDownSpy(&keyer, &IambicKeyer::keyDown);

    keyer.updatePaddleState(false, true);
    QCOMPARE(keyDownSpy.count(), 1);

    // Wait for dah + space → should repeat
    processEventsFor(250);

    QVERIFY(keyDownSpy.count() >= 2);
    for (int i = 0; i < keyDownSpy.count(); ++i) {
        QCOMPARE(keyDownSpy.at(i).at(0).toBool(), false);
    }
}

void TestIambicKeyer::squeezeAlternation() {
    IambicKeyer keyer;
    keyer.setWpm(25);

    QSignalSpy keyDownSpy(&keyer, &IambicKeyer::keyDown);

    // Squeeze both paddles
    keyer.updatePaddleState(true, true);
    QCOMPARE(keyDownSpy.count(), 1);
    QCOMPARE(keyDownSpy.at(0).at(0).toBool(), true); // dit first

    // Wait for dit + space + next element
    processEventsFor(120);

    QVERIFY(keyDownSpy.count() >= 2);
    // Second element should be dah (alternation)
    QCOMPARE(keyDownSpy.at(1).at(0).toBool(), false);

    // Wait for dah + space + next element
    processEventsFor(250);

    QVERIFY(keyDownSpy.count() >= 3);
    // Third element should be dit again
    QCOMPARE(keyDownSpy.at(2).at(0).toBool(), true);
}

void TestIambicKeyer::modeBSqueezeRelease() {
    IambicKeyer keyer;
    keyer.setWpm(25);
    keyer.setMode(IambicMode::IambicB);

    QSignalSpy keyDownSpy(&keyer, &IambicKeyer::keyDown);

    // Squeeze both paddles
    keyer.updatePaddleState(true, true);
    QCOMPARE(keyDownSpy.count(), 1);
    QCOMPARE(keyDownSpy.at(0).at(0).toBool(), true); // dit

    // Wait for dit element to finish, then release during inter-element space
    processEventsFor(55);

    // Release both paddles during inter-element space
    keyer.updatePaddleState(false, false);

    // Wait for space to expire and Mode B extra element
    processEventsFor(100);

    // Mode B should send one more alternate element (dah) after squeeze release
    QVERIFY(keyDownSpy.count() >= 2);
    QCOMPARE(keyDownSpy.at(1).at(0).toBool(), false); // dah (Mode B extra)
}

void TestIambicKeyer::modeASqueezeRelease() {
    IambicKeyer keyer;
    keyer.setWpm(25);
    keyer.setMode(IambicMode::IambicA);

    QSignalSpy keyDownSpy(&keyer, &IambicKeyer::keyDown);

    // Squeeze both paddles
    keyer.updatePaddleState(true, true);
    QCOMPARE(keyDownSpy.count(), 1);

    // Wait for dit to finish, release during inter-element space
    processEventsFor(55);
    keyer.updatePaddleState(false, false);

    // Wait for space to expire
    processEventsFor(100);

    // Mode A: no extra element after release — should stop at 1
    QCOMPARE(keyDownSpy.count(), 1);
}

void TestIambicKeyer::bothPaddlesFromIdle() {
    IambicKeyer keyer;
    keyer.setWpm(25);

    QSignalSpy keyDownSpy(&keyer, &IambicKeyer::keyDown);

    keyer.updatePaddleState(true, true);
    QCOMPARE(keyDownSpy.count(), 1);
    // Both from idle → dit first
    QCOMPARE(keyDownSpy.at(0).at(0).toBool(), true);
}

void TestIambicKeyer::stopResetsState() {
    IambicKeyer keyer;
    keyer.setWpm(25);

    QSignalSpy keyDownSpy(&keyer, &IambicKeyer::keyDown);
    QSignalSpy keyUpSpy(&keyer, &IambicKeyer::keyUp);

    // Start sending
    keyer.updatePaddleState(true, false);
    QCOMPARE(keyDownSpy.count(), 1);

    // Stop mid-tone
    keyer.stop();
    QCOMPARE(keyUpSpy.count(), 1); // keyUp emitted on stop during tone

    // Wait — should NOT produce more elements
    processEventsFor(120);
    QCOMPARE(keyDownSpy.count(), 1);
    QCOMPARE(keyUpSpy.count(), 1);
}

void TestIambicKeyer::wpmAffectsTiming() {
    IambicKeyer keyer;

    keyer.setWpm(20);
    QCOMPARE(keyer.wpm(), 20);
    // 1200/20 = 60ms dit

    keyer.setWpm(30);
    QCOMPARE(keyer.wpm(), 30);
    // 1200/30 = 40ms dit

    // Invalid WPM defaults to 25
    keyer.setWpm(0);
    QCOMPARE(keyer.wpm(), 25);
}

QTEST_MAIN(TestIambicKeyer)
#include "test_iambickeyer.moc"
