#include "mainwindow.h"
#include "ui/radiomanagerdialog.h"
#include "ui/sidecontrolpanel.h"
#include "ui/rightsidepanel.h"
#include "ui/bottommenubar.h"
#include "ui/featuremenubar.h"
#include "ui/modepopupwidget.h"
#include "ui/menuoverlay.h"
#include "ui/bandpopupwidget.h"
#include "ui/displaypopupwidget.h"
#include "ui/buttonrowpopup.h"
#include "ui/fnpopupwidget.h"
#include "ui/macrodialog.h"
#include "ui/optionsdialog.h"
#include "ui/txmeterwidget.h"
#include "ui/notificationwidget.h"
#include "ui/vforowwidget.h"
#include "models/menumodel.h"
#include "dsp/panadapter_rhi.h"
#include "dsp/minipan_rhi.h"
#include "audio/audioengine.h"
#include "audio/opusdecoder.h"
#include "audio/opusencoder.h"
#include "hardware/kpoddevice.h"
#include "network/kpa1500client.h"
#include "network/catserver.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>
#include <QFont>
#include <QDateTime>
#include <QPainter>
#include <QFrame>
#include <QEvent>
#include <QResizeEvent>
#include <QRegularExpression>
#include <QMouseEvent>
#include <QShowEvent>

// K4 Color scheme
namespace K4Colors {
const QString Background = "#1a1a1a";
const QString DarkBackground = "#0d0d0d";
const QString VfoAAmber = "#FFB000";
const QString VfoBCyan = "#00BFFF";
const QString TxRed = "#FF0000";
const QString AgcGreen = "#00FF00";
const QString InactiveGray = "#666666";
const QString TextWhite = "#FFFFFF";
const QString TextGray = "#999999";
const QString RitCyan = "#00CED1";
} // namespace K4Colors

// K4 Span range: 5 kHz to 368 kHz
// UP (zoom out): +1 kHz until 144, then +4 kHz until 368
// DOWN (zoom in): -4 kHz until 140, then -1 kHz until 5
static constexpr int SPAN_MIN = 5000;
static constexpr int SPAN_MAX = 368000;
static constexpr int SPAN_THRESHOLD_UP = 144000;   // Switch to 4kHz steps above this
static constexpr int SPAN_THRESHOLD_DOWN = 140000; // Switch to 1kHz steps below this

static int getNextSpanUp(int currentSpan) {
    if (currentSpan >= SPAN_MAX)
        return SPAN_MAX;
    int increment = (currentSpan < SPAN_THRESHOLD_UP) ? 1000 : 4000;
    int newSpan = currentSpan + increment;
    return qMin(newSpan, SPAN_MAX);
}

static int getNextSpanDown(int currentSpan) {
    if (currentSpan <= SPAN_MIN)
        return SPAN_MIN;
    int decrement = (currentSpan > SPAN_THRESHOLD_DOWN) ? 4000 : 1000;
    int newSpan = currentSpan - decrement;
    return qMax(newSpan, SPAN_MIN);
}

