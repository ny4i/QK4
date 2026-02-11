#include <QTest>
#include <QSignalSpy>
#include "models/radiostate.h"

class TestRadioState : public QObject {
    Q_OBJECT

private slots:
    // =========================================================================
    // Static helpers: modeFromCode
    // =========================================================================
    void testModeFromCode_allModes() {
        QCOMPARE(RadioState::modeFromCode(1), RadioState::LSB);
        QCOMPARE(RadioState::modeFromCode(2), RadioState::USB);
        QCOMPARE(RadioState::modeFromCode(3), RadioState::CW);
        QCOMPARE(RadioState::modeFromCode(4), RadioState::FM);
        QCOMPARE(RadioState::modeFromCode(5), RadioState::AM);
        QCOMPARE(RadioState::modeFromCode(6), RadioState::DATA);
        QCOMPARE(RadioState::modeFromCode(7), RadioState::CW_R);
        QCOMPARE(RadioState::modeFromCode(9), RadioState::DATA_R);
    }

    void testModeFromCode_unknown() {
        // Unknown codes default to USB
        QCOMPARE(RadioState::modeFromCode(0), RadioState::USB);
        QCOMPARE(RadioState::modeFromCode(8), RadioState::USB);
        QCOMPARE(RadioState::modeFromCode(99), RadioState::USB);
        QCOMPARE(RadioState::modeFromCode(-1), RadioState::USB);
    }

    // =========================================================================
    // Static helpers: modeToString
    // =========================================================================
    void testModeToString_allModes() {
        QCOMPARE(RadioState::modeToString(RadioState::LSB), QString("LSB"));
        QCOMPARE(RadioState::modeToString(RadioState::USB), QString("USB"));
        QCOMPARE(RadioState::modeToString(RadioState::CW), QString("CW"));
        QCOMPARE(RadioState::modeToString(RadioState::FM), QString("FM"));
        QCOMPARE(RadioState::modeToString(RadioState::AM), QString("AM"));
        QCOMPARE(RadioState::modeToString(RadioState::DATA), QString("DATA"));
        QCOMPARE(RadioState::modeToString(RadioState::CW_R), QString("CW-R"));
        QCOMPARE(RadioState::modeToString(RadioState::DATA_R), QString("DATA-R"));
    }

    // =========================================================================
    // Static helpers: dataSubModeToString
    // =========================================================================
    void testDataSubModeToString() {
        QCOMPARE(RadioState::dataSubModeToString(0), QString("DATA"));
        QCOMPARE(RadioState::dataSubModeToString(1), QString("AFSK"));
        QCOMPARE(RadioState::dataSubModeToString(2), QString("FSK"));
        QCOMPARE(RadioState::dataSubModeToString(3), QString("PSK"));
        QCOMPARE(RadioState::dataSubModeToString(99), QString("DATA")); // unknown → DATA
    }