// ============== MainWindow Implementation ==============
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_tcpClient(new TcpClient(this)), m_radioState(new RadioState(this)),
      m_clockTimer(new QTimer(this)), m_audioEngine(new AudioEngine(this)), m_opusDecoder(new OpusDecoder(this)),
      m_opusEncoder(new OpusEncoder(this)), m_menuModel(new MenuModel(this)), m_menuOverlay(nullptr) {
    // Initialize Opus decoder (K4 sends 12kHz stereo: left=Main, right=Sub)
    m_opusDecoder->initialize(12000, 2);

    // Initialize Opus encoder for TX audio (12kHz mono)
    m_opusEncoder->initialize(12000, 1);

    // Load saved audio device settings
    QString savedMicDevice = RadioSettings::instance()->micDevice();
    if (!savedMicDevice.isEmpty()) {
        m_audioEngine->setMicDevice(savedMicDevice);
    }
    QString savedSpeakerDevice = RadioSettings::instance()->speakerDevice();
    if (!savedSpeakerDevice.isEmpty()) {
        m_audioEngine->setOutputDevice(savedSpeakerDevice);
    }
    m_audioEngine->setMicGain(RadioSettings::instance()->micGain() / 100.0f);

    // IMPORTANT: setupUi() MUST be called BEFORE setupMenuBar()!
    // Qt 6.10.1 bug on macOS Tahoe: calling menuBar() before creating QRhiWidget
    // prevents the RHI backing store from being set up correctly, causing
    // "QRhiWidget: No QRhi" errors and blank panadapter display.
    setupUi();
    setupMenuBar();

    // Menu items are populated from MEDF responses in onCatResponse()
    // when the radio sends RDY; after connection

    // Create menu overlay (positioned over spectrum container)
    m_menuOverlay = new MenuOverlayWidget(m_menuModel, this);
    m_menuOverlay->hide();

    // Connect menu overlay signals
    connect(m_menuOverlay, &MenuOverlayWidget::menuValueChangeRequested, this, &MainWindow::onMenuValueChangeRequested);
    connect(m_menuOverlay, &MenuOverlayWidget::closed, this, [this]() {
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setMenuActive(false);
        }
    });

    // Create band selection popup
    m_bandPopup = new BandPopupWidget(this);
    connect(m_bandPopup, &BandPopupWidget::bandSelected, this, &MainWindow::onBandSelected);
    connect(m_bandPopup, &BandPopupWidget::closed, this, [this]() {
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setBandActive(false);
        }
    });

    // Create display popup
    m_displayPopup = new DisplayPopupWidget(this);
    connect(m_displayPopup, &DisplayPopupWidget::closed, this, [this]() {
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setDisplayActive(false);
        }
    });
    // DisplayPopup pan mode changed -> update panadapter display
    // (K4 doesn't echo #DPM commands, so DisplayPopup notifies us directly)
    connect(m_displayPopup, &DisplayPopupWidget::dualPanModeChanged, this, [this](int mode) {
        switch (mode) {
        case 0: // A only
            setPanadapterMode(PanadapterMode::MainOnly);
            break;
        case 1: // B only
            setPanadapterMode(PanadapterMode::SubOnly);
            break;
        case 2: // Dual (A+B)
            setPanadapterMode(PanadapterMode::Dual);
            break;
        }
    });

    // DisplayPopup CAT commands -> TcpClient
    connect(m_displayPopup, &DisplayPopupWidget::catCommandRequested, m_tcpClient, &TcpClient::sendCAT);

    // Create Fn popup with dual-action buttons (macro system)
    m_fnPopup = new FnPopupWidget(this);
    connect(m_fnPopup, &FnPopupWidget::closed, this, [this]() {
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setFnActive(false);
        }
    });
    connect(m_fnPopup, &FnPopupWidget::functionTriggered, this, &MainWindow::onFnFunctionTriggered);

    // Create macro configuration dialog (full-screen overlay)
    m_macroDialog = new MacroDialog(this);
    m_macroDialog->hide();

    // Create button row popups for MAIN RX, SUB RX, TX

    m_mainRxPopup = new ButtonRowPopup(this);
    m_mainRxPopup->setButtonLabels({"1", "2", "3", "4", "5", "6", "7"});
    connect(m_mainRxPopup, &ButtonRowPopup::closed, this, [this]() {
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setMainRxActive(false);
        }
    });

    m_subRxPopup = new ButtonRowPopup(this);
    m_subRxPopup->setButtonLabels({"1", "2", "3", "4", "5", "6", "7"});
    connect(m_subRxPopup, &ButtonRowPopup::closed, this, [this]() {
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setSubRxActive(false);
        }
    });

    m_txPopup = new ButtonRowPopup(this);
    m_txPopup->setButtonLabels({"1", "2", "3", "4", "5", "6", "7"});
    connect(m_txPopup, &ButtonRowPopup::closed, this, [this]() {
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setTxActive(false);
        }
    });

    // Create notification popup for K4 error/status messages (ERxx:)
    m_notificationWidget = new NotificationWidget(this);

    // TcpClient signals
    connect(m_tcpClient, &TcpClient::stateChanged, this, &MainWindow::onStateChanged);
    connect(m_tcpClient, &TcpClient::errorOccurred, this, &MainWindow::onError);
    connect(m_tcpClient, &TcpClient::authenticated, this, &MainWindow::onAuthenticated);
    connect(m_tcpClient, &TcpClient::authenticationFailed, this, &MainWindow::onAuthenticationFailed);

    // Protocol CAT responses -> RadioState
    connect(m_tcpClient->protocol(), &Protocol::catResponseReceived, this, &MainWindow::onCatResponse);

    // RadioState signals -> UI updates (VFO A)
    connect(m_radioState, &RadioState::frequencyChanged, this, &MainWindow::onFrequencyChanged);
    connect(m_radioState, &RadioState::modeChanged, this, &MainWindow::onModeChanged);
    connect(m_radioState, &RadioState::modeChanged, this, [this](RadioState::Mode) {
        onVoxChanged(false); // Refresh VOX display when mode changes (VOX is mode-specific)
    });
    // Data sub-mode changes also update mode label (AFSK, FSK, PSK, DATA)
    connect(m_radioState, &RadioState::dataSubModeChanged, this,
            [this](int) { m_modeALabel->setText(m_radioState->modeStringFull()); });
    connect(m_radioState, &RadioState::sMeterChanged, this, &MainWindow::onSMeterChanged);
    connect(m_radioState, &RadioState::filterBandwidthChanged, this, &MainWindow::onBandwidthChanged);

    // RadioState signals -> UI updates (VFO B)
    connect(m_radioState, &RadioState::frequencyBChanged, this, &MainWindow::onFrequencyBChanged);
    connect(m_radioState, &RadioState::modeBChanged, this, &MainWindow::onModeBChanged);
    // Data sub-mode changes also update mode label (AFSK, FSK, PSK, DATA)
    connect(m_radioState, &RadioState::dataSubModeBChanged, this,
            [this](int) { m_modeBLabel->setText(m_radioState->modeStringFullB()); });
    connect(m_radioState, &RadioState::sMeterBChanged, this, &MainWindow::onSMeterBChanged);
    connect(m_radioState, &RadioState::filterBandwidthBChanged, this, &MainWindow::onBandwidthBChanged);

    // RadioState signals -> Status bar updates
    connect(m_radioState, &RadioState::rfPowerChanged, this, &MainWindow::onRfPowerChanged);
    connect(m_radioState, &RadioState::supplyVoltageChanged, this, &MainWindow::onSupplyVoltageChanged);
    connect(m_radioState, &RadioState::supplyCurrentChanged, this, &MainWindow::onSupplyCurrentChanged);
    connect(m_radioState, &RadioState::swrChanged, this, &MainWindow::onSwrChanged);

    // Error/notification messages from K4 (ERxx: format) -> show notification popup
    connect(m_radioState, &RadioState::errorNotificationReceived, this, &MainWindow::onErrorNotification);

    // TX Meter data -> update power displays and VFO multifunction meters during TX
    connect(m_radioState, &RadioState::txMeterChanged, this, [this](int alc, int comp, double fwdPower, double swr) {
        // Update status bar power label
        QString powerStr;
        if (fwdPower < 10.0) {
            powerStr = QString("%1 W").arg(fwdPower, 0, 'f', 1);
        } else {
            powerStr = QString("%1 W").arg(static_cast<int>(fwdPower));
        }
        m_powerLabel->setText(powerStr);
        // Update side panel power reading
        m_sideControlPanel->setPowerReading(fwdPower);

        // Calculate PA drain current (Id) from forward power and supply voltage
        // Formula: Id = ForwardPower / (Voltage × Efficiency)
        // K4 PA efficiency is approximately 34% (measured: 80W @ 17A @ 13.8V)
        double voltage = m_radioState->supplyVoltage();
        double paCurrent = 0.0;
        if (voltage > 0 && fwdPower > 0) {
            paCurrent = fwdPower / (voltage * 0.34);
        }

        // Update TX meters in VFO widgets (multifunction S/Po meters)
        // Both VFOs receive TX data - display based on which is transmitting
        m_vfoA->setTxMeters(alc, comp, fwdPower, swr);
        m_vfoA->setTxMeterCurrent(paCurrent);
        m_vfoB->setTxMeters(alc, comp, fwdPower, swr);
        m_vfoB->setTxMeterCurrent(paCurrent);
    });

    // TX state changes -> switch VFO meters between S-meter (RX) and Po (TX) mode
    // Also change TX indicator color to red when transmitting
    connect(m_radioState, &RadioState::transmitStateChanged, this, [this](bool transmitting) {
        // Both VFOs switch mode together - the active TX meter will show power
        m_vfoA->setTransmitting(transmitting);
        m_vfoB->setTransmitting(transmitting);

        // TX indicator and triangles turn red when transmitting
        QString color = transmitting ? "#FF0000" : K4Colors::VfoAAmber;
        m_txIndicator->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold;").arg(color));
        m_txTriangle->setStyleSheet(QString("color: %1; font-size: 18px;").arg(color));
        m_txTriangleB->setStyleSheet(QString("color: %1; font-size: 18px;").arg(color));
    });

    // SUB indicator - green when sub RX enabled, grey when off
    // Also updates DIV indicator since DIV requires SUB to be on
    connect(m_radioState, &RadioState::subRxEnabledChanged, this, [this](bool enabled) {
        if (enabled) {
            m_subLabel->setStyleSheet("background-color: #00FF00;"
                                      "color: black;"
                                      "font-size: 9px;"
                                      "font-weight: bold;"
                                      "border-radius: 2px;");
            // If DIV is also on, light up the DIV indicator (handles timing when SB3 comes after DV1)
            if (m_radioState->diversityEnabled()) {
                m_divLabel->setStyleSheet("background-color: #00FF00;"
                                          "color: black;"
                                          "font-size: 9px;"
                                          "font-weight: bold;"
                                          "border-radius: 2px;");
            }
        } else {
            m_subLabel->setStyleSheet("background-color: #444444;"
                                      "color: #888888;"
                                      "font-size: 9px;"
                                      "font-weight: bold;"
                                      "border-radius: 2px;");
            // DIV requires SUB - turn off DIV indicator when SUB is off
            m_divLabel->setStyleSheet("background-color: #444444;"
                                      "color: #888888;"
                                      "font-size: 9px;"
                                      "font-weight: bold;"
                                      "border-radius: 2px;");
        }
    });

    // DIV indicator - green only when BOTH diversity AND sub RX are enabled
    // (DIV requires SUB to be on - can't have DIV without SUB)
    connect(m_radioState, &RadioState::diversityChanged, this, [this](bool enabled) {
        // DIV only shows green if both diversity is enabled AND sub RX is enabled
        bool showActive = enabled && m_radioState->subReceiverEnabled();
        if (showActive) {
            m_divLabel->setStyleSheet("background-color: #00FF00;"
                                      "color: black;"
                                      "font-size: 9px;"
                                      "font-weight: bold;"
                                      "border-radius: 2px;");
        } else {
            m_divLabel->setStyleSheet("background-color: #444444;"
                                      "color: #888888;"
                                      "font-size: 9px;"
                                      "font-weight: bold;"
                                      "border-radius: 2px;");
        }
    });

    // NOTE: KPA1500 amplifier integration groundwork is in the KPA1500 section (after m_kpa1500Client creation)

    // RadioState signals -> Side control panel updates (BW/SHFT/HI/LO)
    // Helper to update all 4 filter display values (called on BW or SHFT change)
    // When B SET is enabled, shows VFO B (Sub RX) filter values instead of VFO A
    auto updateFilterDisplay = [this]() {
        bool bSet = m_radioState->bSetEnabled();

        // Get bandwidth and shift from correct VFO
        int bwHz = bSet ? m_radioState->filterBandwidthB() : m_radioState->filterBandwidth();
        int shiftHz = bSet ? m_radioState->shiftBHz() : m_radioState->shiftHz();

        // BW/SHFT in kHz
        m_sideControlPanel->setBandwidth(bwHz / 1000.0);
        m_sideControlPanel->setShift(shiftHz / 1000.0);

        // Calculate and set HI/LO in kHz
        // High = Shift + (Bandwidth / 2)
        // Low  = Shift - (Bandwidth / 2)
        int highHz = shiftHz + (bwHz / 2);
        int lowHz = shiftHz - (bwHz / 2);
        m_sideControlPanel->setHighCut(highHz / 1000.0);
        m_sideControlPanel->setLowCut(lowHz / 1000.0);
    };
    connect(m_radioState, &RadioState::filterBandwidthChanged, this, updateFilterDisplay);
    connect(m_radioState, &RadioState::ifShiftChanged, this, updateFilterDisplay);
    connect(m_radioState, &RadioState::filterBandwidthBChanged, this, updateFilterDisplay);
    connect(m_radioState, &RadioState::ifShiftBChanged, this, updateFilterDisplay);
    connect(m_radioState, &RadioState::bSetChanged, this, updateFilterDisplay);
    connect(m_radioState, &RadioState::keyerSpeedChanged, m_sideControlPanel, &SideControlPanel::setWpm);
    connect(m_radioState, &RadioState::cwPitchChanged, this, [this](int pitch) {
        m_sideControlPanel->setPitch(pitch / 1000.0); // Hz to kHz (500Hz = 0.50)
    });
    connect(m_radioState, &RadioState::rfPowerChanged, this,
            [this](double watts, bool) { m_sideControlPanel->setPower(watts); });
    connect(m_radioState, &RadioState::qskDelayChanged, this, [this](int delay) {
        m_sideControlPanel->setDelay(delay / 100.0); // 10ms units to seconds (20 -> 0.20)
    });
    connect(m_radioState, &RadioState::rfGainChanged, m_sideControlPanel, &SideControlPanel::setMainRfGain);
    connect(m_radioState, &RadioState::squelchChanged, m_sideControlPanel, &SideControlPanel::setMainSquelch);
    connect(m_radioState, &RadioState::rfGainBChanged, m_sideControlPanel, &SideControlPanel::setSubRfGain);
    connect(m_radioState, &RadioState::squelchBChanged, m_sideControlPanel, &SideControlPanel::setSubSquelch);
    connect(m_radioState, &RadioState::micGainChanged, m_sideControlPanel, &SideControlPanel::setMicGain);
    connect(m_radioState, &RadioState::compressionChanged, m_sideControlPanel, &SideControlPanel::setCompression);
    // Mode-dependent WPM/PTCH vs MIC/CMP display
    connect(m_radioState, &RadioState::modeChanged, this, [this](RadioState::Mode mode) {
        bool isCW = (mode == RadioState::CW || mode == RadioState::CW_R);
        m_sideControlPanel->setDisplayMode(isCW);
        // Refresh values after mode switch
        if (isCW) {
            m_sideControlPanel->setWpm(m_radioState->keyerSpeed());
            m_sideControlPanel->setPitch(m_radioState->cwPitch() / 1000.0);
        } else {
            m_sideControlPanel->setMicGain(m_radioState->micGain());
            m_sideControlPanel->setCompression(m_radioState->compression());
        }
    });

    // RadioState signals -> Center section updates
    connect(m_radioState, &RadioState::splitChanged, this, &MainWindow::onSplitChanged);
    connect(m_radioState, &RadioState::antennaChanged, this, &MainWindow::onAntennaChanged);
    connect(m_radioState, &RadioState::antennaNameChanged, this, &MainWindow::onAntennaNameChanged);
    connect(m_radioState, &RadioState::voxChanged, this, &MainWindow::onVoxChanged);
    connect(m_radioState, &RadioState::qskEnabledChanged, this, &MainWindow::onQskEnabledChanged);
    connect(m_radioState, &RadioState::testModeChanged, this, &MainWindow::onTestModeChanged);
    connect(m_radioState, &RadioState::atuModeChanged, this, &MainWindow::onAtuModeChanged);
    connect(m_radioState, &RadioState::ritXitChanged, this, &MainWindow::onRitXitChanged);
    connect(m_radioState, &RadioState::messageBankChanged, this, &MainWindow::onMessageBankChanged);

    // Filter position indicators
    connect(m_radioState, &RadioState::filterPositionChanged, this,
            [this](int pos) { m_filterALabel->setText(QString("FIL%1").arg(pos)); });
    connect(m_radioState, &RadioState::filterPositionBChanged, this,
            [this](int pos) { m_filterBLabel->setText(QString("FIL%1").arg(pos)); });

    // RadioState signals -> Processing state updates (AGC, PRE, ATT, NB, NR)
    connect(m_radioState, &RadioState::processingChanged, this, &MainWindow::onProcessingChanged);
    connect(m_radioState, &RadioState::processingChangedB, this, &MainWindow::onProcessingChangedB);

    // RadioState REF level -> Panadapter (for dynamic waterfall color scaling)
    connect(m_radioState, &RadioState::refLevelChanged, this, [this](int level) { m_panadapterA->setRefLevel(level); });
    connect(m_radioState, &RadioState::refLevelBChanged, this,
            [this](int level) { m_panadapterB->setRefLevel(level); });

    // RadioState scale -> Panadapter (for display gain/range adjustment)
    // Note: #SCL is a GLOBAL setting - applies to both panadapters (no #SCL$ variant exists)
    connect(m_radioState, &RadioState::scaleChanged, this, [this](int scale) {
        m_panadapterA->setScale(scale);
        m_panadapterB->setScale(scale);
    });

    // RadioState span -> Panadapter (for frequency labels and bin extraction)
    connect(m_radioState, &RadioState::spanChanged, this, [this](int spanHz) { m_panadapterA->setSpan(spanHz); });
    connect(m_radioState, &RadioState::spanBChanged, this, [this](int spanHz) { m_panadapterB->setSpan(spanHz); });

    // RadioState waterfall height -> Panadapter (global setting applies to both)
    connect(m_radioState, &RadioState::waterfallHeightChanged, this, [this](int percent) {
        m_panadapterA->setWaterfallHeight(percent);
        m_panadapterB->setWaterfallHeight(percent);
    });

    // RadioState display state -> DisplayPopup (for button face updates)
    // Separate LCD and EXT signals
    connect(m_radioState, &RadioState::dualPanModeLcdChanged, m_displayPopup, &DisplayPopupWidget::setDualPanModeLcd);
    connect(m_radioState, &RadioState::dualPanModeExtChanged, m_displayPopup, &DisplayPopupWidget::setDualPanModeExt);

    // RadioState dual pan mode -> Panadapter widget visibility
    // Sync app's panadapter display with radio's #DPM mode
    connect(m_radioState, &RadioState::dualPanModeLcdChanged, this, [this](int mode) {
        switch (mode) {
        case 0: // A only
            setPanadapterMode(PanadapterMode::MainOnly);
            break;
        case 1: // B only
            setPanadapterMode(PanadapterMode::SubOnly);
            break;
        case 2: // Dual (A+B)
            setPanadapterMode(PanadapterMode::Dual);
            break;
        }
    });
    connect(m_radioState, &RadioState::displayModeLcdChanged, m_displayPopup, &DisplayPopupWidget::setDisplayModeLcd);
    connect(m_radioState, &RadioState::displayModeExtChanged, m_displayPopup, &DisplayPopupWidget::setDisplayModeExt);
    connect(m_radioState, &RadioState::waterfallColorChanged, m_displayPopup, &DisplayPopupWidget::setWaterfallColor);
    connect(m_radioState, &RadioState::averagingChanged, m_displayPopup, &DisplayPopupWidget::setAveraging);
    connect(m_radioState, &RadioState::peakModeChanged, m_displayPopup, &DisplayPopupWidget::setPeakMode);
    connect(m_radioState, &RadioState::fixedTuneChanged, m_displayPopup, &DisplayPopupWidget::setFixedTuneMode);
    connect(m_radioState, &RadioState::freezeChanged, m_displayPopup, &DisplayPopupWidget::setFreeze);
    connect(m_radioState, &RadioState::vfoACursorChanged, m_displayPopup, &DisplayPopupWidget::setVfoACursor);
    connect(m_radioState, &RadioState::vfoBCursorChanged, m_displayPopup, &DisplayPopupWidget::setVfoBCursor);
    // VFO cursor visibility → panadapter passband indicator
    // Visible for ON (1) and AUTO (2), hidden for OFF (0) and HIDE (3)
    connect(m_radioState, &RadioState::vfoACursorChanged, this,
            [this](int mode) { m_panadapterA->setCursorVisible(mode == 1 || mode == 2); });
    connect(m_radioState, &RadioState::vfoBCursorChanged, this,
            [this](int mode) { m_panadapterB->setCursorVisible(mode == 1 || mode == 2); });
    connect(m_radioState, &RadioState::autoRefLevelChanged, m_displayPopup, &DisplayPopupWidget::setAutoRefLevel);
    connect(m_radioState, &RadioState::scaleChanged, m_displayPopup, &DisplayPopupWidget::setScale);
    connect(m_radioState, &RadioState::ddcNbModeChanged, m_displayPopup, &DisplayPopupWidget::setDdcNbMode);
    connect(m_radioState, &RadioState::ddcNbLevelChanged, m_displayPopup, &DisplayPopupWidget::setDdcNbLevel);
    connect(m_radioState, &RadioState::waterfallHeightChanged, m_displayPopup, &DisplayPopupWidget::setWaterfallHeight);
    connect(m_radioState, &RadioState::waterfallHeightExtChanged, m_displayPopup,
            &DisplayPopupWidget::setWaterfallHeightExt);
    // Also update span/ref values in popup
    connect(m_radioState, &RadioState::spanChanged, this, [this](int spanHz) {
        m_displayPopup->setSpanValueA(spanHz / 1000.0); // Hz to kHz
    });
    connect(m_radioState, &RadioState::spanBChanged, this, [this](int spanHz) {
        m_displayPopup->setSpanValueB(spanHz / 1000.0); // Hz to kHz
    });
    connect(m_radioState, &RadioState::refLevelChanged, m_displayPopup, &DisplayPopupWidget::setRefLevelValueA);
    connect(m_radioState, &RadioState::refLevelBChanged, m_displayPopup, &DisplayPopupWidget::setRefLevelValueB);

    // Averaging control +/- -> CAT commands (range 1-20, step by 1)
    connect(m_displayPopup, &DisplayPopupWidget::averagingIncrementRequested, this, [this]() {
        int current = m_radioState->averaging();
        int next = qMin(current + 1, 20);
        m_radioState->setAveraging(next); // Optimistic update
        m_tcpClient->sendCAT(QString("#AVG%1;").arg(next, 2, 10, QChar('0')));
    });
    connect(m_displayPopup, &DisplayPopupWidget::averagingDecrementRequested, this, [this]() {
        int current = m_radioState->averaging();
        int next = qMax(current - 1, 1);
        m_radioState->setAveraging(next); // Optimistic update
        m_tcpClient->sendCAT(QString("#AVG%1;").arg(next, 2, 10, QChar('0')));
    });

    // DDC NB level control +/- -> CAT commands
    connect(m_displayPopup, &DisplayPopupWidget::nbLevelIncrementRequested, this, [this]() {
        int current = m_radioState->ddcNbLevel();
        int next = qMin(current + 1, 14);
        m_tcpClient->sendCAT(QString("#NBL$%1;").arg(next, 2, 10, QChar('0')));
    });
    connect(m_displayPopup, &DisplayPopupWidget::nbLevelDecrementRequested, this, [this]() {
        int current = m_radioState->ddcNbLevel();
        int next = qMax(current - 1, 0);
        m_tcpClient->sendCAT(QString("#NBL$%1;").arg(next, 2, 10, QChar('0')));
    });

    // Waterfall height control +/- -> CAT commands (respects LCD/EXT selection)
    // LCD controls our app's panadapter, EXT is just for external HDMI display
    connect(m_displayPopup, &DisplayPopupWidget::waterfallHeightIncrementRequested, this, [this]() {
        bool isExt = m_displayPopup->isExtEnabled() && !m_displayPopup->isLcdEnabled();
        int current = isExt ? m_radioState->waterfallHeightExt() : m_radioState->waterfallHeight();
        int next = qMin(current + 1, 90); // 1% steps, max 90%
        QString cmd =
            isExt ? QString("#HWFH%1;").arg(next, 2, 10, QChar('0')) : QString("#WFH%1;").arg(next, 2, 10, QChar('0'));
        m_tcpClient->sendCAT(cmd);
        // Optimistically update RadioState and UI (K4 may not echo this command)
        if (!isExt) {
            m_radioState->setWaterfallHeight(next);
            m_panadapterA->setWaterfallHeight(next);
            m_panadapterB->setWaterfallHeight(next);
            m_displayPopup->setWaterfallHeight(next);
        } else {
            m_radioState->setWaterfallHeightExt(next);
            m_displayPopup->setWaterfallHeightExt(next);
        }
    });
    connect(m_displayPopup, &DisplayPopupWidget::waterfallHeightDecrementRequested, this, [this]() {
        bool isExt = m_displayPopup->isExtEnabled() && !m_displayPopup->isLcdEnabled();
        int current = isExt ? m_radioState->waterfallHeightExt() : m_radioState->waterfallHeight();
        int next = qMax(current - 1, 10); // 1% steps, min 10%
        QString cmd =
            isExt ? QString("#HWFH%1;").arg(next, 2, 10, QChar('0')) : QString("#WFH%1;").arg(next, 2, 10, QChar('0'));
        m_tcpClient->sendCAT(cmd);
        // Optimistically update RadioState and UI (K4 may not echo this command)
        if (!isExt) {
            m_radioState->setWaterfallHeight(next);
            m_panadapterA->setWaterfallHeight(next);
            m_panadapterB->setWaterfallHeight(next);
            m_displayPopup->setWaterfallHeight(next);
        } else {
            m_radioState->setWaterfallHeightExt(next);
            m_displayPopup->setWaterfallHeightExt(next);
        }
    });

    // Span control from display popup -> CAT commands (respects A/B selection)
    // Inverted controls: + zooms in (decrease span), - zooms out (increase span)
    connect(m_displayPopup, &DisplayPopupWidget::spanIncrementRequested, this, [this]() {
        bool vfoA = m_displayPopup->isVfoAEnabled();
        bool vfoB = m_displayPopup->isVfoBEnabled();
        int currentSpan = (vfoB && !vfoA) ? m_radioState->spanHzB() : m_radioState->spanHz();
        int newSpan = getNextSpanDown(currentSpan); // + zooms in
        if (newSpan != currentSpan) {
            if (vfoA) {
                m_radioState->setSpanHz(newSpan);
                m_tcpClient->sendCAT(QString("#SPN%1;").arg(newSpan));
            }
            if (vfoB) {
                m_radioState->setSpanHzB(newSpan);
                m_tcpClient->sendCAT(QString("#SPN$%1;").arg(newSpan));
            }
        }
    });
    connect(m_displayPopup, &DisplayPopupWidget::spanDecrementRequested, this, [this]() {
        bool vfoA = m_displayPopup->isVfoAEnabled();
        bool vfoB = m_displayPopup->isVfoBEnabled();
        int currentSpan = (vfoB && !vfoA) ? m_radioState->spanHzB() : m_radioState->spanHz();
        int newSpan = getNextSpanUp(currentSpan); // - zooms out
        if (newSpan != currentSpan) {
            if (vfoA) {
                m_radioState->setSpanHz(newSpan);
                m_tcpClient->sendCAT(QString("#SPN%1;").arg(newSpan));
            }
            if (vfoB) {
                m_radioState->setSpanHzB(newSpan);
                m_tcpClient->sendCAT(QString("#SPN$%1;").arg(newSpan));
            }
        }
    });

    // Scale control (GLOBAL - affects both panadapters, no A/B variants)
    connect(m_displayPopup, &DisplayPopupWidget::scaleIncrementRequested, this, [this]() {
        int currentScale = m_radioState->scale();
        if (currentScale < 0)
            currentScale = 75;                      // Default if not yet received
        int newScale = qMin(currentScale + 1, 150); // Increment by 1, max 150
        if (newScale != currentScale) {
            m_tcpClient->sendCAT(QString("#SCL%1;").arg(newScale));
        }
    });
    connect(m_displayPopup, &DisplayPopupWidget::scaleDecrementRequested, this, [this]() {
        int currentScale = m_radioState->scale();
        if (currentScale < 0)
            currentScale = 75;                     // Default if not yet received
        int newScale = qMax(currentScale - 1, 10); // Decrement by 1, min 10
        if (newScale != currentScale) {
            m_tcpClient->sendCAT(QString("#SCL%1;").arg(newScale));
        }
    });

    // Protocol spectrum data -> Panadapter
    connect(m_tcpClient->protocol(), &Protocol::spectrumDataReady, this, &MainWindow::onSpectrumData);
    connect(m_tcpClient->protocol(), &Protocol::miniSpectrumDataReady, this, &MainWindow::onMiniSpectrumData);

    // Protocol audio data -> Opus decoder -> Speaker
    connect(m_tcpClient->protocol(), &Protocol::audioDataReady, this, &MainWindow::onAudioData);

    // Clock timer for date/time display
    connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    m_clockTimer->start(1000);
    updateDateTime();

    // KPOD device
    m_kpodDevice = new KpodDevice(this);

    // Connect KPOD signals
    connect(m_kpodDevice, &KpodDevice::encoderRotated, this, &MainWindow::onKpodEncoderRotated);
    connect(m_kpodDevice, &KpodDevice::rockerPositionChanged, this,
            [this](KpodDevice::RockerPosition pos) { onKpodRockerChanged(static_cast<int>(pos)); });
    connect(m_kpodDevice, &KpodDevice::pollError, this, &MainWindow::onKpodPollError);

    // Connect KPOD button signals to macro execution
    connect(m_kpodDevice, &KpodDevice::buttonTapped, this, [this](int buttonNum) {
        QString functionId = QString("K-pod.%1T").arg(buttonNum);
        executeMacro(functionId);
    });
    connect(m_kpodDevice, &KpodDevice::buttonHeld, this, [this](int buttonNum) {
        QString functionId = QString("K-pod.%1H").arg(buttonNum);
        executeMacro(functionId);
    });

    // Connect KPOD hotplug signals - auto-start polling when device arrives
    connect(m_kpodDevice, &KpodDevice::deviceConnected, this, [this]() {
        qDebug() << "KPOD: Device arrived via hotplug";
        if (RadioSettings::instance()->kpodEnabled() && !m_kpodDevice->isPolling()) {
            qDebug() << "KPOD: Auto-starting polling (enabled in settings)";
            m_kpodDevice->startPolling();
        }
    });
    connect(m_kpodDevice, &KpodDevice::deviceDisconnected, this,
            [this]() { qDebug() << "KPOD: Device removed via hotplug"; });

    // Connect to settings for KPOD enable/disable
    connect(RadioSettings::instance(), &RadioSettings::kpodEnabledChanged, this, &MainWindow::onKpodEnabledChanged);

    // Start KPOD polling if enabled and detected
    if (RadioSettings::instance()->kpodEnabled() && m_kpodDevice->isDetected()) {
        m_kpodDevice->startPolling();
    }

    // KPA1500 amplifier client
    m_kpa1500Client = new KPA1500Client(this);

    // Connect KPA1500 signals
    connect(m_kpa1500Client, &KPA1500Client::connected, this, &MainWindow::onKpa1500Connected);
    connect(m_kpa1500Client, &KPA1500Client::disconnected, this, &MainWindow::onKpa1500Disconnected);
    connect(m_kpa1500Client, &KPA1500Client::errorOccurred, this, &MainWindow::onKpa1500Error);

    // Connect to settings for KPA1500 enable/disable and settings changes
    connect(RadioSettings::instance(), &RadioSettings::kpa1500EnabledChanged, this,
            &MainWindow::onKpa1500EnabledChanged);
    connect(RadioSettings::instance(), &RadioSettings::kpa1500SettingsChanged, this,
            &MainWindow::onKpa1500SettingsChanged);

    // Start KPA1500 connection if enabled and configured
    if (RadioSettings::instance()->kpa1500Enabled() && !RadioSettings::instance()->kpa1500Host().isEmpty()) {
        m_kpa1500Client->connectToHost(RadioSettings::instance()->kpa1500Host(),
                                       RadioSettings::instance()->kpa1500Port());
    }

    // Initialize KPA1500 status display
    updateKpa1500Status();

    // CAT server for external app integration (WSJT-X, MacLoggerDX, etc.)
    // Apps connect using their built-in K4 support - no protocol translation needed
    m_catServer = new CatServer(m_radioState, this);
    m_catServer->setTcpClient(m_tcpClient);

    // Forward CAT commands from external apps to the real K4
    connect(m_catServer, &CatServer::catCommandReceived, this,
            [this](const QString &command) { m_tcpClient->sendCAT(command); });

    // TX;/RX; from external apps controls audio input gate
    // Audio stream itself triggers K4 TX - timing-critical for FT8/FT4
    connect(m_catServer, &CatServer::pttRequested, this, [this](bool on) {
        m_pttActive = on;
        if (on) {
            m_txSequence = 0;
        }
        m_audioEngine->setMicEnabled(on);
        m_bottomMenuBar->setPttActive(on);
    });

    // Connect to settings for CAT server enable/disable
    connect(RadioSettings::instance(), &RadioSettings::rigctldEnabledChanged, this, [this](bool enabled) {
        if (enabled) {
            m_catServer->start(RadioSettings::instance()->rigctldPort());
        } else {
            m_catServer->stop();
        }
    });
    connect(RadioSettings::instance(), &RadioSettings::rigctldPortChanged, this, [this](quint16 port) {
        if (RadioSettings::instance()->rigctldEnabled()) {
            m_catServer->stop();
            m_catServer->start(port);
        }
    });

    // Start CAT server if enabled
    if (RadioSettings::instance()->rigctldEnabled()) {
        m_catServer->start(RadioSettings::instance()->rigctldPort());
    }

    // resize directly instead of deferring - testing if deferred resize affects QRhi
    // QTimer::singleShot(0, this, [this]() { resize(1340, 800); });
}

MainWindow::~MainWindow() {}

void MainWindow::setupMenuBar() {
    // Standard menu bar order: File, Connect, Tools, View, Help
    // On macOS, Qt automatically creates the app menu with About/Preferences
    menuBar()->setStyleSheet(QString("QMenuBar { background-color: %1; color: %2; }"
                                     "QMenuBar::item:selected { background-color: #333; }")
                                 .arg(K4Colors::DarkBackground, K4Colors::TextWhite));

    // File menu (first, per Windows convention)
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *quitAction = new QAction("E&xit", this);
    quitAction->setMenuRole(QAction::QuitRole); // macOS: moves to app menu
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(quitAction);

    // Connect menu
    QMenu *connectMenu = menuBar()->addMenu("&Connect");
    QAction *radiosAction = new QAction("&Radios...", this);
    connect(radiosAction, &QAction::triggered, this, &MainWindow::showRadioManager);
    connectMenu->addAction(radiosAction);

    // Tools menu
    QMenu *toolsMenu = menuBar()->addMenu("&Tools");
    QAction *optionsAction = new QAction("&Settings...", this);
    optionsAction->setMenuRole(QAction::PreferencesRole); // macOS: moves to app menu as Preferences
    connect(optionsAction, &QAction::triggered, this, [this]() {
        OptionsDialog dialog(m_radioState, m_kpa1500Client, m_audioEngine, m_kpodDevice, m_catServer, this);
        dialog.exec();
    });
    toolsMenu->addAction(optionsAction);

    // View menu
    QMenu *viewMenu = menuBar()->addMenu("&View");
    Q_UNUSED(viewMenu)

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *aboutAction = new QAction("&About K4Controller", this);
    aboutAction->setMenuRole(QAction::AboutRole); // macOS: moves to app menu
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(
            this, "About K4Controller",
            QString("<h2>K4Controller</h2>"
                    "<p>Version %1</p>"
                    "<p>Remote control application for Elecraft K4 radios.</p>"
                    "<p>Copyright &copy; 2024-2025 AI5QK</p>"
                    "<p><a href='https://github.com/mikeg-dal/K4Controller'>github.com/mikeg-dal/K4Controller</a></p>")
                .arg(QCoreApplication::applicationVersion()));
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::setupUi() {
    setWindowTitle("K4Controller");
    setMinimumSize(1340, 800);
    resize(1340, 800); // Default to minimum size on launch

    // NOTE: Do NOT set WA_NativeWindow here!
    // Qt 6.10.1 bug on macOS Tahoe: WA_NativeWindow forces native window creation
    // before QRhiWidget can configure it for MetalSurface, causing
    // "QMetalSwapChain only supports MetalSurface windows" crash.

    setStyleSheet(QString("QMainWindow { background-color: %1; }").arg(K4Colors::Background));

    auto *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet(QString("background-color: %1;").arg(K4Colors::Background));
    setCentralWidget(centralWidget);

    // Main vertical layout
    auto *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Top status bar
    setupTopStatusBar(centralWidget);
    mainLayout->addWidget(m_titleLabel->parentWidget());

    // Middle section: Side Panel + Main Content (L-shaped)
    auto *middleWidget = new QWidget(centralWidget);
    auto *middleLayout = new QHBoxLayout(middleWidget);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(0);

    // Side Control Panel (left)
    m_sideControlPanel = new SideControlPanel(middleWidget);
    middleLayout->addWidget(m_sideControlPanel);

    // Main content (VFO + Spectrum)
    auto *contentWidget = new QWidget(middleWidget);
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(4, 4, 4, 4);
    contentLayout->setSpacing(2);

    // VFO section (A | Center | B)
    auto *vfoWidget = new QWidget(contentWidget);
    setupVfoSection(vfoWidget);
    contentLayout->addWidget(vfoWidget);

    // Spectrum/Waterfall display
    setupSpectrumPlaceholder(contentWidget);
    contentLayout->addWidget(m_spectrumContainer, 1);

    middleLayout->addWidget(contentWidget, 1);

    // Right Side Panel (mirrors left panel dimensions)
    m_rightSidePanel = new RightSidePanel(middleWidget);
    middleLayout->addWidget(m_rightSidePanel);

    mainLayout->addWidget(middleWidget, 1);

    // Feature Menu Bar (popup, positioned above bottom menu bar when shown)
    m_featureMenuBar = new FeatureMenuBar(this);

    // Feature Menu Bar CAT commands - send appropriate command based on current feature
    connect(m_featureMenuBar, &FeatureMenuBar::toggleRequested, this, [this]() {
        bool bSet = m_radioState->bSetEnabled();
        switch (m_featureMenuBar->currentFeature()) {
        case FeatureMenuBar::Attenuator: {
            // Optimistic UI update - flip enabled state (use Sub RX state if B SET)
            bool newState = bSet ? !m_radioState->attenuatorEnabledB() : !m_radioState->attenuatorEnabled();
            m_featureMenuBar->setFeatureEnabled(newState);
            m_tcpClient->sendCAT(bSet ? "RA$/;" : "RA/;");
            break;
        }
        case FeatureMenuBar::NbLevel: {
            // Toggle NB on/off
            bool curState = bSet ? m_radioState->noiseBlankerEnabledB() : m_radioState->noiseBlankerEnabled();
            m_featureMenuBar->setFeatureEnabled(!curState);
            m_tcpClient->sendCAT(bSet ? "NB$/;" : "NB/;");
            break;
        }
        case FeatureMenuBar::NrAdjust: {
            bool newState = bSet ? !m_radioState->noiseReductionEnabledB() : !m_radioState->noiseReductionEnabled();
            m_featureMenuBar->setFeatureEnabled(newState);
            m_tcpClient->sendCAT(bSet ? "NR$/;" : "NR/;");
            break;
        }
        case FeatureMenuBar::ManualNotch: {
            // Toggle notch on/off for correct VFO
            bool curState = bSet ? m_radioState->manualNotchEnabledB() : m_radioState->manualNotchEnabled();
            m_featureMenuBar->setFeatureEnabled(!curState);
            m_tcpClient->sendCAT(bSet ? "NM$/;" : "NM/;");
            break;
        }
        }
    });
    connect(m_featureMenuBar, &FeatureMenuBar::incrementRequested, this, [this]() {
        bool bSet = m_radioState->bSetEnabled();
        switch (m_featureMenuBar->currentFeature()) {
        case FeatureMenuBar::Attenuator: {
            // Optimistic: show next step (RA+ adds 3dB, max 21)
            int curLevel = bSet ? m_radioState->attenuatorLevelB() : m_radioState->attenuatorLevel();
            int newLevel = qMin(curLevel + 3, 21);
            m_featureMenuBar->setValue(newLevel);
            m_tcpClient->sendCAT(bSet ? "RA$+;" : "RA+;");
            break;
        }
        case FeatureMenuBar::NbLevel: {
            int curLevel = bSet ? m_radioState->noiseBlankerLevelB() : m_radioState->noiseBlankerLevel();
            int newLevel = qMin(curLevel + 1, 15);
            int enabled =
                bSet ? (m_radioState->noiseBlankerEnabledB() ? 1 : 0) : (m_radioState->noiseBlankerEnabled() ? 1 : 0);
            int filter = bSet ? m_radioState->noiseBlankerFilterWidthB() : m_radioState->noiseBlankerFilterWidth();
            // Optimistic state + UI update
            if (bSet) {
                m_radioState->setNoiseBlankerLevelB(newLevel);
            } else {
                m_radioState->setNoiseBlankerLevel(newLevel);
            }
            m_featureMenuBar->setValue(newLevel);
            QString prefix = bSet ? "NB$" : "NB";
            QString cmd = QString("%1%2%3%4;").arg(prefix).arg(newLevel, 2, 10, QChar('0')).arg(enabled).arg(filter);
            m_tcpClient->sendCAT(cmd);
            break;
        }
        case FeatureMenuBar::NrAdjust: {
            int curLevel = bSet ? m_radioState->noiseReductionLevelB() : m_radioState->noiseReductionLevel();
            int newLevel = qMin(curLevel + 1, 10);
            int enabled = bSet ? (m_radioState->noiseReductionEnabledB() ? 1 : 0)
                               : (m_radioState->noiseReductionEnabled() ? 1 : 0);
            // Optimistic state + UI update
            if (bSet) {
                m_radioState->setNoiseReductionLevelB(newLevel);
            } else {
                m_radioState->setNoiseReductionLevel(newLevel);
            }
            m_featureMenuBar->setValue(newLevel);
            QString prefix = bSet ? "NR$" : "NR";
            QString cmd = QString("%1%2%3;").arg(prefix).arg(newLevel, 2, 10, QChar('0')).arg(enabled);
            m_tcpClient->sendCAT(cmd);
            break;
        }
        case FeatureMenuBar::ManualNotch: {
            // Use correct VFO's pitch state
            int curPitch = bSet ? m_radioState->manualNotchPitchB() : m_radioState->manualNotchPitch();
            int newPitch = qMin(curPitch + 10, 5000);
            int enabled =
                bSet ? (m_radioState->manualNotchEnabledB() ? 1 : 0) : (m_radioState->manualNotchEnabled() ? 1 : 0);
            // Optimistic state + UI update
            if (bSet) {
                m_radioState->setManualNotchPitchB(newPitch);
            } else {
                m_radioState->setManualNotchPitch(newPitch);
            }
            m_featureMenuBar->setValue(newPitch);
            QString prefix = bSet ? "NM$" : "NM";
            m_tcpClient->sendCAT(QString("%1%2%3;").arg(prefix).arg(newPitch, 4, 10, QChar('0')).arg(enabled));
            break;
        }
        }
    });
    connect(m_featureMenuBar, &FeatureMenuBar::decrementRequested, this, [this]() {
        bool bSet = m_radioState->bSetEnabled();
        switch (m_featureMenuBar->currentFeature()) {
        case FeatureMenuBar::Attenuator: {
            // Optimistic: show next step (RA- subtracts 3dB, min 0)
            int curLevel = bSet ? m_radioState->attenuatorLevelB() : m_radioState->attenuatorLevel();
            int newLevel = qMax(curLevel - 3, 0);
            m_featureMenuBar->setValue(newLevel);
            m_tcpClient->sendCAT(bSet ? "RA$-;" : "RA-;");
            break;
        }
        case FeatureMenuBar::NbLevel: {
            int curLevel = bSet ? m_radioState->noiseBlankerLevelB() : m_radioState->noiseBlankerLevel();
            int newLevel = qMax(curLevel - 1, 0);
            int enabled =
                bSet ? (m_radioState->noiseBlankerEnabledB() ? 1 : 0) : (m_radioState->noiseBlankerEnabled() ? 1 : 0);
            int filter = bSet ? m_radioState->noiseBlankerFilterWidthB() : m_radioState->noiseBlankerFilterWidth();
            // Optimistic state + UI update
            if (bSet) {
                m_radioState->setNoiseBlankerLevelB(newLevel);
            } else {
                m_radioState->setNoiseBlankerLevel(newLevel);
            }
            m_featureMenuBar->setValue(newLevel);
            QString prefix = bSet ? "NB$" : "NB";
            m_tcpClient->sendCAT(
                QString("%1%2%3%4;").arg(prefix).arg(newLevel, 2, 10, QChar('0')).arg(enabled).arg(filter));
            break;
        }
        case FeatureMenuBar::NrAdjust: {
            int curLevel = bSet ? m_radioState->noiseReductionLevelB() : m_radioState->noiseReductionLevel();
            int newLevel = qMax(curLevel - 1, 0);
            int enabled = bSet ? (m_radioState->noiseReductionEnabledB() ? 1 : 0)
                               : (m_radioState->noiseReductionEnabled() ? 1 : 0);
            // Optimistic state + UI update
            if (bSet) {
                m_radioState->setNoiseReductionLevelB(newLevel);
            } else {
                m_radioState->setNoiseReductionLevel(newLevel);
            }
            m_featureMenuBar->setValue(newLevel);
            QString prefix = bSet ? "NR$" : "NR";
            m_tcpClient->sendCAT(QString("%1%2%3;").arg(prefix).arg(newLevel, 2, 10, QChar('0')).arg(enabled));
            break;
        }
        case FeatureMenuBar::ManualNotch: {
            // Use correct VFO's pitch state
            int curPitch = bSet ? m_radioState->manualNotchPitchB() : m_radioState->manualNotchPitch();
            int newPitch = qMax(curPitch - 10, 150);
            int enabled =
                bSet ? (m_radioState->manualNotchEnabledB() ? 1 : 0) : (m_radioState->manualNotchEnabled() ? 1 : 0);
            // Optimistic state + UI update
            if (bSet) {
                m_radioState->setManualNotchPitchB(newPitch);
            } else {
                m_radioState->setManualNotchPitch(newPitch);
            }
            m_featureMenuBar->setValue(newPitch);
            QString prefix = bSet ? "NM$" : "NM";
            m_tcpClient->sendCAT(QString("%1%2%3;").arg(prefix).arg(newPitch, 4, 10, QChar('0')).arg(enabled));
            break;
        }
        }
    });
    connect(m_featureMenuBar, &FeatureMenuBar::extraButtonClicked, this, [this]() {
        // Extra button cycles NB filter: NONE(0) -> NARROW(1) -> WIDE(2) -> NONE(0)
        if (m_featureMenuBar->currentFeature() == FeatureMenuBar::NbLevel) {
            bool bSet = m_radioState->bSetEnabled();
            int curFilter = bSet ? m_radioState->noiseBlankerFilterWidthB() : m_radioState->noiseBlankerFilterWidth();
            int newFilter = (curFilter + 1) % 3;
            int level = bSet ? m_radioState->noiseBlankerLevelB() : m_radioState->noiseBlankerLevel();
            int enabled =
                bSet ? (m_radioState->noiseBlankerEnabledB() ? 1 : 0) : (m_radioState->noiseBlankerEnabled() ? 1 : 0);
            // Optimistic state + UI update
            if (bSet) {
                m_radioState->setNoiseBlankerFilterB(newFilter);
            } else {
                m_radioState->setNoiseBlankerFilter(newFilter);
            }
            m_featureMenuBar->setNbFilter(newFilter);
            QString prefix = bSet ? "NB$" : "NB";
            m_tcpClient->sendCAT(
                QString("%1%2%3%4;").arg(prefix).arg(level, 2, 10, QChar('0')).arg(enabled).arg(newFilter));
        }
    });

    // Update feature menu bar from RadioState - helper lambda
    auto updateFeatureMenuBarState = [this]() {
        if (!m_featureMenuBar->isMenuVisible())
            return;
        bool bSet = m_radioState->bSetEnabled();
        switch (m_featureMenuBar->currentFeature()) {
        case FeatureMenuBar::Attenuator:
            if (bSet) {
                m_featureMenuBar->setFeatureEnabled(m_radioState->attenuatorEnabledB());
                m_featureMenuBar->setValue(m_radioState->attenuatorLevelB());
            } else {
                m_featureMenuBar->setFeatureEnabled(m_radioState->attenuatorEnabled());
                m_featureMenuBar->setValue(m_radioState->attenuatorLevel());
            }
            break;
        case FeatureMenuBar::NbLevel:
            if (bSet) {
                m_featureMenuBar->setFeatureEnabled(m_radioState->noiseBlankerEnabledB());
                m_featureMenuBar->setValue(m_radioState->noiseBlankerLevelB());
                m_featureMenuBar->setNbFilter(m_radioState->noiseBlankerFilterWidthB());
            } else {
                m_featureMenuBar->setFeatureEnabled(m_radioState->noiseBlankerEnabled());
                m_featureMenuBar->setValue(m_radioState->noiseBlankerLevel());
                m_featureMenuBar->setNbFilter(m_radioState->noiseBlankerFilterWidth());
            }
            break;
        case FeatureMenuBar::NrAdjust:
            if (bSet) {
                m_featureMenuBar->setFeatureEnabled(m_radioState->noiseReductionEnabledB());
                m_featureMenuBar->setValue(m_radioState->noiseReductionLevelB());
            } else {
                m_featureMenuBar->setFeatureEnabled(m_radioState->noiseReductionEnabled());
                m_featureMenuBar->setValue(m_radioState->noiseReductionLevel());
            }
            break;
        case FeatureMenuBar::ManualNotch:
            // Use correct VFO's notch state
            if (bSet) {
                m_featureMenuBar->setFeatureEnabled(m_radioState->manualNotchEnabledB());
                m_featureMenuBar->setValue(m_radioState->manualNotchPitchB());
            } else {
                m_featureMenuBar->setFeatureEnabled(m_radioState->manualNotchEnabled());
                m_featureMenuBar->setValue(m_radioState->manualNotchPitch());
            }
            break;
        }
    };
    // Connect both Main and Sub RX processing changes
    connect(m_radioState, &RadioState::processingChanged, this, updateFeatureMenuBarState);
    connect(m_radioState, &RadioState::processingChangedB, this, updateFeatureMenuBarState);
    connect(m_radioState, &RadioState::notchChanged, this, updateFeatureMenuBarState);
    connect(m_radioState, &RadioState::notchBChanged, this, updateFeatureMenuBarState);
    // Also update when B SET changes to refresh display with correct VFO's state
    connect(m_radioState, &RadioState::bSetChanged, this, updateFeatureMenuBarState);

    // Mode Popup Widget (popup, positioned above bottom menu bar when shown)
    m_modePopup = new ModePopupWidget(this);
    connect(m_modePopup, &ModePopupWidget::modeSelected, this, [this](const QString &catCmd) {
        // Send the command to the radio
        m_tcpClient->sendCAT(catCmd);

        // Optimistically update data sub-mode (K4 doesn't echo DT SET commands)
        // Parse DT or DT$ from command like "MD6;DT1;" or "MD$6;DT$3;"
        QRegularExpression dtRegex("DT(\\$?)(\\d)");
        QRegularExpressionMatch match = dtRegex.match(catCmd);
        if (match.hasMatch()) {
            bool isSubRx = !match.captured(1).isEmpty(); // DT$ = Sub RX
            int subMode = match.captured(2).toInt();
            qDebug() << "Optimistic DT update: isSubRx=" << isSubRx << "subMode=" << subMode;
            if (isSubRx) {
                m_radioState->setDataSubModeB(subMode);
            } else {
                m_radioState->setDataSubMode(subMode);
            }
        }
    });
    // Update mode popup when mode changes - use A or B based on B SET state
    connect(m_radioState, &RadioState::modeChanged, this, [this](RadioState::Mode mode) {
        if (!m_radioState->bSetEnabled()) {
            m_modePopup->setCurrentMode(static_cast<int>(mode));
        }
    });
    connect(m_radioState, &RadioState::modeBChanged, this, [this](RadioState::Mode mode) {
        if (m_radioState->bSetEnabled()) {
            m_modePopup->setCurrentMode(static_cast<int>(mode));
        }
    });
    connect(m_radioState, &RadioState::dataSubModeChanged, this, [this](int subMode) {
        if (!m_radioState->bSetEnabled()) {
            m_modePopup->setCurrentDataSubMode(subMode);
        }
    });
    connect(m_radioState, &RadioState::dataSubModeBChanged, this, [this](int subMode) {
        if (m_radioState->bSetEnabled()) {
            m_modePopup->setCurrentDataSubMode(subMode);
        }
    });
    // Update B SET state for mode popup - also refresh mode/submode/frequency display
    connect(m_radioState, &RadioState::bSetChanged, this, [this](bool enabled) {
        m_modePopup->setBSetEnabled(enabled);
        // Update displayed mode and frequency to match the new target VFO
        if (enabled) {
            m_modePopup->setFrequency(m_radioState->vfoB());
            m_modePopup->setCurrentMode(static_cast<int>(m_radioState->modeB()));
            m_modePopup->setCurrentDataSubMode(m_radioState->dataSubModeB());
        } else {
            m_modePopup->setFrequency(m_radioState->vfoA());
            m_modePopup->setCurrentMode(static_cast<int>(m_radioState->mode()));
            m_modePopup->setCurrentDataSubMode(m_radioState->dataSubMode());
        }
    });

    // B SET indicator visibility and side panel indicator color
    connect(m_radioState, &RadioState::bSetChanged, this, [this](bool enabled) {
        qDebug() << "B SET changed:" << enabled;
        // Show/hide B SET indicator (hide SPLIT when B SET active)
        m_bSetLabel->setVisible(enabled);
        m_splitLabel->setVisible(!enabled);

        // Change side panel BW/SHFT indicator color (cyan=MainRx, green=SubRx)
        m_sideControlPanel->setActiveReceiver(enabled);
    });

    // Bottom Menu Bar
    m_bottomMenuBar = new BottomMenuBar(centralWidget);
    mainLayout->addWidget(m_bottomMenuBar);

    // Connect side control panel icon buttons
    connect(m_sideControlPanel, &SideControlPanel::connectClicked, this, &MainWindow::showRadioManager);
    connect(m_sideControlPanel, &SideControlPanel::helpClicked, this, []() {
        // TODO: Show help dialog
    });

    // Connect volume slider to OpusDecoder (Main RX / VFO A)
    connect(m_sideControlPanel, &SideControlPanel::volumeChanged, this, [this](int value) {
        if (m_opusDecoder) {
            m_opusDecoder->setMainVolume(value / 100.0f);
        }
        RadioSettings::instance()->setVolume(value); // Persist setting
    });

    // Connect sub volume slider to OpusDecoder (Sub RX / VFO B)
    connect(m_sideControlPanel, &SideControlPanel::subVolumeChanged, this, [this](int value) {
        if (m_opusDecoder) {
            m_opusDecoder->setSubVolume(value / 100.0f);
        }
        RadioSettings::instance()->setSubVolume(value); // Persist setting
    });

    // Connect side control panel scroll signals to CAT commands
    // After sending CAT, update RadioState optimistically (radio doesn't echo these commands)
    // Group 1: WPM/PTCH (CW mode) and MIC/CMP (Voice mode)
    connect(m_sideControlPanel, &SideControlPanel::wpmChanged, this, [this](int delta) {
        int newWpm = qBound(8, m_radioState->keyerSpeed() + delta, 50);
        m_tcpClient->sendCAT(QString("KS%1;").arg(newWpm, 3, 10, QChar('0')));
        m_radioState->setKeyerSpeed(newWpm);
    });
    connect(m_sideControlPanel, &SideControlPanel::pitchChanged, this, [this](int delta) {
        int currentPitch = m_radioState->cwPitch(); // In Hz
        int newPitch = qBound(300, currentPitch + (delta * 10), 990);
        m_tcpClient->sendCAT(QString("CW%1;").arg(newPitch / 10, 2, 10, QChar('0')));
        m_radioState->setCwPitch(newPitch);
    });
    connect(m_sideControlPanel, &SideControlPanel::micGainChanged, this, [this](int delta) {
        int newGain = qBound(0, m_radioState->micGain() + delta, 80);
        m_tcpClient->sendCAT(QString("MG%1;").arg(newGain, 3, 10, QChar('0')));
        m_radioState->setMicGain(newGain);
    });
    connect(m_sideControlPanel, &SideControlPanel::compressionChanged, this, [this](int delta) {
        int newComp = qBound(0, m_radioState->compression() + delta, 30);
        m_tcpClient->sendCAT(QString("CP%1;").arg(newComp, 3, 10, QChar('0')));
        m_radioState->setCompression(newComp);
    });
    // Group 1: PWR/DLY
    // PC command uses PCnnnr; format: L=QRP (0.1-10W), H=QRO (11-110W)
    // QRP (≤10W): 0.1W increments, e.g., 10.0, 9.9, 9.8, ... 0.1
    // QRO (>10W): 1W increments, e.g., 11, 12, 13, ... 110
    connect(m_sideControlPanel, &SideControlPanel::powerChanged, this, [this](int delta) {
        double currentPower = m_radioState->rfPower();
        double newPower;

        if (currentPower <= 10.0) {
            // Currently in QRP range: 0.1W increments
            newPower = currentPower + (delta * 0.1);
            if (newPower > 10.0) {
                // Transition to QRO at 11W
                newPower = 11.0;
                int powerVal = static_cast<int>(newPower);
                m_tcpClient->sendCAT(QString("PC%1H;").arg(powerVal, 3, 10, QChar('0')));
            } else {
                newPower = qBound(0.1, newPower, 10.0);
                int powerVal = static_cast<int>(qRound(newPower * 10)); // 9.9W = 099
                m_tcpClient->sendCAT(QString("PC%1L;").arg(powerVal, 3, 10, QChar('0')));
            }
        } else {
            // Currently in QRO range: 1W increments
            newPower = currentPower + delta;
            if (newPower <= 10.0) {
                // Transition to QRP at 10.0W
                newPower = 10.0;
                int powerVal = static_cast<int>(qRound(newPower * 10)); // 10.0W = 100
                m_tcpClient->sendCAT(QString("PC%1L;").arg(powerVal, 3, 10, QChar('0')));
            } else {
                newPower = qBound(11.0, newPower, 110.0);
                int powerVal = static_cast<int>(newPower);
                m_tcpClient->sendCAT(QString("PC%1H;").arg(powerVal, 3, 10, QChar('0')));
            }
        }
        m_radioState->setRfPower(newPower);
    });
    connect(m_sideControlPanel, &SideControlPanel::delayChanged, this, [this](int delta) {
        int currentDelay = m_radioState->delayForCurrentMode();
        int newDelay = qBound(0, currentDelay + delta, 250);
        m_tcpClient->sendCAT(QString("SD%1;").arg(newDelay, 4, 10, QChar('0')));
        // Note: Delay is mode-specific, handled by RadioState parsing
    });
    // Group 2: BW/HI and SHFT/LO
    // BW command uses 10Hz units (divide by 10)
    connect(m_sideControlPanel, &SideControlPanel::bandwidthChanged, this, [this](int delta) {
        bool bSet = m_radioState->bSetEnabled();
        int currentBw = bSet ? m_radioState->filterBandwidthB() : m_radioState->filterBandwidth();
        int newBw = qBound(50, currentBw + (delta * 50), 5000);
        QString cmd = bSet ? "BW$" : "BW";
        m_tcpClient->sendCAT(QString("%1%2;").arg(cmd).arg(newBw / 10, 4, 10, QChar('0')));
        if (bSet) {
            m_radioState->setFilterBandwidthB(newBw);
        } else {
            m_radioState->setFilterBandwidth(newBw);
        }
    });
    connect(m_sideControlPanel, &SideControlPanel::highCutChanged, this, [this](int delta) {
        bool bSet = m_radioState->bSetEnabled();
        int currentBw = bSet ? m_radioState->filterBandwidthB() : m_radioState->filterBandwidth();
        int newBw = qBound(50, currentBw + (delta * 50), 5000);
        QString cmd = bSet ? "BW$" : "BW";
        m_tcpClient->sendCAT(QString("%1%2;").arg(cmd).arg(newBw / 10, 4, 10, QChar('0')));
        if (bSet) {
            m_radioState->setFilterBandwidthB(newBw);
        } else {
            m_radioState->setFilterBandwidth(newBw);
        }
    });
    connect(m_sideControlPanel, &SideControlPanel::shiftChanged, this, [this](int delta) {
        bool bSet = m_radioState->bSetEnabled();
        int currentShift = bSet ? m_radioState->ifShiftB() : m_radioState->ifShift();
        int newShift = qBound(-999, currentShift + delta, 999);
        QString prefix = bSet ? "IS$" : "IS";
        QString cmd =
            QString("%1%2%3;").arg(prefix).arg(newShift >= 0 ? "+" : "-").arg(qAbs(newShift), 4, 10, QChar('0'));
        m_tcpClient->sendCAT(cmd);
        if (bSet) {
            m_radioState->setIfShiftB(newShift);
        } else {
            m_radioState->setIfShift(newShift);
        }
    });
    connect(m_sideControlPanel, &SideControlPanel::lowCutChanged, this, [this](int delta) {
        bool bSet = m_radioState->bSetEnabled();
        int currentShift = bSet ? m_radioState->ifShiftB() : m_radioState->ifShift();
        int newShift = qBound(-999, currentShift + delta, 999);
        QString prefix = bSet ? "IS$" : "IS";
        QString cmd =
            QString("%1%2%3;").arg(prefix).arg(newShift >= 0 ? "+" : "-").arg(qAbs(newShift), 4, 10, QChar('0'));
        m_tcpClient->sendCAT(cmd);
        if (bSet) {
            m_radioState->setIfShiftB(newShift);
        } else {
            m_radioState->setIfShift(newShift);
        }
    });
    // Group 3: M.RF/M.SQL and S.RF/S.SQL
    // RF Gain uses RG-nn; format where nn is 00-60 (representing -0 to -60 dB attenuation)
    // Scroll up = less attenuation = decrease value, scroll down = more attenuation = increase value
    connect(m_sideControlPanel, &SideControlPanel::mainRfGainChanged, this, [this](int delta) {
        int newGain = qBound(0, m_radioState->rfGain() - delta, 60);
        m_tcpClient->sendCAT(QString("RG-%1;").arg(newGain, 2, 10, QChar('0')));
        m_radioState->setRfGain(newGain);
    });
    connect(m_sideControlPanel, &SideControlPanel::mainSquelchChanged, this, [this](int delta) {
        int newSql = qBound(0, m_radioState->squelchLevel() + delta, 29);
        m_tcpClient->sendCAT(QString("SQ%1;").arg(newSql, 3, 10, QChar('0')));
        m_radioState->setSquelchLevel(newSql);
    });
    connect(m_sideControlPanel, &SideControlPanel::subRfGainChanged, this, [this](int delta) {
        int newGain = qBound(0, m_radioState->rfGainB() - delta, 60);
        m_tcpClient->sendCAT(QString("RG$-%1;").arg(newGain, 2, 10, QChar('0')));
        m_radioState->setRfGainB(newGain);
    });
    connect(m_sideControlPanel, &SideControlPanel::subSquelchChanged, this, [this](int delta) {
        int newSql = qBound(0, m_radioState->squelchLevelB() + delta, 29);
        m_tcpClient->sendCAT(QString("SQ$%1;").arg(newSql, 3, 10, QChar('0')));
        m_radioState->setSquelchLevelB(newSql);
    });

    // Connect TX function button signals to CAT commands
    connect(m_sideControlPanel, &SideControlPanel::tuneClicked, this, [this]() { m_tcpClient->sendCAT("SW16;"); });
    connect(m_sideControlPanel, &SideControlPanel::tuneLpClicked, this, [this]() { m_tcpClient->sendCAT("SW131;"); });
    connect(m_sideControlPanel, &SideControlPanel::xmitClicked, this, [this]() { m_tcpClient->sendCAT("SW30;"); });
    connect(m_sideControlPanel, &SideControlPanel::testClicked, this, [this]() { m_tcpClient->sendCAT("SW132;"); });
    connect(m_sideControlPanel, &SideControlPanel::atuClicked, this, [this]() { m_tcpClient->sendCAT("SW158;"); });
    connect(m_sideControlPanel, &SideControlPanel::atuTuneClicked, this, [this]() { m_tcpClient->sendCAT("SW40;"); });
    connect(m_sideControlPanel, &SideControlPanel::voxClicked, this, [this]() { m_tcpClient->sendCAT("SW50;"); });
    connect(m_sideControlPanel, &SideControlPanel::qskClicked, this, [this]() { m_tcpClient->sendCAT("SW134;"); });
    connect(m_sideControlPanel, &SideControlPanel::antClicked, this, [this]() { m_tcpClient->sendCAT("SW60;"); });
    // remAntClicked - not yet implemented (TBD)
    connect(m_sideControlPanel, &SideControlPanel::rxAntClicked, this, [this]() { m_tcpClient->sendCAT("SW70;"); });
    connect(m_sideControlPanel, &SideControlPanel::subAntClicked, this, [this]() { m_tcpClient->sendCAT("SW157;"); });

    // Connect right side panel button signals to CAT commands
    // Primary (left-click) signals
    connect(m_rightSidePanel, &RightSidePanel::preClicked, this, [this]() { m_tcpClient->sendCAT("SW61;"); });
    connect(m_rightSidePanel, &RightSidePanel::nbClicked, this, [this]() { m_tcpClient->sendCAT("SW32;"); });
    connect(m_rightSidePanel, &RightSidePanel::nrClicked, this, [this]() { m_tcpClient->sendCAT("SW62;"); });
    connect(m_rightSidePanel, &RightSidePanel::ntchClicked, this, [this]() { m_tcpClient->sendCAT("SW31;"); });
    connect(m_rightSidePanel, &RightSidePanel::filClicked, this, [this]() { m_tcpClient->sendCAT("SW33;"); });
    connect(m_rightSidePanel, &RightSidePanel::abClicked, this, [this]() { m_tcpClient->sendCAT("SW41;"); });
    // revClicked - TBD (needs press/release pattern)
    connect(m_rightSidePanel, &RightSidePanel::atobClicked, this, [this]() { m_tcpClient->sendCAT("SW72;"); });
    connect(m_rightSidePanel, &RightSidePanel::spotClicked, this, [this]() { m_tcpClient->sendCAT("SW42;"); });
    connect(m_rightSidePanel, &RightSidePanel::modeClicked, this, [this]() {
        // Toggle mode popup - if open, close it; otherwise show it
        if (m_modePopup->isVisible()) {
            m_modePopup->hidePopup();
        } else {
            // Update current state before showing - use A or B based on B SET
            bool bSet = m_radioState->bSetEnabled();
            if (bSet) {
                m_modePopup->setFrequency(m_radioState->vfoB());
                m_modePopup->setCurrentMode(static_cast<int>(m_radioState->modeB()));
                m_modePopup->setCurrentDataSubMode(m_radioState->dataSubModeB());
            } else {
                m_modePopup->setFrequency(m_radioState->vfoA());
                m_modePopup->setCurrentMode(static_cast<int>(m_radioState->mode()));
                m_modePopup->setCurrentDataSubMode(m_radioState->dataSubMode());
            }
            m_modePopup->setBSetEnabled(bSet);
            // Arrow points to MAIN RX or SUB RX based on B SET state
            QWidget *arrowTarget = bSet ? m_bottomMenuBar->subRxButton() : m_bottomMenuBar->mainRxButton();
            m_modePopup->showAboveWidget(m_bottomMenuBar, arrowTarget);
        }
    });

    // Secondary (right-click) signals - these show feature menus with toggle behavior
    // If same menu is open, close it; otherwise switch to the new menu
    auto toggleFeatureMenu = [this](FeatureMenuBar::Feature feature) {
        if (m_featureMenuBar->isMenuVisible() && m_featureMenuBar->currentFeature() == feature) {
            m_featureMenuBar->hideMenu();
        } else {
            // Populate initial state from RadioState (use Sub RX state if B SET enabled)
            bool bSet = m_radioState->bSetEnabled();
            switch (feature) {
            case FeatureMenuBar::Attenuator:
                if (bSet) {
                    m_featureMenuBar->setFeatureEnabled(m_radioState->attenuatorEnabledB());
                    m_featureMenuBar->setValue(m_radioState->attenuatorLevelB());
                } else {
                    m_featureMenuBar->setFeatureEnabled(m_radioState->attenuatorEnabled());
                    m_featureMenuBar->setValue(m_radioState->attenuatorLevel());
                }
                break;
            case FeatureMenuBar::NbLevel:
                if (bSet) {
                    m_featureMenuBar->setFeatureEnabled(m_radioState->noiseBlankerEnabledB());
                    m_featureMenuBar->setValue(m_radioState->noiseBlankerLevelB());
                    m_featureMenuBar->setNbFilter(m_radioState->noiseBlankerFilterWidthB());
                } else {
                    m_featureMenuBar->setFeatureEnabled(m_radioState->noiseBlankerEnabled());
                    m_featureMenuBar->setValue(m_radioState->noiseBlankerLevel());
                    m_featureMenuBar->setNbFilter(m_radioState->noiseBlankerFilterWidth());
                }
                break;
            case FeatureMenuBar::NrAdjust:
                if (bSet) {
                    m_featureMenuBar->setFeatureEnabled(m_radioState->noiseReductionEnabledB());
                    m_featureMenuBar->setValue(m_radioState->noiseReductionLevelB());
                } else {
                    m_featureMenuBar->setFeatureEnabled(m_radioState->noiseReductionEnabled());
                    m_featureMenuBar->setValue(m_radioState->noiseReductionLevel());
                }
                break;
            case FeatureMenuBar::ManualNotch:
                // Use correct VFO's notch state
                if (bSet) {
                    m_featureMenuBar->setFeatureEnabled(m_radioState->manualNotchEnabledB());
                    m_featureMenuBar->setValue(m_radioState->manualNotchPitchB());
                } else {
                    m_featureMenuBar->setFeatureEnabled(m_radioState->manualNotchEnabled());
                    m_featureMenuBar->setValue(m_radioState->manualNotchPitch());
                }
                break;
            }
            // Show popup positioned above the bottom menu bar (like other popups)
            m_featureMenuBar->showForFeature(feature);
            m_featureMenuBar->showAboveWidget(m_bottomMenuBar);
        }
    };
    connect(m_rightSidePanel, &RightSidePanel::attnClicked, this,
            [=]() { toggleFeatureMenu(FeatureMenuBar::Attenuator); });
    connect(m_rightSidePanel, &RightSidePanel::levelClicked, this,
            [=]() { toggleFeatureMenu(FeatureMenuBar::NbLevel); });
    connect(m_rightSidePanel, &RightSidePanel::adjClicked, this,
            [=]() { toggleFeatureMenu(FeatureMenuBar::NrAdjust); });
    connect(m_rightSidePanel, &RightSidePanel::manualClicked, this,
            [=]() { toggleFeatureMenu(FeatureMenuBar::ManualNotch); });
    connect(m_rightSidePanel, &RightSidePanel::apfClicked, this, [this]() { m_tcpClient->sendCAT("SW144;"); });
    connect(m_rightSidePanel, &RightSidePanel::splitClicked, this, [this]() { m_tcpClient->sendCAT("SW145;"); });
    connect(m_rightSidePanel, &RightSidePanel::btoaClicked, this, [this]() { m_tcpClient->sendCAT("SW147;"); });
    connect(m_rightSidePanel, &RightSidePanel::autoClicked, this, [this]() { m_tcpClient->sendCAT("SW146;"); });
    // altClicked (MODE/ALT right-click) - send SW148 for ALT function
    connect(m_rightSidePanel, &RightSidePanel::altClicked, this, [this]() { m_tcpClient->sendCAT("SW148;"); });

    // PF row primary (left-click) signals
    connect(m_rightSidePanel, &RightSidePanel::bsetClicked, this, [this]() { m_tcpClient->sendCAT("SW44;"); });
    connect(m_rightSidePanel, &RightSidePanel::clrClicked, this, [this]() { m_tcpClient->sendCAT("SW64;"); });
    connect(m_rightSidePanel, &RightSidePanel::ritClicked, this, [this]() { m_tcpClient->sendCAT("SW54;"); });
    connect(m_rightSidePanel, &RightSidePanel::xitClicked, this, [this]() { m_tcpClient->sendCAT("SW74;"); });

    // PF row secondary (right-click) signals
    // PF1-PF4 execute user-configured macros (or default K4 PF functions if no macro set)
    connect(m_rightSidePanel, &RightSidePanel::pf1Clicked, this, [this]() {
        MacroEntry macro = RadioSettings::instance()->macro(MacroIds::PF1);
        if (!macro.command.isEmpty()) {
            executeMacro(MacroIds::PF1);
        } else {
            m_tcpClient->sendCAT("SW153;"); // Default: K4 PF1
        }
    });
    connect(m_rightSidePanel, &RightSidePanel::pf2Clicked, this, [this]() {
        MacroEntry macro = RadioSettings::instance()->macro(MacroIds::PF2);
        if (!macro.command.isEmpty()) {
            executeMacro(MacroIds::PF2);
        } else {
            m_tcpClient->sendCAT("SW154;"); // Default: K4 PF2
        }
    });
    connect(m_rightSidePanel, &RightSidePanel::pf3Clicked, this, [this]() {
        MacroEntry macro = RadioSettings::instance()->macro(MacroIds::PF3);
        if (!macro.command.isEmpty()) {
            executeMacro(MacroIds::PF3);
        } else {
            m_tcpClient->sendCAT("SW155;"); // Default: K4 PF3
        }
    });
    connect(m_rightSidePanel, &RightSidePanel::pf4Clicked, this, [this]() {
        MacroEntry macro = RadioSettings::instance()->macro(MacroIds::PF4);
        if (!macro.command.isEmpty()) {
            executeMacro(MacroIds::PF4);
        } else {
            m_tcpClient->sendCAT("SW156;"); // Default: K4 PF4
        }
    });

    // Bottom row signals (SUB, DIVERSITY, RATE)
    connect(m_rightSidePanel, &RightSidePanel::subClicked, this, [this]() { m_tcpClient->sendCAT("SW83;"); });
    connect(m_rightSidePanel, &RightSidePanel::diversityClicked, this, [this]() { m_tcpClient->sendCAT("SW152;"); });
    connect(m_rightSidePanel, &RightSidePanel::rateClicked, this,
            [this]() { m_tcpClient->sendCAT("SW73;"); }); // Cycle fine rates
    connect(m_rightSidePanel, &RightSidePanel::khzClicked, this,
            [this]() { m_tcpClient->sendCAT("SW150;"); }); // Jump to 100kHz

    // Connect memory buttons (M1-M4, REC, STORE, RCL)
    // Primary actions (left click)
    connect(m_m1Btn, &QPushButton::clicked, this, [this]() { m_tcpClient->sendCAT("SW17;"); });
    connect(m_m2Btn, &QPushButton::clicked, this, [this]() { m_tcpClient->sendCAT("SW51;"); });
    connect(m_m3Btn, &QPushButton::clicked, this, [this]() { m_tcpClient->sendCAT("SW18;"); });
    connect(m_m4Btn, &QPushButton::clicked, this, [this]() { m_tcpClient->sendCAT("SW52;"); });
    connect(m_recBtn, &QPushButton::clicked, this, [this]() { m_tcpClient->sendCAT("SW19;"); });
    connect(m_storeBtn, &QPushButton::clicked, this, [this]() { m_tcpClient->sendCAT("SW20;"); });
    connect(m_rclBtn, &QPushButton::clicked, this, [this]() { m_tcpClient->sendCAT("SW34;"); });

    // Install event filters for right-click (alternate actions)
    m_recBtn->installEventFilter(this);
    m_storeBtn->installEventFilter(this);
    m_rclBtn->installEventFilter(this);

    // Connect bottom menu bar signals
    connect(m_bottomMenuBar, &BottomMenuBar::menuClicked, this, &MainWindow::showMenuOverlay);
    connect(m_bottomMenuBar, &BottomMenuBar::fnClicked, this, &MainWindow::toggleFnPopup);
    connect(m_bottomMenuBar, &BottomMenuBar::displayClicked, this, &MainWindow::toggleDisplayPopup);
    connect(m_bottomMenuBar, &BottomMenuBar::bandClicked, this, &MainWindow::toggleBandPopup);
    connect(m_bottomMenuBar, &BottomMenuBar::mainRxClicked, this, &MainWindow::toggleMainRxPopup);
    connect(m_bottomMenuBar, &BottomMenuBar::subRxClicked, this, &MainWindow::toggleSubRxPopup);
    connect(m_bottomMenuBar, &BottomMenuBar::txClicked, this, &MainWindow::toggleTxPopup);

    // Style button to toggle between spectrum display styles
    connect(m_bottomMenuBar, &BottomMenuBar::styleClicked, this, [this]() {
        SpectrumStyle currentStyle = m_panadapterA->spectrumStyle();
        SpectrumStyle newStyle;
        QString styleName;

        // Toggle: BlueAmplitude <-> Blue
        if (currentStyle == SpectrumStyle::BlueAmplitude) {
            newStyle = SpectrumStyle::Blue;
            styleName = "Blue (Y-position)";
        } else {
            newStyle = SpectrumStyle::BlueAmplitude;
            styleName = "Blue Amplitude (LUT)";
        }

        m_panadapterA->setSpectrumStyle(newStyle);
        m_panadapterB->setSpectrumStyle(newStyle);
        qDebug() << "Spectrum style changed to:" << styleName;
    });

    // PTT button connections
    connect(m_bottomMenuBar, &BottomMenuBar::pttPressed, this, &MainWindow::onPttPressed);
    connect(m_bottomMenuBar, &BottomMenuBar::pttReleased, this, &MainWindow::onPttReleased);

    // Connect microphone frames to encoding/transmission
    connect(m_audioEngine, &AudioEngine::microphoneFrame, this, &MainWindow::onMicrophoneFrame);
}

void MainWindow::setupTopStatusBar(QWidget *parent) {
    auto *statusBar = new QWidget(parent);
    statusBar->setFixedHeight(28);
    statusBar->setStyleSheet(QString("background-color: %1;").arg(K4Colors::DarkBackground));

    auto *layout = new QHBoxLayout(statusBar);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(20);

    // Elecraft K4 title
    m_titleLabel = new QLabel("Elecraft K4", statusBar);
    m_titleLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 14px;").arg(K4Colors::TextWhite));
    layout->addWidget(m_titleLabel);

    // Date/Time
    m_dateTimeLabel = new QLabel("--/-- --:--:-- Z", statusBar);
    m_dateTimeLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(K4Colors::TextGray));
    layout->addWidget(m_dateTimeLabel);

    layout->addStretch();

    // Power
    m_powerLabel = new QLabel("--- W", statusBar);
    m_powerLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(K4Colors::VfoAAmber));
    layout->addWidget(m_powerLabel);

    // SWR
    m_swrLabel = new QLabel("-.-:1", statusBar);
    m_swrLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(K4Colors::VfoAAmber));
    layout->addWidget(m_swrLabel);

    // Voltage
    m_voltageLabel = new QLabel("--.- V", statusBar);
    m_voltageLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(K4Colors::VfoAAmber));
    layout->addWidget(m_voltageLabel);

    // Current
    m_currentLabel = new QLabel("-.- A", statusBar);
    m_currentLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(K4Colors::VfoAAmber));
    layout->addWidget(m_currentLabel);

    layout->addStretch();

    // KPA1500 status (to left of K4 status)
    m_kpa1500StatusLabel = new QLabel("", statusBar);
    m_kpa1500StatusLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(K4Colors::InactiveGray));
    m_kpa1500StatusLabel->hide(); // Hidden when not enabled
    layout->addWidget(m_kpa1500StatusLabel);

    // K4 Connection status
    m_connectionStatusLabel = new QLabel("K4 Disconnected", statusBar);
    m_connectionStatusLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(K4Colors::InactiveGray));
    layout->addWidget(m_connectionStatusLabel);
}