    // =========================================================================
    // FA command: VFO A frequency
    // =========================================================================
    void testFA_setsFrequency() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::frequencyChanged);

        state.parseCATCommand("FA00014060000;");

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toULongLong(), quint64(14060000));
        QCOMPARE(state.frequency(), quint64(14060000));
        QCOMPARE(state.vfoA(), quint64(14060000));
    }

    void testFA_shortCommand() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::frequencyChanged);

        // "FA" with no value — length <= 2, should be ignored
        state.parseCATCommand("FA;");
        QCOMPARE(spy.count(), 0);
    }

    // =========================================================================
    // FB command: VFO B frequency
    // =========================================================================
    void testFB_setsFrequencyB() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::frequencyBChanged);

        state.parseCATCommand("FB00007050000;");

        QCOMPARE(spy.count(), 1);
        QCOMPARE(state.vfoB(), quint64(7050000));
    }

    // =========================================================================
    // MD command: Mode
    // =========================================================================
    void testMD_setsCW() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::modeChanged);

        state.parseCATCommand("MD3;");

        QCOMPARE(spy.count(), 1);
        QCOMPARE(state.mode(), RadioState::CW);
        QCOMPARE(state.modeString(), QString("CW"));
    }

    void testMD_setsLSB() {
        RadioState state;
        state.parseCATCommand("MD1;");
        QCOMPARE(state.mode(), RadioState::LSB);
    }

    void testMD_setsDATA() {
        RadioState state;
        state.parseCATCommand("MD6;");
        QCOMPARE(state.mode(), RadioState::DATA);
    }

    void testMD_noChangeNoSignal() {
        RadioState state;
        state.parseCATCommand("MD2;"); // USB (default)

        QSignalSpy spy(&state, &RadioState::modeChanged);
        state.parseCATCommand("MD2;"); // Same mode
        QCOMPARE(spy.count(), 0);
    }

    // =========================================================================
    // BW command: Filter bandwidth
    // =========================================================================
    void testBW_setsBandwidth() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::filterBandwidthChanged);

        // BW0050 → 50 * 10 = 500 Hz (different from default 2400)
        state.parseCATCommand("BW0050;");

        QCOMPARE(spy.count(), 1);
        QCOMPARE(state.filterBandwidth(), 500);
    }

    void testBWSub_setsBandwidthB() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::filterBandwidthBChanged);

        state.parseCATCommand("BW$0050;");

        QCOMPARE(spy.count(), 1);
        QCOMPARE(state.filterBandwidthB(), 500);
    }

    // =========================================================================
    // SM command: S-Meter
    // =========================================================================
    void testSM_lowValues() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::sMeterChanged);

        // SM00 → 0/2.0 = S0
        state.parseCATCommand("SM00;");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(state.sMeter(), 0.0);

        // SM18 → 18/2.0 = S9
        state.parseCATCommand("SM18;");
        QCOMPARE(state.sMeter(), 9.0);
    }

    void testSM_aboveS9() {
        RadioState state;

        // SM21 → bars=21, dbAboveS9 = (21-18)*3 = 9, sMeter = 9.0 + 0.9 = 9.9
        state.parseCATCommand("SM21;");
        QVERIFY(qFuzzyCompare(state.sMeter(), 9.9));
    }

    // =========================================================================
    // sMeterString
    // =========================================================================
    void testSMeterString_belowS9() {
        RadioState state;
        state.parseCATCommand("SM10;"); // 10/2.0 = S5
        QCOMPARE(state.sMeterString(), QString("S5"));
    }

    void testSMeterString_atS9() {
        RadioState state;
        state.parseCATCommand("SM18;"); // S9
        QCOMPARE(state.sMeterString(), QString("S9"));
    }

    void testSMeterString_aboveS9() {
        RadioState state;
        // bars=28 → dbAboveS9 = (28-18)*3 = 30, sMeter = 9.0 + 3.0 = 12.0
        // sMeterString: (12.0 - 9.0) * 10 = 30 → "S9+30"
        state.parseCATCommand("SM28;");
        QCOMPARE(state.sMeterString(), QString("S9+30"));
    }

    // =========================================================================
    // PC command: Power Control
    // =========================================================================
    void testPC_qroMode() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::rfPowerChanged);

        // PC050H → 50W QRO
        state.parseCATCommand("PC050H;");

        QCOMPARE(spy.count(), 1);
        QCOMPARE(state.rfPower(), 50.0);
        QVERIFY(!state.isQrpMode());
    }

    void testPC_qrpMode() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::rfPowerChanged);

        // PC099L → 9.9W QRP
        state.parseCATCommand("PC099L;");

        QCOMPARE(spy.count(), 1);
        QCOMPARE(state.rfPower(), 9.9);
        QVERIFY(state.isQrpMode());
    }

    void testPC_xvtrMode_ignored() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::rfPowerChanged);

        // XVTR mode is skipped
        state.parseCATCommand("PC050X;");
        QCOMPARE(spy.count(), 0);
    }

    // =========================================================================
    // TX/RX commands: transmit state
    // =========================================================================
    void testTX_setsTransmitting() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::transmitStateChanged);

        QVERIFY(!state.isTransmitting());

        state.parseCATCommand("TX;");
        QVERIFY(state.isTransmitting());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), true);
    }

    void testRX_clearsTransmitting() {
        RadioState state;
        state.parseCATCommand("TX;");
        QVERIFY(state.isTransmitting());

        QSignalSpy spy(&state, &RadioState::transmitStateChanged);
        state.parseCATCommand("RX;");

        QVERIFY(!state.isTransmitting());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), false);
    }

    void testTX_alreadyTransmitting_noSignal() {
        RadioState state;
        state.parseCATCommand("TX;");

        QSignalSpy spy(&state, &RadioState::transmitStateChanged);
        state.parseCATCommand("TX;"); // Already transmitting
        QCOMPARE(spy.count(), 0);
    }

    void testRX_alreadyReceiving_noSignal() {
        RadioState state;
        // Default is not transmitting

        QSignalSpy spy(&state, &RadioState::transmitStateChanged);
        state.parseCATCommand("RX;"); // Already receiving
        QCOMPARE(spy.count(), 0);
    }

    // =========================================================================
    // modeStringFull: DATA mode + sub-mode
    // =========================================================================
    void testModeStringFull_dataWithSubMode() {
        RadioState state;
        state.parseCATCommand("MD6;"); // DATA mode
        state.setDataSubMode(0);
        QCOMPARE(state.modeStringFull(), QString("DATA"));

        state.setDataSubMode(1);
        QCOMPARE(state.modeStringFull(), QString("AFSK"));

        state.setDataSubMode(2);
        QCOMPARE(state.modeStringFull(), QString("FSK"));

        state.setDataSubMode(3);
        QCOMPARE(state.modeStringFull(), QString("PSK"));
    }

    void testModeStringFull_nonDataMode() {
        RadioState state;
        state.parseCATCommand("MD3;"); // CW
        QCOMPARE(state.modeStringFull(), QString("CW"));

        state.parseCATCommand("MD1;"); // LSB
        QCOMPARE(state.modeStringFull(), QString("LSB"));
    }

    void testModeStringFull_dataR() {
        RadioState state;
        state.parseCATCommand("MD9;"); // DATA-R
        state.setDataSubMode(2);
        QCOMPARE(state.modeStringFull(), QString("FSK"));
    }

    // =========================================================================
    // monitorLevelForCurrentMode
    // =========================================================================
    void testMonitorLevelForCurrentMode() {
        RadioState state;
        state.setMonitorLevel(0, 30); // CW
        state.setMonitorLevel(1, 50); // Data
        state.setMonitorLevel(2, 70); // Voice

        state.parseCATCommand("MD3;"); // CW
        QCOMPARE(state.monitorLevelForCurrentMode(), 30);

        state.parseCATCommand("MD6;"); // DATA
        QCOMPARE(state.monitorLevelForCurrentMode(), 50);

        state.parseCATCommand("MD2;"); // USB (voice)
        QCOMPARE(state.monitorLevelForCurrentMode(), 70);

        state.parseCATCommand("MD1;"); // LSB (voice)
        QCOMPARE(state.monitorLevelForCurrentMode(), 70);

        state.parseCATCommand("MD4;"); // FM (voice)
        QCOMPARE(state.monitorLevelForCurrentMode(), 70);

        state.parseCATCommand("MD7;"); // CW-R
        QCOMPARE(state.monitorLevelForCurrentMode(), 30);
    }

    // =========================================================================
    // monitorModeCode
    // =========================================================================
    void testMonitorModeCode() {
        RadioState state;

        state.parseCATCommand("MD3;"); // CW → 0
        QCOMPARE(state.monitorModeCode(), 0);

        state.parseCATCommand("MD7;"); // CW-R → 0
        QCOMPARE(state.monitorModeCode(), 0);

        state.parseCATCommand("MD6;"); // DATA → 1
        QCOMPARE(state.monitorModeCode(), 1);

        state.parseCATCommand("MD9;"); // DATA-R → 1
        QCOMPARE(state.monitorModeCode(), 1);

        state.parseCATCommand("MD2;"); // USB → 2 (voice)
        QCOMPARE(state.monitorModeCode(), 2);

        state.parseCATCommand("MD1;"); // LSB → 2
        QCOMPARE(state.monitorModeCode(), 2);

        state.parseCATCommand("MD4;"); // FM → 2
        QCOMPARE(state.monitorModeCode(), 2);

        state.parseCATCommand("MD5;"); // AM → 2
        QCOMPARE(state.monitorModeCode(), 2);
    }

    // =========================================================================
    // voxForCurrentMode
    // =========================================================================
    void testVoxForCurrentMode() {
        RadioState state;

        // VX format: VX{C|V|D}{0|1} — one mode at a time
        state.parseCATCommand("VXC1;"); // CW VOX on
        state.parseCATCommand("VXV1;"); // Voice VOX on
        // Data VOX stays off (default false)

        state.parseCATCommand("MD3;"); // CW
        QCOMPARE(state.voxForCurrentMode(), true); // voxCW

        state.parseCATCommand("MD2;"); // USB (voice)
        QCOMPARE(state.voxForCurrentMode(), true); // voxVoice

        state.parseCATCommand("MD6;"); // DATA
        QCOMPARE(state.voxForCurrentMode(), false); // voxData
    }

    // =========================================================================
    // Optimistic setters
    // =========================================================================
    void testSetKeyerSpeed() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::keyerSpeedChanged);

        state.setKeyerSpeed(25);
        QCOMPARE(state.keyerSpeed(), 25);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 25);
    }

    void testSetKeyerSpeed_noChangeNoSignal() {
        RadioState state;
        state.setKeyerSpeed(25);

        QSignalSpy spy(&state, &RadioState::keyerSpeedChanged);
        state.setKeyerSpeed(25); // Same value
        QCOMPARE(spy.count(), 0);
    }

    void testSetCwPitch() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::cwPitchChanged);

        state.setCwPitch(600);
        QCOMPARE(state.cwPitch(), 600);
        QCOMPARE(spy.count(), 1);
    }

    void testSetRfPower() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::rfPowerChanged);

        state.setRfPower(50.0);
        QCOMPARE(state.rfPower(), 50.0);
        QCOMPARE(spy.count(), 1);
    }

    // =========================================================================
    // KS command: Keyer Speed via CAT
    // =========================================================================
    void testKS_setsKeyerSpeed() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::keyerSpeedChanged);

        state.parseCATCommand("KS020;");
        QCOMPARE(state.keyerSpeed(), 20);
        QCOMPARE(spy.count(), 1);
    }

    // =========================================================================
    // IS command: IF Shift
    // =========================================================================
    void testIS_setsIfShift() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::ifShiftChanged);

        state.parseCATCommand("IS0050;");
        QCOMPARE(state.ifShift(), 50);
        QCOMPARE(spy.count(), 1);
    }

    // =========================================================================
    // FT command: Split
    // =========================================================================
    void testFT_enablesSplit() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::splitChanged);

        state.parseCATCommand("FT1;");
        QVERIFY(state.splitEnabled());
        QCOMPARE(spy.count(), 1);

        state.parseCATCommand("FT0;");
        QVERIFY(!state.splitEnabled());
    }

    // =========================================================================
    // Unknown command: no crash, no signal
    // =========================================================================
    void testUnknownCommand_noCrash() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::stateUpdated);

        // Completely unknown command
        state.parseCATCommand("ZZ99;");
        QCOMPARE(spy.count(), 0);
    }

    void testEmptyCommand_noCrash() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::stateUpdated);
        state.parseCATCommand("");
        QCOMPARE(spy.count(), 0);
    }

    void testSemicolonOnly_noCrash() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::stateUpdated);
        state.parseCATCommand(";");
        QCOMPARE(spy.count(), 0);
    }

    // =========================================================================
    // ML command: Monitor Level
    // =========================================================================
    void testML_setsMonitorLevel() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::monitorLevelChanged);

        // ML0050 → mode=0 (CW), level=50
        state.parseCATCommand("ML0050;");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 0); // mode
        QCOMPARE(spy.at(0).at(1).toInt(), 50); // level
        QCOMPARE(state.monitorLevelCW(), 50);
    }

    void testML_voiceMode() {
        RadioState state;
        // ML2075 → mode=2 (Voice), level=75
        state.parseCATCommand("ML2075;");
        QCOMPARE(state.monitorLevelVoice(), 75);
    }

    // =========================================================================
    // SB command: Sub Receiver
    // =========================================================================
    void testSB_togglesSubReceiver() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::subRxEnabledChanged);

        state.parseCATCommand("SB1;");
        QVERIFY(state.subReceiverEnabled());
        QCOMPARE(spy.count(), 1);
    }

    // =========================================================================
    // NB command: Noise Blanker
    // =========================================================================
    void testNB_parsesMainRx() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::processingChanged);

        // NB0512 → level=05, enabled=1, filter=2
        state.parseCATCommand("NB0512;");
        QCOMPARE(state.noiseBlankerLevel(), 5);
        QVERIFY(state.noiseBlankerEnabled());
        QCOMPARE(state.noiseBlankerFilterWidth(), 2);
        QCOMPARE(spy.count(), 1);
    }

    // =========================================================================
    // PA command: Preamp
    // =========================================================================
    void testPA_parsesPreamp() {
        RadioState state;
        state.parseCATCommand("PA21;"); // level=2, enabled=1
        QCOMPARE(state.preamp(), 2);
        QVERIFY(state.preampEnabled());
    }

    // =========================================================================
    // GT command: AGC Speed
    // =========================================================================
    void testGT_setsAgcSpeed() {
        RadioState state;
        state.parseCATCommand("GT2;"); // Fast
        QCOMPARE(state.agcSpeed(), RadioState::AGC_Fast);

        state.parseCATCommand("GT1;"); // Slow
        QCOMPARE(state.agcSpeed(), RadioState::AGC_Slow);

        state.parseCATCommand("GT0;"); // Off
        QCOMPARE(state.agcSpeed(), RadioState::AGC_Off);
    }

    // =========================================================================
    // CW command: CW Pitch
    // =========================================================================
    void testCW_setsPitch() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::cwPitchChanged);

        // CW060 → pitch code 60, Hz = 600
        state.parseCATCommand("CW060;");
        QCOMPARE(state.cwPitch(), 600);
        QCOMPARE(spy.count(), 1);
    }

    void testCW_outOfRange() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::cwPitchChanged);

        // Pitch code below 25 should be rejected
        state.parseCATCommand("CW010;");
        QCOMPARE(spy.count(), 0);
    }

    // =========================================================================
    // Display commands: #REF, #SPN, #SCL
    // =========================================================================
    void testDisplayREF_setsRefLevel() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::refLevelChanged);

        state.parseCATCommand("#REF-090;");
        QCOMPARE(state.refLevel(), -90);
        QCOMPARE(spy.count(), 1);
    }

    void testDisplaySPN_setsSpan() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::spanChanged);

        state.parseCATCommand("#SPN200000;");
        QCOMPARE(state.spanHz(), 200000);
        QCOMPARE(spy.count(), 1);
    }

    // =========================================================================
    // B SET toggle
    // =========================================================================
    void testBSet_toggle() {
        RadioState state;
        QVERIFY(!state.bSetEnabled());

        QSignalSpy spy(&state, &RadioState::bSetChanged);
        state.setBSetEnabled(true);
        QVERIFY(state.bSetEnabled());
        QCOMPARE(spy.count(), 1);

        state.toggleBSet();
        QVERIFY(!state.bSetEnabled());
        QCOMPARE(spy.count(), 2);
    }

    // =========================================================================
    // Optimistic setters: setFilterBandwidth, setIfShift
    // =========================================================================
    void testSetFilterBandwidth() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::filterBandwidthChanged);

        state.setFilterBandwidth(500);
        QCOMPARE(state.filterBandwidth(), 500);
        QCOMPARE(spy.count(), 1);
    }

    void testSetIfShift() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::ifShiftChanged);

        state.setIfShift(50);
        QCOMPARE(state.ifShift(), 50);
        QCOMPARE(spy.count(), 1);
    }

    // =========================================================================
    // PO command: Power Output Meter
    // =========================================================================
    void testPO_setPowerMeter() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::powerMeterChanged);

        state.parseCATCommand("PO075;");
        QCOMPARE(state.powerMeter(), 75);
        QCOMPARE(spy.count(), 1);
    }

    // =========================================================================
    // VX command: VOX enable
    // =========================================================================
    void testVX_setsVoxState() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::voxChanged);

        // VX format: VX{C|V|D}{0|1}
        state.parseCATCommand("VXC1;"); // CW VOX on
        QVERIFY(state.voxCW());
        QCOMPARE(spy.count(), 1);

        state.parseCATCommand("VXV0;"); // Voice VOX off (already default)
        QVERIFY(!state.voxVoice());

        state.parseCATCommand("VXD0;"); // Data VOX off (already default)
        QVERIFY(!state.voxData());
    }

    // =========================================================================
    // setDelayForCurrentMode
    // =========================================================================
    void testSetDelayForCurrentMode() {
        RadioState state;

        state.parseCATCommand("MD3;"); // CW
        state.setDelayForCurrentMode(50);
        QCOMPARE(state.delayForCurrentMode(), 50);

        state.parseCATCommand("MD2;"); // USB (voice)
        state.setDelayForCurrentMode(100);
        QCOMPARE(state.delayForCurrentMode(), 100);

        // Switch back to CW, delay should be 50
        state.parseCATCommand("MD3;");
        QCOMPARE(state.delayForCurrentMode(), 50);
    }

    void testSetDelayForCurrentMode_clamps() {
        RadioState state;
        state.parseCATCommand("MD3;"); // CW

        state.setDelayForCurrentMode(999); // Over max
        QCOMPARE(state.delayForCurrentMode(), 255);

        state.setDelayForCurrentMode(-5); // Below min
        QCOMPARE(state.delayForCurrentMode(), 0);
    }

    // =========================================================================
    // setScale / setSpanHz / setRefLevel optimistic setters
    // =========================================================================
    void testSetScale() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::scaleChanged);

        state.setScale(80);
        QCOMPARE(state.scale(), 80);
        QCOMPARE(spy.count(), 1);
    }

    void testSetScale_outOfRange() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::scaleChanged);

        state.setScale(5); // Below 10
        QCOMPARE(spy.count(), 0);

        state.setScale(200); // Above 150
        QCOMPARE(spy.count(), 0);
    }

    void testSetSpanHz() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::spanChanged);

        state.setSpanHz(200000);
        QCOMPARE(state.spanHz(), 200000);
        QCOMPARE(spy.count(), 1);
    }

    void testSetSpanHz_zeroIgnored() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::spanChanged);

        state.setSpanHz(0);
        QCOMPARE(spy.count(), 0);
    }

    // =========================================================================
    // MD$ command: Sub RX mode
    // =========================================================================
    void testMDSub_setsSubMode() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::modeBChanged);

        state.parseCATCommand("MD$3;");
        QCOMPARE(state.modeB(), RadioState::CW);
        QCOMPARE(spy.count(), 1);
    }

    // =========================================================================
    // FP command: Filter Position
    // =========================================================================
    void testFP_setsFilterPosition() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::filterPositionChanged);

        state.parseCATCommand("FP1;");
        QCOMPARE(state.filterPosition(), 1);
        QCOMPARE(spy.count(), 1);
    }

    // =========================================================================
    // Whitespace handling
    // =========================================================================
    void testParseCATCommand_trimmed() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::frequencyChanged);

        state.parseCATCommand("  FA00014060000;  ");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(state.frequency(), quint64(14060000));
    }

    // =========================================================================
    // SM$ command: Sub RX S-Meter
    // =========================================================================
    void testSMSub() {
        RadioState state;
        QSignalSpy spy(&state, &RadioState::sMeterBChanged);

        state.parseCATCommand("SM$10;"); // 10/2.0 = 5.0
        QCOMPARE(state.sMeterB(), 5.0);
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(TestRadioState)
#include "test_radiostate.moc"