void MainWindow::setupVfoSection(QWidget *parent) {
    // Main vertical layout: VFO row on top, antenna row below
    auto *mainVLayout = new QVBoxLayout(parent);
    mainVLayout->setContentsMargins(4, 4, 4, 4);
    mainVLayout->setSpacing(4);

    // Top row: VFO A | Center | VFO B
    auto *vfoRowWidget = new QWidget(parent);
    auto *layout = new QHBoxLayout(vfoRowWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // ===== VFO A (Left - Amber) - Using VFOWidget =====
    m_vfoA = new VFOWidget(VFOWidget::VFO_A, parent);

    // Connect VFO A click to toggle mini-pan (send CAT to enable Mini-Pan streaming)
    connect(m_vfoA, &VFOWidget::normalContentClicked, this, [this]() {
        m_vfoA->showMiniPan();
        m_radioState->setMiniPanAEnabled(true); // Set state BEFORE sending CAT (K4 doesn't echo)
        m_tcpClient->sendCAT("#MP1;");          // Enable Mini-Pan A streaming
    });
    connect(m_vfoA, &VFOWidget::miniPanClicked, this, [this]() {
        m_radioState->setMiniPanAEnabled(false); // Set state BEFORE sending CAT
        m_tcpClient->sendCAT("#MP0;");           // Disable Mini-Pan A streaming
    });

    // Set Mini-Pan A colors to blue (matching main panadapter A passband)
    m_vfoA->setMiniPanSpectrumColor(QColor(0, 128, 255));     // Blue spectrum
    m_vfoA->setMiniPanPassbandColor(QColor(0, 128, 255, 64)); // Blue passband

    layout->addWidget(m_vfoA, 1, Qt::AlignTop);

    // ===== Center Section =====
    auto *centerWidget = new QWidget(parent);
    centerWidget->setFixedWidth(310);
    centerWidget->setStyleSheet(QString("background-color: %1;").arg(K4Colors::Background));
    auto *centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(4, 4, 4, 4);
    centerLayout->setSpacing(3);

    // Row 1: VFO Row with absolute positioning for perfect TX centering
    // Uses VfoRowWidget to position A, TX, B, SUB/DIV independently
    m_vfoRow = new VfoRowWidget(centerWidget);
    centerLayout->addWidget(m_vfoRow);

    // Get pointers to VfoRowWidget children for signal connections
    m_vfoASquare = m_vfoRow->vfoASquare();
    m_vfoBSquare = m_vfoRow->vfoBSquare();
    m_modeALabel = m_vfoRow->modeALabel();
    m_modeBLabel = m_vfoRow->modeBLabel();
    m_txIndicator = m_vfoRow->txIndicator();
    m_txTriangle = m_vfoRow->txTriangle();
    m_txTriangleB = m_vfoRow->txTriangleB();
    m_testLabel = m_vfoRow->testLabel();
    m_subLabel = m_vfoRow->subLabel();
    m_divLabel = m_vfoRow->divLabel();

    // Install event filters for clickable labels
    m_vfoASquare->installEventFilter(this);
    m_vfoBSquare->installEventFilter(this);
    m_modeALabel->installEventFilter(this);
    m_modeBLabel->installEventFilter(this);

    // SPLIT indicator
    m_splitLabel = new QLabel("SPLIT OFF", centerWidget);
    m_splitLabel->setAlignment(Qt::AlignCenter);
    m_splitLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(K4Colors::VfoAAmber));
    centerLayout->addWidget(m_splitLabel);

    // B SET indicator (green rounded rect with black text, hidden by default)
    m_bSetLabel = new QLabel("B SET", centerWidget);
    m_bSetLabel->setAlignment(Qt::AlignCenter);
    m_bSetLabel->setStyleSheet("background-color: #00FF00;"
                               "color: black;"
                               "font-size: 12px;"
                               "font-weight: bold;"
                               "border-radius: 4px;"
                               "padding: 2px 8px;");
    m_bSetLabel->setVisible(false);
    centerLayout->addWidget(m_bSetLabel, 0, Qt::AlignHCenter);

    // Message Bank indicator
    m_msgBankLabel = new QLabel("MSG: I", centerWidget);
    m_msgBankLabel->setAlignment(Qt::AlignCenter);
    m_msgBankLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(K4Colors::TextGray));
    centerLayout->addWidget(m_msgBankLabel);

    // RIT/XIT Box with border - constrained size
    m_ritXitBox = new QWidget(centerWidget);
    m_ritXitBox->setStyleSheet(QString("border: 1px solid %1;").arg(K4Colors::InactiveGray));
    m_ritXitBox->setMaximumWidth(80);
    m_ritXitBox->setMaximumHeight(40);
    auto *ritXitLayout = new QVBoxLayout(m_ritXitBox);
    ritXitLayout->setContentsMargins(1, 2, 1, 2);
    ritXitLayout->setSpacing(1);

    auto *ritXitLabelsRow = new QHBoxLayout();
    ritXitLabelsRow->setContentsMargins(11, 0, 11, 0);
    ritXitLabelsRow->setSpacing(8);

    m_ritLabel = new QLabel("RIT", m_ritXitBox);
    m_ritLabel->setStyleSheet(QString("color: %1; font-size: 10px; border: none;").arg(K4Colors::InactiveGray));
    ritXitLabelsRow->addWidget(m_ritLabel);

    m_xitLabel = new QLabel("XIT", m_ritXitBox);
    m_xitLabel->setStyleSheet(QString("color: %1; font-size: 10px; border: none;").arg(K4Colors::InactiveGray));
    ritXitLabelsRow->addWidget(m_xitLabel);

    ritXitLabelsRow->setAlignment(Qt::AlignCenter);
    ritXitLayout->addLayout(ritXitLabelsRow);

    // Separator line between labels and value (spans full width)
    auto *ritXitSeparator = new QFrame(m_ritXitBox);
    ritXitSeparator->setFrameShape(QFrame::HLine);
    ritXitSeparator->setFrameShadow(QFrame::Plain);
    ritXitSeparator->setStyleSheet(QString("background-color: %1; border: none;").arg(K4Colors::InactiveGray));
    ritXitSeparator->setFixedHeight(1);
    ritXitLayout->addWidget(ritXitSeparator);

    m_ritXitValueLabel = new QLabel("+0.00", m_ritXitBox);
    m_ritXitValueLabel->setAlignment(Qt::AlignCenter);
    m_ritXitValueLabel->setStyleSheet(
        QString("color: %1; font-size: 14px; font-weight: bold; border: none; padding: 0 11px;")
            .arg(K4Colors::TextWhite));
    ritXitLayout->addWidget(m_ritXitValueLabel);

    // Create filter/RIT/XIT row - filter indicators flanking the RIT/XIT box
    auto *filterRitXitRow = new QHBoxLayout();
    filterRitXitRow->setContentsMargins(0, 0, 0, 0);
    filterRitXitRow->setSpacing(0);

    // VFO A filter indicator (left side, 45px to match A square above)
    auto *filterAContainer = new QWidget(centerWidget);
    filterAContainer->setFixedWidth(45);
    auto *filterALayout = new QVBoxLayout(filterAContainer);
    filterALayout->setContentsMargins(0, 0, 0, 0);
    filterALayout->setSpacing(0);

    m_filterALabel = new QLabel("FIL2", filterAContainer);
    m_filterALabel->setAlignment(Qt::AlignCenter);
    m_filterALabel->setStyleSheet("color: #FFD040; font-size: 10px; font-weight: bold;");
    filterALayout->addWidget(m_filterALabel);

    filterRitXitRow->addWidget(filterAContainer);
    filterRitXitRow->addStretch();

    // RIT/XIT box (centered)
    filterRitXitRow->addWidget(m_ritXitBox);

    filterRitXitRow->addStretch();

    // VFO B filter indicator (right side, 45px to match B square above)
    auto *filterBContainer = new QWidget(centerWidget);
    filterBContainer->setFixedWidth(45);
    auto *filterBLayout = new QVBoxLayout(filterBContainer);
    filterBLayout->setContentsMargins(0, 0, 0, 0);
    filterBLayout->setSpacing(0);

    m_filterBLabel = new QLabel("FIL2", filterBContainer);
    m_filterBLabel->setAlignment(Qt::AlignCenter);
    m_filterBLabel->setStyleSheet("color: #FFD040; font-size: 10px; font-weight: bold;");
    filterBLayout->addWidget(m_filterBLabel);

    filterRitXitRow->addWidget(filterBContainer);

    centerLayout->addLayout(filterRitXitRow);

    // VOX / ATU / QSK indicator row (fixed-height container so visibility toggles don't shift layout)
    auto *indicatorContainer = new QWidget(centerWidget);
    indicatorContainer->setFixedHeight(20);
    auto *indicatorLayout = new QHBoxLayout(indicatorContainer);
    indicatorLayout->setContentsMargins(0, 0, 0, 0);
    indicatorLayout->setSpacing(8);

    indicatorLayout->addStretch();

    // VOX indicator - orange when on, grey when off
    m_voxLabel = new QLabel("VOX", indicatorContainer);
    m_voxLabel->setAlignment(Qt::AlignCenter);
    m_voxLabel->setStyleSheet("color: #999999; font-size: 11px; font-weight: bold;");
    indicatorLayout->addWidget(m_voxLabel);

    // ATU indicator (orange when AUTO, grey when off)
    m_atuLabel = new QLabel("ATU", indicatorContainer);
    m_atuLabel->setAlignment(Qt::AlignCenter);
    m_atuLabel->setStyleSheet("color: #999999; font-size: 11px; font-weight: bold;");
    indicatorLayout->addWidget(m_atuLabel);

    // QSK indicator - white when on, grey when off
    m_qskLabel = new QLabel("QSK", indicatorContainer);
    m_qskLabel->setAlignment(Qt::AlignCenter);
    m_qskLabel->setStyleSheet("color: #999999; font-size: 11px; font-weight: bold;");
    indicatorLayout->addWidget(m_qskLabel);

    indicatorLayout->addStretch();

    centerLayout->addWidget(indicatorContainer);

    // ===== Memory Buttons Row (M1-M4, REC, STORE, RCL) =====
    centerLayout->addStretch(); // Push buttons to vertical center

    // Helper lambda to create memory button with optional sub-label
    auto createMemoryButton = [centerWidget](const QString &label, const QString &subLabel,
                                             bool isLighter) -> QWidget * {
        auto *container = new QWidget(centerWidget);
        auto *layout = new QVBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);

        auto *btn = new QPushButton(label, container);
        btn->setFixedSize(36, 24);
        btn->setCursor(Qt::PointingHandCursor);

        if (isLighter) {
            // Lighter grey for REC, STORE, RCL (2 shades lighter)
            btn->setStyleSheet(R"(
                QPushButton {
                    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #888888, stop:0.4 #777777,
                        stop:0.6 #6a6a6a, stop:1 #606060);
                    color: #FFFFFF;
                    border: 1px solid #909090;
                    border-radius: 3px;
                    font-size: 9px;
                    font-weight: bold;
                }
                QPushButton:hover {
                    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #989898, stop:0.4 #878787,
                        stop:0.6 #7a7a7a, stop:1 #707070);
                    border: 1px solid #a0a0a0;
                }
                QPushButton:pressed {
                    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #606060, stop:0.4 #6a6a6a,
                        stop:0.6 #777777, stop:1 #888888);
                    border: 1px solid #b0b0b0;
                }
            )");
        } else {
            // Standard dark grey for M1-M4, REC
            btn->setStyleSheet(R"(
                QPushButton {
                    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #4a4a4a, stop:0.4 #3a3a3a,
                        stop:0.6 #353535, stop:1 #2a2a2a);
                    color: #FFFFFF;
                    border: 1px solid #606060;
                    border-radius: 3px;
                    font-size: 9px;
                    font-weight: bold;
                }
                QPushButton:hover {
                    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #5a5a5a, stop:0.4 #4a4a4a,
                        stop:0.6 #454545, stop:1 #3a3a3a);
                    border: 1px solid #808080;
                }
                QPushButton:pressed {
                    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #2a2a2a, stop:0.4 #353535,
                        stop:0.6 #3a3a3a, stop:1 #4a4a4a);
                    border: 1px solid #909090;
                }
            )");
        }
        layout->addWidget(btn, 0, Qt::AlignHCenter);

        // Add sub-label if provided
        if (!subLabel.isEmpty()) {
            auto *sub = new QLabel(subLabel, container);
            sub->setStyleSheet("color: #FFB000; font-size: 7px;");
            sub->setAlignment(Qt::AlignCenter);
            layout->addWidget(sub);
        }

        return container;
    };

    // Row 1: M1-M4 (standard grey, no sub-labels)
    auto *memoryRow1 = new QHBoxLayout();
    memoryRow1->setContentsMargins(0, 0, 0, 0);
    memoryRow1->setSpacing(4);

    memoryRow1->addStretch();

    auto *m1Container = createMemoryButton("M1", "", false);
    m_m1Btn = m1Container->findChild<QPushButton *>();
    memoryRow1->addWidget(m1Container);

    auto *m2Container = createMemoryButton("M2", "", false);
    m_m2Btn = m2Container->findChild<QPushButton *>();
    memoryRow1->addWidget(m2Container);

    auto *m3Container = createMemoryButton("M3", "", false);
    m_m3Btn = m3Container->findChild<QPushButton *>();
    memoryRow1->addWidget(m3Container);

    auto *m4Container = createMemoryButton("M4", "", false);
    m_m4Btn = m4Container->findChild<QPushButton *>();
    memoryRow1->addWidget(m4Container);

    memoryRow1->addStretch();
    centerLayout->addLayout(memoryRow1);

    // Row 2: REC, STORE, RCL (centered below M1-M4)
    auto *memoryRow2 = new QHBoxLayout();
    memoryRow2->setContentsMargins(0, 0, 0, 0);
    memoryRow2->setSpacing(4);

    memoryRow2->addStretch();

    // REC (lighter grey, BANK sub-label)
    auto *recContainer = createMemoryButton("REC", "BANK", true);
    m_recBtn = recContainer->findChild<QPushButton *>();
    memoryRow2->addWidget(recContainer);

    // STORE (lighter grey, AF REC sub-label)
    auto *storeContainer = createMemoryButton("STORE", "AF REC", true);
    m_storeBtn = storeContainer->findChild<QPushButton *>();
    memoryRow2->addWidget(storeContainer);

    // RCL (lighter grey, AF PLAY sub-label)
    auto *rclContainer = createMemoryButton("RCL", "AF PLAY", true);
    m_rclBtn = rclContainer->findChild<QPushButton *>();
    memoryRow2->addWidget(rclContainer);

    memoryRow2->addStretch();
    centerLayout->addLayout(memoryRow2);

    centerLayout->addStretch(); // Balance below
    layout->addWidget(centerWidget);

    // ===== VFO B (Right - Cyan) - Using VFOWidget =====
    m_vfoB = new VFOWidget(VFOWidget::VFO_B, parent);

    // Set Mini-Pan B colors to green (matching main panadapter B passband)
    m_vfoB->setMiniPanSpectrumColor(QColor(0, 200, 0));     // Green spectrum
    m_vfoB->setMiniPanPassbandColor(QColor(0, 255, 0, 64)); // Green passband

    // Connect VFO B click to toggle mini-pan (send CAT to enable Mini-Pan streaming)
    connect(m_vfoB, &VFOWidget::normalContentClicked, this, [this]() {
        m_vfoB->showMiniPan();
        m_radioState->setMiniPanBEnabled(true); // Set state BEFORE sending CAT (K4 doesn't echo)
        m_tcpClient->sendCAT("#MP$1;");         // Enable Mini-Pan B (Sub RX) streaming
    });
    connect(m_vfoB, &VFOWidget::miniPanClicked, this, [this]() {
        m_radioState->setMiniPanBEnabled(false); // Set state BEFORE sending CAT
        m_tcpClient->sendCAT("#MP$0;");          // Disable Mini-Pan B streaming
    });

    layout->addWidget(m_vfoB, 1, Qt::AlignTop);

    // Add the VFO row to main layout
    mainVLayout->addWidget(vfoRowWidget);

    // NOTE: TX meters are now integrated into VFOWidgets as multifunction S/Po meters
    // (see VFOWidget::m_txMeter - displays S-meter when RX, Po when TX)

    // ===== Antenna Row (below VFO section) =====
    // Layout: [RX Ant A (left)] --- [TX Antenna (center)] --- [RX Ant B (right)]
    auto *antennaRow = new QHBoxLayout();
    antennaRow->setContentsMargins(8, 0, 8, 0);
    antennaRow->setSpacing(0);

    // RX Antenna A (Main) - white color, left-justified
    m_rxAntALabel = new QLabel("1:ANT1", parent);
    m_rxAntALabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_rxAntALabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Colors::TextWhite));
    antennaRow->addWidget(m_rxAntALabel);

    antennaRow->addStretch(1); // Push TX antenna to center

    // TX Antenna - orange color, centered
    m_txAntennaLabel = new QLabel("1:ANT1", parent);
    m_txAntennaLabel->setAlignment(Qt::AlignCenter);
    m_txAntennaLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Colors::VfoAAmber));
    antennaRow->addWidget(m_txAntennaLabel);

    antennaRow->addStretch(1); // Push RX Ant B to right

    // RX Antenna B (Sub) - white color, right-justified
    m_rxAntBLabel = new QLabel("1:ANT1", parent);
    m_rxAntBLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_rxAntBLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Colors::TextWhite));
    antennaRow->addWidget(m_rxAntBLabel);

    mainVLayout->addLayout(antennaRow);
}

void MainWindow::setupSpectrumPlaceholder(QWidget *parent) {
    // Container for spectrum displays
    m_spectrumContainer = new QWidget(parent);
    m_spectrumContainer->setStyleSheet(QString("background-color: %1;").arg(K4Colors::DarkBackground));
    m_spectrumContainer->setMinimumHeight(300);

    // Use QHBoxLayout for side-by-side panadapters (Main left, Sub right)
    auto *layout = new QHBoxLayout(m_spectrumContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2); // Small gap between panadapters

    // Main panadapter for VFO A (left side) - QRhiWidget with Metal/DirectX/Vulkan
    m_panadapterA = new PanadapterRhiWidget(m_spectrumContainer);
    m_panadapterA->setSpectrumLineColor(QColor(K4Colors::VfoAAmber));
    m_panadapterA->setDbRange(-140.0f, -20.0f);
    m_panadapterA->setSpectrumRatio(0.35f);
    m_panadapterA->setGridEnabled(true);
    layout->addWidget(m_panadapterA);

    // Sub panadapter for VFO B (right side) - QRhiWidget with Metal/DirectX/Vulkan
    m_panadapterB = new PanadapterRhiWidget(m_spectrumContainer);
    m_panadapterB->setSpectrumLineColor(QColor(K4Colors::VfoBCyan));
    m_panadapterB->setDbRange(-140.0f, -20.0f);
    m_panadapterB->setSpectrumRatio(0.35f);
    m_panadapterB->setGridEnabled(true);
    m_panadapterB->setPassbandColor(QColor(0, 200, 0, 64));    // Green passband for VFO B (25% alpha)
    m_panadapterB->setFrequencyMarkerColor(QColor(0, 140, 0)); // Darker green marker for VFO B
    layout->addWidget(m_panadapterB);
    m_panadapterB->hide(); // Start hidden (MainOnly mode)

    // Span control buttons - overlay on panadapter (lower right, above freq labels)
    QString btnStyle = "QPushButton { background: rgba(0,0,0,0.6); color: white; "
                       "border: 1px solid #666; border-radius: 4px; "
                       "font-size: 14px; font-weight: bold; min-width: 28px; min-height: 24px; }"
                       "QPushButton:hover { background: rgba(80,80,80,0.8); }";

    // Main panadapter (A) buttons
    m_spanDownBtn = new QPushButton("-", m_panadapterA);
    m_spanDownBtn->setStyleSheet(btnStyle);
    m_spanDownBtn->setFixedSize(28, 24);

    m_spanUpBtn = new QPushButton("+", m_panadapterA);
    m_spanUpBtn->setStyleSheet(btnStyle);
    m_spanUpBtn->setFixedSize(28, 24);

    m_centerBtn = new QPushButton("C", m_panadapterA);
    m_centerBtn->setStyleSheet(btnStyle);
    m_centerBtn->setFixedSize(28, 24);

    // Sub panadapter (B) buttons
    m_spanDownBtnB = new QPushButton("-", m_panadapterB);
    m_spanDownBtnB->setStyleSheet(btnStyle);
    m_spanDownBtnB->setFixedSize(28, 24);

    m_spanUpBtnB = new QPushButton("+", m_panadapterB);
    m_spanUpBtnB->setStyleSheet(btnStyle);
    m_spanUpBtnB->setFixedSize(28, 24);

    m_centerBtnB = new QPushButton("C", m_panadapterB);
    m_centerBtnB->setStyleSheet(btnStyle);
    m_centerBtnB->setFixedSize(28, 24);

    // VFO indicator badges - bottom-left corner of waterfall, tab shape with top-right rounded
    QString vfoIndicatorStyle = "QLabel { background: #707070; color: black; "
                                "font-size: 16px; font-weight: bold; "
                                "border-top-left-radius: 0px; border-top-right-radius: 8px; "
                                "border-bottom-left-radius: 0px; border-bottom-right-radius: 0px; }";

    m_vfoIndicatorA = new QLabel("A", m_panadapterA);
    m_vfoIndicatorA->setStyleSheet(vfoIndicatorStyle);
    m_vfoIndicatorA->setFixedSize(34, 30);
    m_vfoIndicatorA->setAlignment(Qt::AlignCenter);

    m_vfoIndicatorB = new QLabel("B", m_panadapterB);
    m_vfoIndicatorB->setStyleSheet(vfoIndicatorStyle);
    m_vfoIndicatorB->setFixedSize(34, 30);
    m_vfoIndicatorB->setAlignment(Qt::AlignCenter);

    // Position buttons (will be repositioned in resizeEvent of panadapter)
    // Triangle layout: C centered above, - and + below (bottom-right)
    m_spanDownBtn->move(m_panadapterA->width() - 70, m_panadapterA->height() - 45);
    m_spanUpBtn->move(m_panadapterA->width() - 35, m_panadapterA->height() - 45);
    m_centerBtn->move(m_panadapterA->width() - 52, m_panadapterA->height() - 73);

    m_spanDownBtnB->move(m_panadapterB->width() - 70, m_panadapterB->height() - 45);
    m_spanUpBtnB->move(m_panadapterB->width() - 35, m_panadapterB->height() - 45);
    m_centerBtnB->move(m_panadapterB->width() - 52, m_panadapterB->height() - 73);

    // VFO indicators at bottom-left corner, flush with edges
    m_vfoIndicatorA->move(0, m_panadapterA->height() - 30);
    m_vfoIndicatorB->move(0, m_panadapterB->height() - 30);

    // Span adjustment for Main: K4 span steps, inverted controls
    // - button = zoom out (increase span), + button = zoom in (decrease span)
    connect(m_spanDownBtn, &QPushButton::clicked, this, [this]() {
        int currentSpan = m_radioState->spanHz();
        int newSpan = getNextSpanUp(currentSpan); // - zooms out
        if (newSpan != currentSpan) {
            m_radioState->setSpanHz(newSpan);
            m_tcpClient->sendCAT(QString("#SPN%1;").arg(newSpan));
        }
    });

    connect(m_spanUpBtn, &QPushButton::clicked, this, [this]() {
        int currentSpan = m_radioState->spanHz();
        int newSpan = getNextSpanDown(currentSpan); // + zooms in
        if (newSpan != currentSpan) {
            m_radioState->setSpanHz(newSpan);
            m_tcpClient->sendCAT(QString("#SPN%1;").arg(newSpan));
        }
    });

    connect(m_centerBtn, &QPushButton::clicked, this, [this]() { m_tcpClient->sendCAT("FC;"); });

    // Span adjustment for Sub: uses $ suffix for Sub RX commands
    connect(m_spanDownBtnB, &QPushButton::clicked, this, [this]() {
        int currentSpan = m_radioState->spanHzB();
        int newSpan = getNextSpanUp(currentSpan); // - zooms out
        if (newSpan != currentSpan) {
            m_radioState->setSpanHzB(newSpan);
            m_tcpClient->sendCAT(QString("#SPN$%1;").arg(newSpan));
        }
    });

    connect(m_spanUpBtnB, &QPushButton::clicked, this, [this]() {
        int currentSpan = m_radioState->spanHzB();
        int newSpan = getNextSpanDown(currentSpan); // + zooms in
        if (newSpan != currentSpan) {
            m_radioState->setSpanHzB(newSpan);
            m_tcpClient->sendCAT(QString("#SPN$%1;").arg(newSpan));
        }
    });

    connect(m_centerBtnB, &QPushButton::clicked, this, [this]() { m_tcpClient->sendCAT("FC$;"); });

    // Install event filter to reposition span buttons when panadapters resize
    m_panadapterA->installEventFilter(this);
    m_panadapterB->installEventFilter(this);

    // Debug: Connect to renderFailed signal to diagnose QRhiWidget issues
    connect(m_panadapterA, &QRhiWidget::renderFailed, this,
            []() { qCritical() << "!!! PanadapterA renderFailed() emitted - QRhi could not be obtained !!!"; });
    connect(m_panadapterB, &QRhiWidget::renderFailed, this,
            []() { qCritical() << "!!! PanadapterB renderFailed() emitted - QRhi could not be obtained !!!"; });

    // Update panadapter when frequency/mode changes
    connect(m_radioState, &RadioState::frequencyChanged, this,
            [this](quint64 freq) { m_panadapterA->setTunedFrequency(freq); });
    connect(m_radioState, &RadioState::modeChanged, this,
            [this](RadioState::Mode mode) { m_panadapterA->setMode(RadioState::modeToString(mode)); });
    connect(m_radioState, &RadioState::filterBandwidthChanged, this,
            [this](int bw) { m_panadapterA->setFilterBandwidth(bw); });
    connect(m_radioState, &RadioState::ifShiftChanged, this, [this](int shift) { m_panadapterA->setIfShift(shift); });
    connect(m_radioState, &RadioState::cwPitchChanged, this, [this](int pitch) { m_panadapterA->setCwPitch(pitch); });

    // Notch filter visualization
    connect(m_radioState, &RadioState::notchChanged, this, [this]() {
        bool enabled = m_radioState->manualNotchEnabled();
        int pitch = m_radioState->manualNotchPitch();
        m_panadapterA->setNotchFilter(enabled, pitch);
        // Update mini-pan too (using forwarding method that handles lazy creation)
        m_vfoA->setMiniPanNotchFilter(enabled, pitch);
        // Update NTCH indicator in VFO processing row
        m_vfoA->setNotch(m_radioState->autoNotchEnabled(), m_radioState->manualNotchEnabled());
    });
    // Also update mini-pan mode when mode changes
    connect(m_radioState, &RadioState::modeChanged, this,
            [this](RadioState::Mode mode) { m_vfoA->setMiniPanMode(RadioState::modeToString(mode)); });

    // Mini-pan filter passband visualization (using forwarding methods)
    connect(m_radioState, &RadioState::filterBandwidthChanged, this,
            [this](int bw) { m_vfoA->setMiniPanFilterBandwidth(bw); });
    connect(m_radioState, &RadioState::ifShiftChanged, this, [this](int shift) { m_vfoA->setMiniPanIfShift(shift); });
    connect(m_radioState, &RadioState::cwPitchChanged, this, [this](int pitch) { m_vfoA->setMiniPanCwPitch(pitch); });

    // Tuning rate indicator (VT command)
    connect(m_radioState, &RadioState::tuningStepChanged, this, [this](int step) { m_vfoA->setTuningRate(step); });
    connect(m_radioState, &RadioState::tuningStepBChanged, this, [this](int step) { m_vfoB->setTuningRate(step); });

    // Mouse control: click to tune
    connect(m_panadapterA, &PanadapterRhiWidget::frequencyClicked, this, [this](qint64 freq) {
        // Guard: only send if connected and frequency is valid
        if (!m_tcpClient->isConnected() || freq <= 0)
            return;
        QString cmd = QString("FA%1;").arg(freq, 11, 10, QChar('0'));
        m_tcpClient->sendCAT(cmd);
        // Request frequency back to update UI (K4 doesn't echo SET commands)
        m_tcpClient->sendCAT("FA;");
    });

    // Mouse control: scroll wheel to adjust frequency using K4's native step
    connect(m_panadapterA, &PanadapterRhiWidget::frequencyScrolled, this, [this](int steps) {
        // Guard: only send if connected
        if (!m_tcpClient->isConnected())
            return;
        // Use UP/DN commands - respects K4's current VFO step size
        QString cmd = (steps > 0) ? "UP;" : "DN;";
        int count = qAbs(steps);
        for (int i = 0; i < count; i++) {
            m_tcpClient->sendCAT(cmd);
        }
        // Request frequency back to update UI
        m_tcpClient->sendCAT("FA;");
    });

    // VFO B connections
    connect(m_radioState, &RadioState::frequencyBChanged, this,
            [this](quint64 freq) { m_panadapterB->setTunedFrequency(freq); });
    connect(m_radioState, &RadioState::modeBChanged, this,
            [this](RadioState::Mode mode) { m_panadapterB->setMode(RadioState::modeToString(mode)); });
    connect(m_radioState, &RadioState::filterBandwidthBChanged, this,
            [this](int bw) { m_panadapterB->setFilterBandwidth(bw); });
    connect(m_radioState, &RadioState::ifShiftBChanged, this, [this](int shift) { m_panadapterB->setIfShift(shift); });
    connect(m_radioState, &RadioState::cwPitchChanged, this, [this](int pitch) { m_panadapterB->setCwPitch(pitch); });
    connect(m_radioState, &RadioState::notchBChanged, this, [this]() {
        bool enabled = m_radioState->manualNotchEnabledB();
        int pitch = m_radioState->manualNotchPitchB();
        m_panadapterB->setNotchFilter(enabled, pitch);
    });

    // VFO B Mini-Pan connections (mode-dependent bandwidth, using forwarding methods)
    connect(m_radioState, &RadioState::modeBChanged, this,
            [this](RadioState::Mode mode) { m_vfoB->setMiniPanMode(RadioState::modeToString(mode)); });
    connect(m_radioState, &RadioState::filterBandwidthBChanged, this,
            [this](int bw) { m_vfoB->setMiniPanFilterBandwidth(bw); });
    connect(m_radioState, &RadioState::ifShiftBChanged, this, [this](int shift) { m_vfoB->setMiniPanIfShift(shift); });
    connect(m_radioState, &RadioState::cwPitchChanged, this, [this](int pitch) { m_vfoB->setMiniPanCwPitch(pitch); });
    connect(m_radioState, &RadioState::notchBChanged, this, [this]() {
        bool enabled = m_radioState->manualNotchEnabledB();
        int pitch = m_radioState->manualNotchPitchB();
        m_vfoB->setMiniPanNotchFilter(enabled, pitch);
        // Update NTCH indicator in VFO B processing row
        m_vfoB->setNotch(m_radioState->autoNotchEnabledB(), m_radioState->manualNotchEnabledB());
    });

    // Mouse control for VFO B: click to tune
    connect(m_panadapterB, &PanadapterRhiWidget::frequencyClicked, this, [this](qint64 freq) {
        // Guard: only send if connected and frequency is valid
        if (!m_tcpClient->isConnected() || freq <= 0)
            return;
        QString cmd = QString("FB%1;").arg(freq, 11, 10, QChar('0'));
        m_tcpClient->sendCAT(cmd);
        // Request frequency back to update UI (K4 doesn't echo SET commands)
        m_tcpClient->sendCAT("FB;");
    });

    // Mouse control for VFO B: scroll wheel to adjust frequency using K4's native step
    connect(m_panadapterB, &PanadapterRhiWidget::frequencyScrolled, this, [this](int steps) {
        // Guard: only send if connected
        if (!m_tcpClient->isConnected())
            return;
        // Use UP$/DN$ commands for Sub RX - respects K4's current VFO step size
        QString cmd = (steps > 0) ? "UP$;" : "DN$;";
        int count = qAbs(steps);
        for (int i = 0; i < count; i++) {
            m_tcpClient->sendCAT(cmd);
        }
        // Request frequency back to update UI
        m_tcpClient->sendCAT("FB;");
    });
}

void MainWindow::updateDateTime() {
    QDateTime now = QDateTime::currentDateTimeUtc();
    m_dateTimeLabel->setText(now.toString("M-dd / HH:mm:ss") + " Z");
    m_sideControlPanel->setTime(now.toString("HH:mm:ss") + " Z");
}

QString MainWindow::formatFrequency(quint64 freq) {
    QString freqStr = QString::number(freq);
    while (freqStr.length() < 8) {
        freqStr.prepend('0');
    }

    // Insert dots: XX.XXX.XXX
    QString formatted;
    int len = freqStr.length();
    for (int i = 0; i < len; i++) {
        formatted.append(freqStr[i]);
        int posFromEnd = len - i - 1;
        if (posFromEnd > 0 && posFromEnd % 3 == 0) {
            formatted.append('.');
        }
    }

    // Remove leading zero for frequencies < 10 MHz (40m-160m)
    if (formatted.startsWith('0')) {
        formatted = formatted.mid(1);
    }
    return formatted;
}

void MainWindow::showRadioManager() {
    RadioManagerDialog dialog(this);
    connect(&dialog, &RadioManagerDialog::connectRequested, this, &MainWindow::connectToRadio);
    connect(&dialog, &RadioManagerDialog::disconnectRequested, this, [this]() {
        // TcpClient::disconnectFromHost() sends RRN; automatically
        m_tcpClient->disconnectFromHost();
    });

    // Set the connected host so dialog can show "Disconnect" for active connection
    if (m_tcpClient->isConnected()) {
        dialog.setConnectedHost(m_currentRadio.host);
    }

    dialog.exec();
}

void MainWindow::connectToRadio(const RadioEntry &radio) {
    if (m_tcpClient->isConnected()) {
        m_tcpClient->disconnectFromHost();
    }

    m_currentRadio = radio;
    m_titleLabel->setText("Elecraft K4 - " + radio.name);

    qDebug() << "Connecting to" << radio.host << ":" << radio.port << (radio.useTls ? "(TLS/PSK)" : "(unencrypted)");
    m_tcpClient->connectToHost(radio.host, radio.port, radio.password, radio.useTls, radio.identity);
}

void MainWindow::onConnectClicked() {
    showRadioManager();
}

void MainWindow::onDisconnectClicked() {
    m_tcpClient->disconnectFromHost();
}

void MainWindow::onStateChanged(TcpClient::ConnectionState state) {
    updateConnectionState(state);
}

void MainWindow::onError(const QString &error) {
    m_connectionStatusLabel->setText("Error: " + error);
    m_connectionStatusLabel->setStyleSheet(
        QString("color: %1; font-size: 12px; font-weight: bold;").arg(K4Colors::TxRed));
}

void MainWindow::onAuthenticated() {
    qDebug() << "Successfully authenticated with K4 radio";

    // Start audio engine for RX audio
    if (m_audioEngine->start()) {
        qDebug() << "Audio engine started for RX audio";
        // Apply current volume slider settings to OpusDecoder (for channel mixing)
        m_opusDecoder->setMainVolume(m_sideControlPanel->volume() / 100.0f);
        m_opusDecoder->setSubVolume(m_sideControlPanel->subVolume() / 100.0f);
        // Apply saved mic gain setting
        m_audioEngine->setMicGain(RadioSettings::instance()->micGain() / 100.0f);
    } else {
        qWarning() << "Failed to start audio engine";
    }

    // Most state is already included in the RDY; response from TcpClient.
    // Only query commands NOT included in RDY dump:
    m_tcpClient->sendCAT("#DSM;");  // Display mode (LCD) - not in RDY
    m_tcpClient->sendCAT("#HDSM;"); // Display mode (EXT) - not in RDY
    m_tcpClient->sendCAT("#FRZ;");  // Freeze - not in RDY
    m_tcpClient->sendCAT("SIRC1;"); // Enable 1-second client stats updates
}

void MainWindow::onAuthenticationFailed() {
    qDebug() << "Authentication failed";
    m_connectionStatusLabel->setText("Auth Failed");
    m_connectionStatusLabel->setStyleSheet(
        QString("color: %1; font-size: 12px; font-weight: bold;").arg(K4Colors::TxRed));
}

void MainWindow::onCatResponse(const QString &response) {
    // Parse CAT commands (may contain multiple commands separated by ;)
    QStringList commands = response.split(';', Qt::SkipEmptyParts);
    for (const QString &cmd : commands) {
        m_radioState->parseCATCommand(cmd + ";");

        // Parse MEDF (menu definitions) from RDY response
        if (cmd.startsWith("MEDF")) {
            m_menuModel->parseMEDF(cmd + ";");
        }
        // Route ME (menu value) commands to MenuModel for real-time updates
        else if (cmd.startsWith("ME")) {
            m_menuModel->parseME(cmd + ";");
        }
        // Parse BN (Band Number) response for VFO A
        else if (cmd.startsWith("BN") && !cmd.startsWith("BN$")) {
            // VFO A band number: BNnn where nn is 00-10 or 16-25
            bool ok;
            int bandNum = cmd.mid(2, 2).toInt(&ok);
            if (ok) {
                updateBandSelection(bandNum);
            }
        }
    }
}

void MainWindow::onFrequencyChanged(quint64 freq) {
    m_vfoA->setFrequency(formatFrequency(freq));
}

void MainWindow::onFrequencyBChanged(quint64 freq) {
    m_vfoB->setFrequency(formatFrequency(freq));
}

void MainWindow::onModeChanged(RadioState::Mode mode) {
    Q_UNUSED(mode)
    // Use full mode string which includes data sub-mode (AFSK, FSK, PSK, DATA)
    m_modeALabel->setText(m_radioState->modeStringFull());
}

void MainWindow::onModeBChanged(RadioState::Mode mode) {
    Q_UNUSED(mode)
    // Use full mode string which includes data sub-mode (AFSK, FSK, PSK, DATA)
    m_modeBLabel->setText(m_radioState->modeStringFullB());
}

void MainWindow::onSMeterChanged(double value) {
    m_vfoA->setSMeterValue(value);
}

void MainWindow::onSMeterBChanged(double value) {
    m_vfoB->setSMeterValue(value);
}

void MainWindow::onBandwidthChanged(int bw) {
    Q_UNUSED(bw)
    // Could update a bandwidth display if needed
}

void MainWindow::onBandwidthBChanged(int bw) {
    Q_UNUSED(bw)
    // Could update a bandwidth display if needed
}

void MainWindow::updateConnectionState(TcpClient::ConnectionState state) {
    switch (state) {
    case TcpClient::Disconnected:
        m_connectionStatusLabel->setText("K4 Disconnected");
        m_connectionStatusLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(K4Colors::InactiveGray));
        m_titleLabel->setText("Elecraft K4");
        // Stop audio engine to prevent accessing invalid data
        if (m_audioEngine) {
            m_audioEngine->stop();
        }
        break;

    case TcpClient::Connecting:
        m_connectionStatusLabel->setText("K4 Connecting...");
        m_connectionStatusLabel->setStyleSheet(
            QString("color: %1; font-size: 12px; font-weight: bold;").arg(K4Colors::VfoAAmber));
        break;

    case TcpClient::Authenticating:
        m_connectionStatusLabel->setText("K4 Authenticating...");
        m_connectionStatusLabel->setStyleSheet(
            QString("color: %1; font-size: 12px; font-weight: bold;").arg(K4Colors::VfoAAmber));
        break;

    case TcpClient::Connected:
        m_connectionStatusLabel->setText("K4 Connected");
        m_connectionStatusLabel->setStyleSheet(
            QString("color: %1; font-size: 12px; font-weight: bold;").arg(K4Colors::AgcGreen));
        break;
    }
}

void MainWindow::onRfPowerChanged(double watts, bool isQrp) {
    Q_UNUSED(watts)
    Q_UNUSED(isQrp)
    // NOTE: This is the power SETTING (PC command), not actual TX power.
    // The power display is updated from txMeterChanged signal during TX.
    // We don't update the display here - it should show 0 when not transmitting.
}

void MainWindow::onSupplyVoltageChanged(double volts) {
    m_voltageLabel->setText(QString("%1 V").arg(volts, 0, 'f', 1));
    m_sideControlPanel->setVoltage(volts);
}

void MainWindow::onSupplyCurrentChanged(double amps) {
    m_currentLabel->setText(QString("%1 A").arg(amps, 0, 'f', 1));
    m_sideControlPanel->setCurrent(amps);
}

void MainWindow::onSwrChanged(double swr) {
    m_swrLabel->setText(QString("%1:1").arg(swr, 0, 'f', 1));
    m_sideControlPanel->setSwr(swr);
}

void MainWindow::onSplitChanged(bool enabled) {
    if (enabled) {
        m_splitLabel->setText("SPLIT ON");
        m_splitLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Colors::AgcGreen));
        // When split is on, TX goes to VFO B - clear left triangle, show right triangle
        m_txTriangle->setText("");
        m_txTriangleB->setText("▶");
    } else {
        m_splitLabel->setText("SPLIT OFF");
        m_splitLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(K4Colors::VfoAAmber));
        // When split is off, TX stays on VFO A - show left triangle, clear right triangle
        m_txTriangle->setText("◀");
        m_txTriangleB->setText("");
    }
}

void MainWindow::onAntennaChanged(int txAnt, int rxAntMain, int rxAntSub) {
    // Format RX antenna display based on AR command value
    // AR values 0-3 are special modes, not antenna indices:
    // 0 = Disconnected, 1 = EXT XVTR, 2 = RX uses TX ant, 3 = INT XVTR
    // 4 = RX1, 5 = RX2, 6-7 = ATU antennas
    auto formatRxAntenna = [this, txAnt](int arValue) -> QString {
        switch (arValue) {
        case 0: // Disconnected
            return "OFF";
        case 1: // EXT. XVTR IN
            return "EXT";
        case 2: // RX USES TX ANT - show TX antenna
            return QString("%1:%2").arg(txAnt).arg(m_radioState->antennaName(txAnt));
        case 3: // INT. XVTR IN
            return "INT";
        case 4: // RX ANT IN1
            return QString("RX1:%1").arg(m_radioState->antennaName(4));
        case 5: // RX ANT IN2 / ATU RX ANT1
            return QString("RX2:%1").arg(m_radioState->antennaName(5));
        default: // ATU antennas (6-7)
            return QString("ATU%1").arg(arValue - 4);
        }
    };

    // TX antenna (AN command) - always 1-3, format as "N:name"
    m_txAntennaLabel->setText(QString("%1:%2").arg(txAnt).arg(m_radioState->antennaName(txAnt)));

    // RX antennas (AR/AR$ commands) - use special mode handling
    m_rxAntALabel->setText(formatRxAntenna(rxAntMain));
    m_rxAntBLabel->setText(formatRxAntenna(rxAntSub));
}

void MainWindow::onAntennaNameChanged(int index, const QString &name) {
    Q_UNUSED(index)
    Q_UNUSED(name)
    // Refresh antenna displays when a name changes
    onAntennaChanged(m_radioState->txAntenna(), m_radioState->rxAntennaMain(), m_radioState->rxAntennaSub());
}

void MainWindow::onVoxChanged(bool enabled) {
    Q_UNUSED(enabled)
    // Use mode-specific VOX state (CW modes use VXC, Voice modes use VXV, Data modes use VXD)
    bool voxOn = m_radioState->voxForCurrentMode();
    if (voxOn) {
        m_voxLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Colors::VfoAAmber));
    } else {
        m_voxLabel->setStyleSheet("color: #999999; font-size: 11px; font-weight: bold;");
    }
}

void MainWindow::onQskEnabledChanged(bool enabled) {
    // QSK indicator: white when enabled, grey when disabled
    if (enabled) {
        m_qskLabel->setStyleSheet("color: #FFFFFF; font-size: 11px; font-weight: bold;");
    } else {
        m_qskLabel->setStyleSheet("color: #999999; font-size: 11px; font-weight: bold;");
    }
}

void MainWindow::onTestModeChanged(bool enabled) {
    // TEST indicator: visible in red when test mode is on
    m_testLabel->setVisible(enabled);
}

void MainWindow::onAtuModeChanged(int mode) {
    // ATU indicator: orange when AUTO mode (2), grey otherwise
    if (mode == 2) {
        m_atuLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Colors::VfoAAmber));
    } else {
        m_atuLabel->setStyleSheet("color: #999999; font-size: 11px; font-weight: bold;");
    }
}

void MainWindow::onRitXitChanged(bool ritEnabled, bool xitEnabled, int offset) {
    // Update RIT label
    if (ritEnabled) {
        m_ritLabel->setStyleSheet(
            QString("color: %1; font-size: 10px; font-weight: bold; border: none;").arg(K4Colors::TextWhite));
    } else {
        m_ritLabel->setStyleSheet(QString("color: %1; font-size: 10px; border: none;").arg(K4Colors::InactiveGray));
    }

    // Update XIT label
    if (xitEnabled) {
        m_xitLabel->setStyleSheet(
            QString("color: %1; font-size: 10px; font-weight: bold; border: none;").arg(K4Colors::TextWhite));
    } else {
        m_xitLabel->setStyleSheet(QString("color: %1; font-size: 10px; border: none;").arg(K4Colors::InactiveGray));
    }

    // Update offset value (in kHz)
    double offsetKHz = offset / 1000.0;
    QString sign = (offset >= 0) ? "+" : "";
    m_ritXitValueLabel->setText(QString("%1%2").arg(sign).arg(offsetKHz, 0, 'f', 2));
}

void MainWindow::onMessageBankChanged(int bank) {
    if (bank == 1) {
        m_msgBankLabel->setText("MSG: I");
    } else {
        m_msgBankLabel->setText("MSG: II");
    }
}

void MainWindow::onProcessingChanged() {
    // AGC
    QString agcText;
    switch (m_radioState->agcSpeed()) {
    case RadioState::AGC_Off:
        agcText = "AGC";
        break;
    case RadioState::AGC_Slow:
        agcText = "AGC-S";
        break;
    case RadioState::AGC_Fast:
        agcText = "AGC-F";
        break;
    }
    m_vfoA->setAGC(agcText);

    // Preamp (level 0-3)
    m_vfoA->setPreamp(m_radioState->preampEnabled() && m_radioState->preamp() > 0, m_radioState->preamp());

    // Attenuator (level 0-21 dB)
    m_vfoA->setAtt(m_radioState->attenuatorEnabled() && m_radioState->attenuatorLevel() > 0,
                   m_radioState->attenuatorLevel());

    // Noise Blanker
    m_vfoA->setNB(m_radioState->noiseBlankerEnabled());

    // Noise Reduction
    m_vfoA->setNR(m_radioState->noiseReductionEnabled());
}

void MainWindow::onProcessingChangedB() {
    // AGC
    QString agcText;
    switch (m_radioState->agcSpeedB()) {
    case RadioState::AGC_Off:
        agcText = "AGC";
        break;
    case RadioState::AGC_Slow:
        agcText = "AGC-S";
        break;
    case RadioState::AGC_Fast:
        agcText = "AGC-F";
        break;
    }
    m_vfoB->setAGC(agcText);

    // Preamp (level 0-3)
    m_vfoB->setPreamp(m_radioState->preampEnabledB() && m_radioState->preampB() > 0, m_radioState->preampB());

    // Attenuator (level 0-21 dB)
    m_vfoB->setAtt(m_radioState->attenuatorEnabledB() && m_radioState->attenuatorLevelB() > 0,
                   m_radioState->attenuatorLevelB());

    // Noise Blanker
    m_vfoB->setNB(m_radioState->noiseBlankerEnabledB());

    // Noise Reduction
    m_vfoB->setNR(m_radioState->noiseReductionEnabledB());
}

void MainWindow::onSpectrumData(int receiver, const QByteArray &data, qint64 centerFreq, qint32 sampleRate,
                                float noiseFloor) {
    // Route spectrum data to appropriate panadapter
    // receiver: 0 = Main (VFO A), 1 = Sub (VFO B)
    if (receiver == 0) {
        m_panadapterA->updateSpectrum(data, centerFreq, sampleRate, noiseFloor);
    } else if (receiver == 1) {
        m_panadapterB->updateSpectrum(data, centerFreq, sampleRate, noiseFloor);
    }
}

void MainWindow::onMiniSpectrumData(int receiver, const QByteArray &data) {
    // Route Mini-PAN data based on receiver byte (0=Main/A, 1=Sub/B)
    if (receiver == 0 && m_vfoA->isMiniPanVisible()) {
        m_vfoA->updateMiniPan(data);
    } else if (receiver == 1 && m_vfoB->isMiniPanVisible()) {
        m_vfoB->updateMiniPan(data);
    }
}

void MainWindow::onAudioData(const QByteArray &payload) {
    // Only process audio when connected
    if (!m_tcpClient->isConnected()) {
        return;
    }

    // Decode K4 audio packet (handles header parsing, stereo decode, de-interleave, gain boost)
    // Returns mono Float32 PCM for main channel (RX1/VFO A) only
    QByteArray pcmData = m_opusDecoder->decodeK4Packet(payload);

    if (!pcmData.isEmpty()) {
        m_audioEngine->playAudio(pcmData);
    }
}

void MainWindow::onPttPressed() {
    if (!m_tcpClient->isConnected()) {
        return;
    }

    m_pttActive = true;
    m_txSequence = 0; // Reset sequence on new PTT press
    m_audioEngine->setMicEnabled(true);
    m_bottomMenuBar->setPttActive(true);
    qDebug() << "PTT pressed - microphone enabled";
}

void MainWindow::onPttReleased() {
    m_pttActive = false;
    m_audioEngine->setMicEnabled(false);
    m_bottomMenuBar->setPttActive(false);
    qDebug() << "PTT released - microphone disabled";
}

void MainWindow::onMicrophoneFrame(const QByteArray &s16leData) {
    // Only transmit when PTT is active and connected
    if (!m_pttActive || !m_tcpClient->isConnected()) {
        return;
    }

    // Encode the audio frame using Opus
    QByteArray opusData = m_opusEncoder->encode(s16leData);
    if (opusData.isEmpty()) {
        return;
    }

    // Build and send the audio packet
    QByteArray packet = Protocol::buildAudioPacket(opusData, m_txSequence++);
    m_tcpClient->sendRaw(packet);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    // Handle clicks on VFO A square/mode label -> open mode popup for VFO A
    if ((watched == m_vfoASquare || watched == m_modeALabel) && event->type() == QEvent::MouseButtonPress) {
        // Toggle popup - close if open, otherwise show for VFO A
        if (m_modePopup->isVisible()) {
            m_modePopup->hidePopup();
        } else {
            m_modePopup->setFrequency(m_radioState->vfoA());
            m_modePopup->setCurrentMode(static_cast<int>(m_radioState->mode()));
            m_modePopup->setCurrentDataSubMode(m_radioState->dataSubMode());
            m_modePopup->setBSetEnabled(false); // Commands target VFO A
            // Arrow points to MAIN RX button
            m_modePopup->showAboveWidget(m_bottomMenuBar, m_bottomMenuBar->mainRxButton());
        }
        return true;
    }

    // Handle clicks on VFO B square/mode label -> open mode popup for VFO B
    if ((watched == m_vfoBSquare || watched == m_modeBLabel) && event->type() == QEvent::MouseButtonPress) {
        // Toggle popup - close if open, otherwise show for VFO B
        if (m_modePopup->isVisible()) {
            m_modePopup->hidePopup();
        } else {
            m_modePopup->setFrequency(m_radioState->vfoB());
            m_modePopup->setCurrentMode(static_cast<int>(m_radioState->modeB()));
            m_modePopup->setCurrentDataSubMode(m_radioState->dataSubModeB());
            m_modePopup->setBSetEnabled(true); // Commands target VFO B (MD$, DT$)
            // Arrow points to SUB RX button
            m_modePopup->showAboveWidget(m_bottomMenuBar, m_bottomMenuBar->subRxButton());
        }
        return true;
    }

    // Reposition span control buttons and VFO indicator when panadapter A resizes
    if (watched == m_panadapterA && event->type() == QEvent::Resize) {
        QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
        int w = resizeEvent->size().width();
        int h = resizeEvent->size().height();

        // Position buttons at lower right, above the frequency label bar (20px)
        // Triangle layout: C centered above, - and + below
        m_spanDownBtn->move(w - 70, h - 45);
        m_spanUpBtn->move(w - 35, h - 45);
        m_centerBtn->move(w - 52, h - 73);

        // VFO indicator at bottom-left corner
        m_vfoIndicatorA->move(0, h - 30);
    }

    // Reposition span control buttons and VFO indicator when panadapter B resizes
    if (watched == m_panadapterB && event->type() == QEvent::Resize) {
        QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
        int w = resizeEvent->size().width();
        int h = resizeEvent->size().height();

        // Position B buttons at lower right, above the frequency label bar (20px)
        // Triangle layout: C centered above, - and + below
        m_spanDownBtnB->move(w - 70, h - 45);
        m_spanUpBtnB->move(w - 35, h - 45);
        m_centerBtnB->move(w - 52, h - 73);

        // VFO indicator at bottom-left corner
        m_vfoIndicatorB->move(0, h - 30);
    }

    // Handle right-click on memory buttons (alternate actions)
    if (event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            if (watched == m_recBtn) {
                m_tcpClient->sendCAT("SW137;"); // BANK
                return true;
            } else if (watched == m_storeBtn) {
                m_tcpClient->sendCAT("SW138;"); // AF REC
                return true;
            } else if (watched == m_rclBtn) {
                m_tcpClient->sendCAT("SW139;"); // AF PLAY
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
}

void MainWindow::setPanadapterMode(PanadapterMode mode) {
    m_panadapterMode = mode;
    switch (mode) {
    case PanadapterMode::MainOnly:
        m_panadapterA->show();
        m_panadapterB->hide();
        break;
    case PanadapterMode::Dual:
        m_panadapterA->show();
        m_panadapterB->show();
        break;
    case PanadapterMode::SubOnly:
        m_panadapterA->hide();
        m_panadapterB->show();
        break;
    }
}

void MainWindow::showMenuOverlay() {
    // Close display popup if visible
    if (m_displayPopup && m_displayPopup->isVisible()) {
        m_displayPopup->hidePopup();
    }

    // Toggle menu overlay visibility
    if (m_spectrumContainer && m_menuOverlay) {
        if (m_menuOverlay->isVisible()) {
            // Hide the overlay
            m_menuOverlay->hide();
            if (m_bottomMenuBar) {
                m_bottomMenuBar->setMenuActive(false);
            }
        } else {
            // Show the overlay
            QPoint pos = m_spectrumContainer->mapTo(this, QPoint(0, 0));
            m_menuOverlay->setGeometry(pos.x(), pos.y(), m_spectrumContainer->width(), m_spectrumContainer->height());
            m_menuOverlay->show();
            m_menuOverlay->raise();

            // Set MENU button to active (inverse colors)
            if (m_bottomMenuBar) {
                m_bottomMenuBar->setMenuActive(true);
            }
        }
    }
}

void MainWindow::onMenuValueChangeRequested(int menuId, const QString &action) {
    // Build and send ME command
    // action: "+" = increment, "-" = decrement, "/" = toggle
    QString cmd = QString("ME%1.%2;").arg(menuId, 4, 10, QChar('0')).arg(action);
    qDebug() << "Menu value change:" << cmd;

    // For prototype: update local model immediately (optimistic update)
    MenuItem *item = m_menuModel->getMenuItem(menuId);
    if (item) {
        int newValue = item->currentValue;
        if (action == "+") {
            newValue = qMin(item->currentValue + item->step, item->maxValue);
        } else if (action == "-") {
            newValue = qMax(item->currentValue - item->step, item->minValue);
        } else if (action == "/") {
            // Toggle binary
            newValue = (item->currentValue == 0) ? 1 : 0;
        }
        m_menuModel->updateValue(menuId, newValue);
    }

    // When connected to radio, send the command
    if (m_tcpClient->isConnected()) {
        m_tcpClient->sendCAT(cmd);
    }
}

void MainWindow::toggleDisplayPopup() {
    bool wasVisible = m_displayPopup && m_displayPopup->isVisible();
    closeAllPopups();

    if (!wasVisible && m_displayPopup && m_bottomMenuBar) {
        m_displayPopup->showAboveButton(m_bottomMenuBar->displayButton());
        m_bottomMenuBar->setDisplayActive(true);
    }
}

void MainWindow::closeAllPopups() {
    // Close menu overlay
    if (m_menuOverlay && m_menuOverlay->isVisible()) {
        m_menuOverlay->hide();
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setMenuActive(false);
        }
    }

    // Close band popup
    if (m_bandPopup && m_bandPopup->isVisible()) {
        m_bandPopup->hidePopup();
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setBandActive(false);
        }
    }

    // Close display popup
    if (m_displayPopup && m_displayPopup->isVisible()) {
        m_displayPopup->hidePopup();
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setDisplayActive(false);
        }
    }

    // Close Fn popup
    if (m_fnPopup && m_fnPopup->isVisible()) {
        m_fnPopup->hidePopup();
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setFnActive(false);
        }
    }

    // Close Main RX popup
    if (m_mainRxPopup && m_mainRxPopup->isVisible()) {
        m_mainRxPopup->hidePopup();
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setMainRxActive(false);
        }
    }

    // Close Sub RX popup
    if (m_subRxPopup && m_subRxPopup->isVisible()) {
        m_subRxPopup->hidePopup();
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setSubRxActive(false);
        }
    }

    // Close TX popup
    if (m_txPopup && m_txPopup->isVisible()) {
        m_txPopup->hidePopup();
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setTxActive(false);
        }
    }
}

void MainWindow::toggleBandPopup() {
    bool wasVisible = m_bandPopup && m_bandPopup->isVisible();
    closeAllPopups();

    if (!wasVisible && m_bandPopup && m_bottomMenuBar) {
        m_bandPopup->showAboveButton(m_bottomMenuBar->bandButton());
        m_bottomMenuBar->setBandActive(true);
    }
}

void MainWindow::toggleFnPopup() {
    bool wasVisible = m_fnPopup && m_fnPopup->isVisible();
    closeAllPopups();

    if (!wasVisible && m_fnPopup && m_bottomMenuBar) {
        m_fnPopup->showAboveButton(m_bottomMenuBar->fnButton());
        m_bottomMenuBar->setFnActive(true);
    }
}

void MainWindow::toggleMainRxPopup() {
    bool wasVisible = m_mainRxPopup && m_mainRxPopup->isVisible();
    closeAllPopups();

    if (!wasVisible && m_mainRxPopup && m_bottomMenuBar) {
        m_mainRxPopup->showAboveButton(m_bottomMenuBar->mainRxButton());
        m_bottomMenuBar->setMainRxActive(true);
    }
}

void MainWindow::toggleSubRxPopup() {
    bool wasVisible = m_subRxPopup && m_subRxPopup->isVisible();
    closeAllPopups();

    if (!wasVisible && m_subRxPopup && m_bottomMenuBar) {
        m_subRxPopup->showAboveButton(m_bottomMenuBar->subRxButton());
        m_bottomMenuBar->setSubRxActive(true);
    }
}

void MainWindow::toggleTxPopup() {
    bool wasVisible = m_txPopup && m_txPopup->isVisible();
    closeAllPopups();

    if (!wasVisible && m_txPopup && m_bottomMenuBar) {
        m_txPopup->showAboveButton(m_bottomMenuBar->txButton());
        m_bottomMenuBar->setTxActive(true);
    }
}

void MainWindow::onBandSelected(const QString &bandName) {
    qDebug() << "Band selected:" << bandName;

    // Get band number from name
    int newBandNum = m_bandPopup->getBandNumber(bandName);

    // GEN and MEM are special modes, not band changes (-1 returned)
    if (newBandNum < 0) {
        qDebug() << "Special mode selected (GEN/MEM) - no BN command";
        return;
    }

    if (m_tcpClient->isConnected()) {
        if (newBandNum == m_currentBandNum) {
            // Same band tapped - invoke band stack
            qDebug() << "Same band - invoking band stack with BN^;";
            m_tcpClient->sendCAT("BN^;");
        } else {
            // Different band selected - change band
            QString cmd = QString("BN%1;").arg(newBandNum, 2, 10, QChar('0'));
            qDebug() << "Changing band:" << cmd;
            m_tcpClient->sendCAT(cmd);
        }
        // Request current band to update UI
        m_tcpClient->sendCAT("BN;");
    }
}

void MainWindow::updateBandSelection(int bandNum) {
    m_currentBandNum = bandNum;

    // Update the band popup to show the current band as selected
    if (m_bandPopup) {
        m_bandPopup->setSelectedBandByNumber(bandNum);
    }
}

// ============== KPOD Event Handlers ==============

void MainWindow::onKpodEncoderRotated(int ticks) {
    if (!m_tcpClient->isConnected()) {
        return;
    }

    // Action depends on rocker position
    switch (m_kpodDevice->rockerPosition()) {
    case KpodDevice::RockerLeft: // VFO A
        // Tune VFO A using UP/DN commands
        {
            QString cmd = (ticks > 0) ? "UP;" : "DN;";
            int count = qAbs(ticks);
            for (int i = 0; i < count; i++) {
                m_tcpClient->sendCAT(cmd);
            }
        }
        break;

    case KpodDevice::RockerCenter: // VFO B
        // Tune VFO B using UP$/DN$ commands (Sub RX)
        {
            QString cmd = (ticks > 0) ? "UP$;" : "DN$;";
            int count = qAbs(ticks);
            for (int i = 0; i < count; i++) {
                m_tcpClient->sendCAT(cmd);
            }
        }
        break;

    case KpodDevice::RockerRight: // RIT/XIT
        // Adjust RIT/XIT offset using RU/RD commands
        {
            QString cmd = (ticks > 0) ? "RU;" : "RD;";
            int count = qAbs(ticks);
            for (int i = 0; i < count; i++) {
                m_tcpClient->sendCAT(cmd);
            }
        }
        break;
    }
}

void MainWindow::onKpodRockerChanged(int position) {
    QString posName;
    switch (static_cast<KpodDevice::RockerPosition>(position)) {
    case KpodDevice::RockerLeft:
        posName = "VFO A";
        break;
    case KpodDevice::RockerCenter:
        posName = "VFO B";
        break;
    case KpodDevice::RockerRight:
        posName = "XIT/RIT";
        break;
    default:
        posName = "Unknown";
        break;
    }
    Q_UNUSED(posName)
}

void MainWindow::onKpodPollError(const QString &error) {
    qWarning() << "KPOD error:" << error;
}

void MainWindow::onKpodEnabledChanged(bool enabled) {
    if (enabled) {
        if (m_kpodDevice->isDetected()) {
            m_kpodDevice->startPolling();
        }
    } else {
        m_kpodDevice->stopPolling();
    }
}

// ============== K4 Error/Notification Slots ==============

void MainWindow::onErrorNotification(int errorCode, const QString &message) {
    Q_UNUSED(errorCode)
    // Show the notification message in a centered popup
    // The message contains the text after "ERxx:" (e.g., "KPA1500 Status: operate.")
    if (m_notificationWidget) {
        m_notificationWidget->showMessage(message, 2000);
    }
}

// ============== KPA1500 Amplifier Slots ==============

void MainWindow::onKpa1500Connected() {
    qDebug() << "KPA1500: Connected to amplifier";
    // Start polling with configured interval
    int pollInterval = RadioSettings::instance()->kpa1500PollInterval();
    m_kpa1500Client->startPolling(pollInterval);
    updateKpa1500Status();
}

void MainWindow::onKpa1500Disconnected() {
    qDebug() << "KPA1500: Disconnected from amplifier";
    updateKpa1500Status();
}

void MainWindow::onKpa1500Error(const QString &error) {
    qWarning() << "KPA1500: Error -" << error;
}

void MainWindow::onKpa1500EnabledChanged(bool enabled) {
    if (enabled) {
        // Connect if host is configured
        QString host = RadioSettings::instance()->kpa1500Host();
        if (!host.isEmpty()) {
            m_kpa1500Client->connectToHost(host, RadioSettings::instance()->kpa1500Port());
        }
    } else {
        m_kpa1500Client->disconnectFromHost();
    }
    updateKpa1500Status();
}

void MainWindow::onKpa1500SettingsChanged() {
    // Reconnect with new settings if enabled
    if (RadioSettings::instance()->kpa1500Enabled()) {
        m_kpa1500Client->disconnectFromHost();
        QString host = RadioSettings::instance()->kpa1500Host();
        if (!host.isEmpty()) {
            m_kpa1500Client->connectToHost(host, RadioSettings::instance()->kpa1500Port());
        }
    }
    updateKpa1500Status();
}

void MainWindow::updateKpa1500Status() {
    bool enabled = RadioSettings::instance()->kpa1500Enabled();
    bool connected = m_kpa1500Client && m_kpa1500Client->isConnected();

    if (!enabled) {
        m_kpa1500StatusLabel->hide();
    } else {
        m_kpa1500StatusLabel->show();
        if (connected) {
            m_kpa1500StatusLabel->setText("KPA1500 Connected");
            m_kpa1500StatusLabel->setStyleSheet(
                QString("color: %1; font-size: 12px; font-weight: bold;").arg(K4Colors::AgcGreen));
        } else {
            m_kpa1500StatusLabel->setText("KPA1500 Not Connected");
            m_kpa1500StatusLabel->setStyleSheet(
                QString("color: %1; font-size: 12px; font-weight: bold;").arg(K4Colors::TxRed));
        }
    }
}

// ============== Fn Popup / Macro Slots ==============

void MainWindow::onFnFunctionTriggered(const QString &functionId) {
    qDebug() << "Fn function triggered:" << functionId;

    // Handle built-in functions
    if (functionId == MacroIds::ScrnCap) {
        // SS0; triggers K4 screenshot (saved to internal SD card)
        if (m_tcpClient && m_tcpClient->isConnected()) {
            m_tcpClient->sendCAT("SS0;");
            qDebug() << "Screenshot captured (SS0;)";
        }
    } else if (functionId == MacroIds::Macros) {
        openMacroDialog();
    } else if (functionId == MacroIds::SwList) {
        // TODO: Show software list
        qDebug() << "Software list - not yet implemented";
    } else if (functionId == MacroIds::Update) {
        // TODO: Check for updates
        qDebug() << "Update check - not yet implemented";
    } else if (functionId == MacroIds::DxList) {
        // TODO: Show DX list
        qDebug() << "DX list - not yet implemented";
    } else {
        // User-configurable macro - execute CAT command
        executeMacro(functionId);
    }
}

void MainWindow::executeMacro(const QString &functionId) {
    MacroEntry macro = RadioSettings::instance()->macro(functionId);
    if (!macro.command.isEmpty()) {
        qDebug() << "Executing macro" << functionId << ":" << macro.command;
        if (m_tcpClient && m_tcpClient->isConnected()) {
            m_tcpClient->sendCAT(macro.command);
        }
    } else {
        qDebug() << "No macro configured for" << functionId;
    }
}

void MainWindow::openMacroDialog() {
    if (m_macroDialog) {
        // Close any open popups first
        closeAllPopups();

        // Size to fill the spectrum container (same as menu overlay)
        if (m_spectrumContainer) {
            QPoint pos = m_spectrumContainer->mapTo(this, QPoint(0, 0));
            m_macroDialog->setGeometry(pos.x(), pos.y(), m_spectrumContainer->width(), m_spectrumContainer->height());
        }

        m_macroDialog->show();
        m_macroDialog->raise();
        m_macroDialog->setFocus();
    }
}
