#include "mainwindow.h"
#include "ui/radiomanagerdialog.h"
#include "ui/sidecontrolpanel.h"
#include "ui/rightsidepanel.h"
#include "ui/bottommenubar.h"
#include "ui/menuoverlay.h"
#include "ui/bandpopupwidget.h"
#include "ui/displaypopupwidget.h"
#include "ui/optionsdialog.h"
#include "models/menumodel.h"
#include "dsp/panadapter.h"
#include "dsp/minipanwidget.h"
#include "audio/audioengine.h"
#include "audio/opusdecoder.h"
#include "hardware/kpoddevice.h"
#include "network/kpa1500client.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QAction>
#include <QDebug>
#include <QFont>
#include <QDateTime>
#include <QPainter>
#include <QFrame>
#include <QEvent>
#include <QResizeEvent>
#include <QMouseEvent>

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

// ============== MainWindow Implementation ==============
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_tcpClient(new TcpClient(this)), m_radioState(new RadioState(this)),
      m_clockTimer(new QTimer(this)), m_audioEngine(new AudioEngine(this)), m_opusDecoder(new OpusDecoder(this)),
      m_menuModel(new MenuModel(this)), m_menuOverlay(nullptr) {
    // Initialize Opus decoder (K4 sends 12kHz stereo: left=Main, right=Sub)
    m_opusDecoder->initialize(12000, 2);
    setupMenuBar();
    setupUi();

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
    connect(m_radioState, &RadioState::sMeterChanged, this, &MainWindow::onSMeterChanged);
    connect(m_radioState, &RadioState::filterBandwidthChanged, this, &MainWindow::onBandwidthChanged);

    // RadioState signals -> UI updates (VFO B)
    connect(m_radioState, &RadioState::frequencyBChanged, this, &MainWindow::onFrequencyBChanged);
    connect(m_radioState, &RadioState::modeBChanged, this, &MainWindow::onModeBChanged);
    connect(m_radioState, &RadioState::sMeterBChanged, this, &MainWindow::onSMeterBChanged);
    connect(m_radioState, &RadioState::filterBandwidthBChanged, this, &MainWindow::onBandwidthBChanged);

    // RadioState signals -> Status bar updates
    connect(m_radioState, &RadioState::rfPowerChanged, this, &MainWindow::onRfPowerChanged);
    connect(m_radioState, &RadioState::supplyVoltageChanged, this, &MainWindow::onSupplyVoltageChanged);
    connect(m_radioState, &RadioState::supplyCurrentChanged, this, &MainWindow::onSupplyCurrentChanged);
    connect(m_radioState, &RadioState::swrChanged, this, &MainWindow::onSwrChanged);

    // RadioState signals -> Side control panel updates (BW/SHFT/HI/LO)
    // Helper to update all 4 filter display values (called on BW or SHFT change)
    auto updateFilterDisplay = [this]() {
        int bwHz = m_radioState->filterBandwidth();
        int shiftHz = m_radioState->shiftHz(); // raw IS value × 10

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
    connect(m_radioState, &RadioState::keyerSpeedChanged, m_sideControlPanel, &SideControlPanel::setWpm);
    connect(m_radioState, &RadioState::cwPitchChanged, this, [this](int pitch) {
        m_sideControlPanel->setPitch(pitch / 1000.0); // Hz to kHz (500Hz = 0.50)
    });
    connect(m_radioState, &RadioState::rfPowerChanged, this,
            [this](double watts, bool) { m_sideControlPanel->setPower(static_cast<int>(watts)); });
    connect(m_radioState, &RadioState::qskDelayChanged, this, [this](int delay) {
        m_sideControlPanel->setDelay(delay / 100.0); // 10ms units to seconds (20 -> 0.20)
    });
    connect(m_radioState, &RadioState::rfGainChanged, m_sideControlPanel, &SideControlPanel::setMainRfGain);
    connect(m_radioState, &RadioState::squelchChanged, m_sideControlPanel, &SideControlPanel::setMainSquelch);
    connect(m_radioState, &RadioState::rfGainBChanged, m_sideControlPanel, &SideControlPanel::setSubRfGain);
    connect(m_radioState, &RadioState::squelchBChanged, m_sideControlPanel, &SideControlPanel::setSubSquelch);

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

    // RadioState span -> Panadapter (for frequency labels and bin extraction)
    connect(m_radioState, &RadioState::spanChanged, this, [this](int spanHz) { m_panadapterA->setSpan(spanHz); });
    connect(m_radioState, &RadioState::spanBChanged, this, [this](int spanHz) { m_panadapterB->setSpan(spanHz); });

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
    connect(m_radioState, &RadioState::ddcNbModeChanged, m_displayPopup, &DisplayPopupWidget::setDdcNbMode);
    connect(m_radioState, &RadioState::ddcNbLevelChanged, m_displayPopup, &DisplayPopupWidget::setDdcNbLevel);
    // Also update span/ref values in popup
    connect(m_radioState, &RadioState::spanChanged, this, [this](int spanHz) {
        m_displayPopup->setSpanValueA(spanHz / 1000.0); // Hz to kHz
    });
    connect(m_radioState, &RadioState::spanBChanged, this, [this](int spanHz) {
        m_displayPopup->setSpanValueB(spanHz / 1000.0); // Hz to kHz
    });
    connect(m_radioState, &RadioState::refLevelChanged, m_displayPopup, &DisplayPopupWidget::setRefLevelValueA);
    connect(m_radioState, &RadioState::refLevelBChanged, m_displayPopup, &DisplayPopupWidget::setRefLevelValueB);

    // Averaging control +/- -> CAT commands
    connect(m_displayPopup, &DisplayPopupWidget::averagingIncrementRequested, this, [this]() {
        // Cycle up: 1->5->10->15->20
        int current = m_radioState->averaging();
        int next = current;
        if (current < 5)
            next = 5;
        else if (current < 10)
            next = 10;
        else if (current < 15)
            next = 15;
        else if (current < 20)
            next = 20;
        else
            next = 20; // Already at max
        m_tcpClient->sendCAT(QString("#AVG%1;").arg(next, 2, 10, QChar('0')));
    });
    connect(m_displayPopup, &DisplayPopupWidget::averagingDecrementRequested, this, [this]() {
        // Cycle down: 20->15->10->5->1
        int current = m_radioState->averaging();
        int next = current;
        if (current > 15)
            next = 15;
        else if (current > 10)
            next = 10;
        else if (current > 5)
            next = 5;
        else if (current > 1)
            next = 1;
        else
            next = 1; // Already at min
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

    // Span control from display popup -> CAT commands (respects A/B selection)
    connect(m_displayPopup, &DisplayPopupWidget::spanIncrementRequested, this, [this]() {
        bool vfoA = m_displayPopup->isVfoAEnabled();
        bool vfoB = m_displayPopup->isVfoBEnabled();
        // Use A as reference if both selected, else use selected VFO
        int currentSpan = (vfoB && !vfoA) ? m_radioState->spanHzB() : m_radioState->spanHz();
        int newSpan = currentSpan + 1000;
        if (newSpan <= 368000) {
            // Target whichever VFO(s) are enabled
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
        // Use A as reference if both selected, else use selected VFO
        int currentSpan = (vfoB && !vfoA) ? m_radioState->spanHzB() : m_radioState->spanHz();
        int newSpan = currentSpan - 1000;
        if (newSpan >= 5000) {
            // Target whichever VFO(s) are enabled
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

    // Defer resize to after layouts are finalized (fixes initial window size issue)
    QTimer::singleShot(0, this, [this]() { resize(1340, 800); });
}

MainWindow::~MainWindow() {}

void MainWindow::setupMenuBar() {
    // Match K4 menu style: Connect, File, Tools, View
    menuBar()->setStyleSheet(QString("QMenuBar { background-color: %1; color: %2; }"
                                     "QMenuBar::item:selected { background-color: #333; }")
                                 .arg(K4Colors::DarkBackground, K4Colors::TextWhite));

    QMenu *connectMenu = menuBar()->addMenu("Connect");
    QAction *radiosAction = new QAction("Radios...", this);
    connect(radiosAction, &QAction::triggered, this, &MainWindow::showRadioManager);
    connectMenu->addAction(radiosAction);

    QMenu *optionsMenu = menuBar()->addMenu("Options");
    QAction *optionsAction = new QAction("Settings...", this);
    optionsAction->setMenuRole(QAction::NoRole); // Prevent macOS from moving to app menu
    connect(optionsAction, &QAction::triggered, this, [this]() {
        OptionsDialog dialog(m_radioState, m_kpa1500Client, this);
        dialog.exec();
    });
    optionsMenu->addAction(optionsAction);

    QMenu *fileMenu = menuBar()->addMenu("File");
    Q_UNUSED(fileMenu)

    QMenu *toolsMenu = menuBar()->addMenu("Tools");
    Q_UNUSED(toolsMenu)

    QMenu *viewMenu = menuBar()->addMenu("View");
    Q_UNUSED(viewMenu)
}

void MainWindow::setupUi() {
    setWindowTitle("K4Controller");
    setMinimumSize(1340, 800);
    resize(1340, 800); // Default to minimum size on launch
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

    // Bottom Menu Bar
    m_bottomMenuBar = new BottomMenuBar(centralWidget);
    mainLayout->addWidget(m_bottomMenuBar);

    // Connect side control panel icon buttons
    connect(m_sideControlPanel, &SideControlPanel::connectClicked, this, &MainWindow::showRadioManager);
    connect(m_sideControlPanel, &SideControlPanel::helpClicked, this, []() {
        // TODO: Show help dialog
    });

    // Connect volume slider to audio engine
    connect(m_sideControlPanel, &SideControlPanel::volumeChanged, this, [this](int value) {
        if (m_audioEngine) {
            m_audioEngine->setVolume(value / 100.0f);
        }
        RadioSettings::instance()->setVolume(value); // Persist setting
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
    connect(m_rightSidePanel, &RightSidePanel::modeClicked, this, [this]() { m_tcpClient->sendCAT("SW43;"); });

    // Secondary (right-click) signals
    connect(m_rightSidePanel, &RightSidePanel::attnClicked, this, [this]() { m_tcpClient->sendCAT("SW141;"); });
    connect(m_rightSidePanel, &RightSidePanel::levelClicked, this, [this]() { m_tcpClient->sendCAT("SW142;"); });
    connect(m_rightSidePanel, &RightSidePanel::adjClicked, this, [this]() { m_tcpClient->sendCAT("SW143;"); });
    connect(m_rightSidePanel, &RightSidePanel::manualClicked, this, [this]() { m_tcpClient->sendCAT("SW140;"); });
    connect(m_rightSidePanel, &RightSidePanel::apfClicked, this, [this]() { m_tcpClient->sendCAT("SW144;"); });
    connect(m_rightSidePanel, &RightSidePanel::splitClicked, this, [this]() { m_tcpClient->sendCAT("SW145;"); });
    connect(m_rightSidePanel, &RightSidePanel::btoaClicked, this, [this]() { m_tcpClient->sendCAT("SW147;"); });
    connect(m_rightSidePanel, &RightSidePanel::autoClicked, this, [this]() { m_tcpClient->sendCAT("SW146;"); });
    connect(m_rightSidePanel, &RightSidePanel::altClicked, this, [this]() { m_tcpClient->sendCAT("SW148;"); });

    // PF row primary (left-click) signals
    connect(m_rightSidePanel, &RightSidePanel::bsetClicked, this, [this]() { m_tcpClient->sendCAT("SW44;"); });
    connect(m_rightSidePanel, &RightSidePanel::clrClicked, this, [this]() { m_tcpClient->sendCAT("SW64;"); });
    connect(m_rightSidePanel, &RightSidePanel::ritClicked, this, [this]() { m_tcpClient->sendCAT("SW54;"); });
    connect(m_rightSidePanel, &RightSidePanel::xitClicked, this, [this]() { m_tcpClient->sendCAT("SW74;"); });

    // PF row secondary (right-click) signals
    connect(m_rightSidePanel, &RightSidePanel::pf1Clicked, this, [this]() { m_tcpClient->sendCAT("SW153;"); });
    connect(m_rightSidePanel, &RightSidePanel::pf2Clicked, this, [this]() { m_tcpClient->sendCAT("SW154;"); });
    connect(m_rightSidePanel, &RightSidePanel::pf3Clicked, this, [this]() { m_tcpClient->sendCAT("SW155;"); });
    connect(m_rightSidePanel, &RightSidePanel::pf4Clicked, this, [this]() { m_tcpClient->sendCAT("SW156;"); });

    // Connect bottom menu bar signals
    connect(m_bottomMenuBar, &BottomMenuBar::menuClicked, this, &MainWindow::showMenuOverlay);
    connect(m_bottomMenuBar, &BottomMenuBar::fnClicked, this, []() {
        // TODO: Show function key popup
    });
    connect(m_bottomMenuBar, &BottomMenuBar::displayClicked, this, &MainWindow::toggleDisplayPopup);
    connect(m_bottomMenuBar, &BottomMenuBar::bandClicked, this, &MainWindow::showBandPopup);
    connect(m_bottomMenuBar, &BottomMenuBar::mainRxClicked, this, []() {
        // TODO: Show main RX settings
    });
    connect(m_bottomMenuBar, &BottomMenuBar::subRxClicked, this, []() {
        // TODO: Show sub RX settings
    });
    connect(m_bottomMenuBar, &BottomMenuBar::txClicked, this, []() {
        // TODO: Show TX settings
    });
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
    connect(m_vfoA->miniPan(), &MiniPanWidget::clicked, this, [this]() {
        m_radioState->setMiniPanAEnabled(false); // Set state BEFORE sending CAT
        m_tcpClient->sendCAT("#MP0;");           // Disable Mini-Pan A streaming
    });

    // Set Mini-Pan A colors to blue (matching main panadapter A passband)
    m_vfoA->miniPan()->setSpectrumColor(QColor(0, 128, 255));     // Blue spectrum
    m_vfoA->miniPan()->setPassbandColor(QColor(0, 128, 255, 64)); // Blue passband

    layout->addWidget(m_vfoA, 1);

    // ===== Center Section =====
    auto *centerWidget = new QWidget(parent);
    centerWidget->setFixedWidth(200);
    centerWidget->setStyleSheet(QString("background-color: %1;").arg(K4Colors::Background));
    auto *centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(4, 4, 4, 4);
    centerLayout->setSpacing(3);

    // Row 1: [A + Mode] [triangle space] TX [triangle space] [B + Mode]
    // Triangle moves between left and right side of TX based on split state
    // Both sides have equal fixed-width spaces for symmetry
    auto *txRow = new QHBoxLayout();
    txRow->setSpacing(2);

    // A column: square + mode below (in fixed-width container for proper centering)
    auto *vfoAContainer = new QWidget(centerWidget);
    vfoAContainer->setFixedWidth(45); // Match mode label width for consistent centering
    auto *vfoAColumn = new QVBoxLayout(vfoAContainer);
    vfoAColumn->setContentsMargins(0, 0, 0, 0);
    vfoAColumn->setSpacing(2);

    m_vfoASquare = new QLabel("A", vfoAContainer);
    m_vfoASquare->setFixedSize(30, 30);
    m_vfoASquare->setAlignment(Qt::AlignCenter);
    m_vfoASquare->setStyleSheet(
        QString("background-color: %1; color: %2; font-size: 16px; font-weight: bold; border-radius: 4px;")
            .arg(K4Colors::VfoBCyan, K4Colors::DarkBackground));
    vfoAColumn->addWidget(m_vfoASquare, 0, Qt::AlignHCenter);

    m_modeALabel = new QLabel("USB", vfoAContainer);
    m_modeALabel->setFixedWidth(45); // Wide enough for "DATA-R"
    m_modeALabel->setAlignment(Qt::AlignCenter);
    m_modeALabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Colors::TextWhite));
    vfoAColumn->addWidget(m_modeALabel, 0, Qt::AlignHCenter);

    txRow->addWidget(vfoAContainer);
    txRow->addStretch();   // Push center elements away from A
    txRow->addSpacing(15); // Extra spacing for filter UI room

    // Center TX area container (TEST indicator above TX/triangles)
    auto *txCenterContainer = new QWidget(centerWidget);
    auto *txCenterVLayout = new QVBoxLayout(txCenterContainer);
    txCenterVLayout->setContentsMargins(0, 0, 0, 0);
    txCenterVLayout->setSpacing(0);

    // TEST indicator - large red, hidden by default
    m_testLabel = new QLabel("TEST", txCenterContainer);
    m_testLabel->setAlignment(Qt::AlignCenter);
    m_testLabel->setStyleSheet("color: #FF0000; font-size: 14px; font-weight: bold;");
    m_testLabel->setVisible(false); // Hidden until test mode is enabled
    txCenterVLayout->addWidget(m_testLabel);

    // TX row (triangles + TX label)
    auto *txIndicatorRow = new QHBoxLayout();
    txIndicatorRow->setSpacing(0);

    // Left triangle space - visible when split is OFF (pointing at A)
    // Uses fixed size container to maintain space even when hidden
    m_txTriangle = new QLabel("◀", txCenterContainer);
    m_txTriangle->setFixedSize(24, 24);
    m_txTriangle->setAlignment(Qt::AlignCenter);
    m_txTriangle->setStyleSheet(QString("color: %1; font-size: 18px;").arg(K4Colors::VfoAAmber));
    txIndicatorRow->addWidget(m_txTriangle);

    // TX indicator in orange
    m_txIndicator = new QLabel("TX", txCenterContainer);
    m_txIndicator->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold;").arg(K4Colors::VfoAAmber));
    txIndicatorRow->addWidget(m_txIndicator);

    // Right triangle space - visible when split is ON (pointing at B)
    // Uses fixed size container to maintain space even when hidden
    m_txTriangleB = new QLabel("", txCenterContainer); // Empty by default
    m_txTriangleB->setFixedSize(24, 24);
    m_txTriangleB->setAlignment(Qt::AlignCenter);
    m_txTriangleB->setStyleSheet(QString("color: %1; font-size: 18px;").arg(K4Colors::VfoAAmber));
    txIndicatorRow->addWidget(m_txTriangleB);

    txCenterVLayout->addLayout(txIndicatorRow);
    txRow->addWidget(txCenterContainer);
    txRow->addSpacing(15); // Extra spacing for filter UI room
    txRow->addStretch();   // Push center elements away from B

    // B column: square + mode below (in fixed-width container for proper centering)
    auto *vfoBContainer = new QWidget(centerWidget);
    vfoBContainer->setFixedWidth(45); // Match mode label width for consistent centering
    auto *vfoBColumn = new QVBoxLayout(vfoBContainer);
    vfoBColumn->setContentsMargins(0, 0, 0, 0);
    vfoBColumn->setSpacing(2);

    m_vfoBSquare = new QLabel("B", vfoBContainer);
    m_vfoBSquare->setFixedSize(30, 30);
    m_vfoBSquare->setAlignment(Qt::AlignCenter);
    m_vfoBSquare->setStyleSheet(
        QString("background-color: %1; color: %2; font-size: 16px; font-weight: bold; border-radius: 4px;")
            .arg(K4Colors::AgcGreen, K4Colors::DarkBackground));
    vfoBColumn->addWidget(m_vfoBSquare, 0, Qt::AlignHCenter);

    m_modeBLabel = new QLabel("USB", vfoBContainer);
    m_modeBLabel->setFixedWidth(45); // Wide enough for "DATA-R"
    m_modeBLabel->setAlignment(Qt::AlignCenter);
    m_modeBLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Colors::TextWhite));
    vfoBColumn->addWidget(m_modeBLabel, 0, Qt::AlignHCenter);

    txRow->addWidget(vfoBContainer);

    txRow->setAlignment(Qt::AlignCenter);
    centerLayout->addLayout(txRow);

    // SPLIT indicator
    m_splitLabel = new QLabel("SPLIT OFF", centerWidget);
    m_splitLabel->setAlignment(Qt::AlignCenter);
    m_splitLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(K4Colors::VfoAAmber));
    centerLayout->addWidget(m_splitLabel);

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
    ritXitLayout->setContentsMargins(4, 2, 4, 2);
    ritXitLayout->setSpacing(1);

    auto *ritXitLabelsRow = new QHBoxLayout();
    ritXitLabelsRow->setSpacing(8);

    m_ritLabel = new QLabel("RIT", m_ritXitBox);
    m_ritLabel->setStyleSheet(QString("color: %1; font-size: 10px; border: none;").arg(K4Colors::InactiveGray));
    ritXitLabelsRow->addWidget(m_ritLabel);

    m_xitLabel = new QLabel("XIT", m_ritXitBox);
    m_xitLabel->setStyleSheet(QString("color: %1; font-size: 10px; border: none;").arg(K4Colors::InactiveGray));
    ritXitLabelsRow->addWidget(m_xitLabel);

    ritXitLabelsRow->setAlignment(Qt::AlignCenter);
    ritXitLayout->addLayout(ritXitLabelsRow);

    // Separator line between labels and value
    auto *ritXitSeparator = new QFrame(m_ritXitBox);
    ritXitSeparator->setFrameShape(QFrame::HLine);
    ritXitSeparator->setFrameShadow(QFrame::Plain);
    ritXitSeparator->setStyleSheet(QString("background-color: %1; border: none;").arg(K4Colors::InactiveGray));
    ritXitSeparator->setFixedHeight(1);
    ritXitLayout->addWidget(ritXitSeparator);

    m_ritXitValueLabel = new QLabel("+0.00", m_ritXitBox);
    m_ritXitValueLabel->setAlignment(Qt::AlignCenter);
    m_ritXitValueLabel->setStyleSheet(
        QString("color: %1; font-size: 14px; font-weight: bold; border: none;").arg(K4Colors::TextWhite));
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

    // ATU indicator (orange, visible when AT=2/AUTO)
    m_atuLabel = new QLabel("ATU", centerWidget);
    m_atuLabel->setAlignment(Qt::AlignCenter);
    m_atuLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;").arg(K4Colors::VfoAAmber));
    m_atuLabel->setVisible(false);
    centerLayout->addWidget(m_atuLabel, 0, Qt::AlignHCenter);

    centerLayout->addStretch();
    layout->addWidget(centerWidget);

    // ===== VFO B (Right - Cyan) - Using VFOWidget =====
    m_vfoB = new VFOWidget(VFOWidget::VFO_B, parent);

    // Set Mini-Pan B colors to green (matching main panadapter B passband)
    m_vfoB->miniPan()->setSpectrumColor(QColor(0, 200, 0));     // Green spectrum
    m_vfoB->miniPan()->setPassbandColor(QColor(0, 255, 0, 64)); // Green passband

    // Connect VFO B click to toggle mini-pan (send CAT to enable Mini-Pan streaming)
    connect(m_vfoB, &VFOWidget::normalContentClicked, this, [this]() {
        m_vfoB->showMiniPan();
        m_radioState->setMiniPanBEnabled(true); // Set state BEFORE sending CAT (K4 doesn't echo)
        m_tcpClient->sendCAT("#MP$1;");         // Enable Mini-Pan B (Sub RX) streaming
    });
    connect(m_vfoB->miniPan(), &MiniPanWidget::clicked, this, [this]() {
        m_radioState->setMiniPanBEnabled(false); // Set state BEFORE sending CAT
        m_tcpClient->sendCAT("#MP$0;");          // Disable Mini-Pan B streaming
    });

    layout->addWidget(m_vfoB, 1);

    // Add the VFO row to main layout
    mainVLayout->addWidget(vfoRowWidget);

    // ===== Antenna Row (below VFO section, centered) =====
    auto *antennaRow = new QHBoxLayout();
    antennaRow->setContentsMargins(8, 0, 8, 0);
    antennaRow->setSpacing(0);

    antennaRow->addStretch(1); // Push content toward center

    // RX Antenna A (Main) - white color, left of VOX
    m_rxAntALabel = new QLabel("1:ANT1", parent);
    m_rxAntALabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_rxAntALabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Colors::TextWhite));
    antennaRow->addWidget(m_rxAntALabel);

    antennaRow->addSpacing(8); // Small gap between RX A and VOX

    // VOX indicator - orange when on, grey when off
    m_voxLabel = new QLabel("VOX", parent);
    m_voxLabel->setAlignment(Qt::AlignCenter);
    m_voxLabel->setStyleSheet("color: #999999; font-size: 11px; font-weight: bold;");
    antennaRow->addWidget(m_voxLabel);

    antennaRow->addSpacing(8); // Small gap between VOX and QSK

    // QSK indicator - white when on, grey when off
    m_qskLabel = new QLabel("QSK", parent);
    m_qskLabel->setAlignment(Qt::AlignCenter);
    m_qskLabel->setStyleSheet("color: #999999; font-size: 11px; font-weight: bold;");
    antennaRow->addWidget(m_qskLabel);

    antennaRow->addSpacing(32); // Spacing between QSK and TX antenna

    // TX Antenna - orange color, centered
    m_txAntennaLabel = new QLabel("1:ANT1", parent);
    m_txAntennaLabel->setAlignment(Qt::AlignCenter);
    m_txAntennaLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Colors::VfoAAmber));
    antennaRow->addWidget(m_txAntennaLabel);

    antennaRow->addSpacing(8); // Small gap between TX and RX B

    // RX Antenna B (Sub) - white color, right of TX antenna
    m_rxAntBLabel = new QLabel("1:ANT1", parent);
    m_rxAntBLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_rxAntBLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Colors::TextWhite));
    antennaRow->addWidget(m_rxAntBLabel);

    antennaRow->addStretch(1); // Push content toward center

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

    // Main panadapter for VFO A (left side)
    m_panadapterA = new PanadapterWidget(m_spectrumContainer);
    m_panadapterA->setSpectrumColor(QColor(K4Colors::VfoAAmber));
    m_panadapterA->setDbRange(-140.0f, -20.0f);
    m_panadapterA->setSpectrumRatio(0.35f);
    m_panadapterA->setGridEnabled(true);
    layout->addWidget(m_panadapterA);

    // Sub panadapter for VFO B (right side)
    m_panadapterB = new PanadapterWidget(m_spectrumContainer);
    m_panadapterB->setSpectrumColor(QColor(K4Colors::VfoBCyan));
    m_panadapterB->setDbRange(-140.0f, -20.0f);
    m_panadapterB->setSpectrumRatio(0.35f);
    m_panadapterB->setGridEnabled(true);
    m_panadapterB->setPassbandColor(QColor(0, 200, 0));        // Green passband for VFO B
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

    // Position buttons (will be repositioned in resizeEvent of panadapter)
    // Triangle layout: C centered above, - and + below
    m_spanDownBtn->move(m_panadapterA->width() - 70, m_panadapterA->height() - 45);
    m_spanUpBtn->move(m_panadapterA->width() - 35, m_panadapterA->height() - 45);
    m_centerBtn->move(m_panadapterA->width() - 52, m_panadapterA->height() - 73);

    m_spanDownBtnB->move(m_panadapterB->width() - 70, m_panadapterB->height() - 45);
    m_spanUpBtnB->move(m_panadapterB->width() - 35, m_panadapterB->height() - 45);
    m_centerBtnB->move(m_panadapterB->width() - 52, m_panadapterB->height() - 73);

    // Span adjustment for Main: 1 kHz increments, range 5 kHz to 368 kHz
    // Uses OPTIMISTIC UPDATES like K4Mobile - update local state immediately
    connect(m_spanDownBtn, &QPushButton::clicked, this, [this]() {
        int currentSpan = m_radioState->spanHz();
        int newSpan = currentSpan - 1000; // -1 kHz
        qDebug() << "Span- clicked: current=" << currentSpan << "new=" << newSpan;
        if (newSpan >= 5000) {
            m_radioState->setSpanHz(newSpan);
            QString cmd = QString("#SPN%1;").arg(newSpan);
            qDebug() << "Sending span command:" << cmd;
            m_tcpClient->sendCAT(cmd);
        }
    });

    connect(m_spanUpBtn, &QPushButton::clicked, this, [this]() {
        int currentSpan = m_radioState->spanHz();
        int newSpan = currentSpan + 1000; // +1 kHz
        qDebug() << "Span+ clicked: current=" << currentSpan << "new=" << newSpan;
        if (newSpan <= 368000) {
            m_radioState->setSpanHz(newSpan);
            QString cmd = QString("#SPN%1;").arg(newSpan);
            qDebug() << "Sending span command:" << cmd;
            m_tcpClient->sendCAT(cmd);
        }
    });

    connect(m_centerBtn, &QPushButton::clicked, this, [this]() {
        qDebug() << "Center clicked: sending FC;";
        m_tcpClient->sendCAT("FC;");
    });

    // Span adjustment for Sub: uses $ suffix for Sub RX commands
    connect(m_spanDownBtnB, &QPushButton::clicked, this, [this]() {
        int currentSpan = m_radioState->spanHzB();
        int newSpan = currentSpan - 1000;
        qDebug() << "Span B- clicked: current=" << currentSpan << "new=" << newSpan;
        if (newSpan >= 5000) {
            m_radioState->setSpanHzB(newSpan);
            QString cmd = QString("#SPN$%1;").arg(newSpan);
            qDebug() << "Sending span B command:" << cmd;
            m_tcpClient->sendCAT(cmd);
        }
    });

    connect(m_spanUpBtnB, &QPushButton::clicked, this, [this]() {
        int currentSpan = m_radioState->spanHzB();
        int newSpan = currentSpan + 1000;
        qDebug() << "Span B+ clicked: current=" << currentSpan << "new=" << newSpan;
        if (newSpan <= 368000) {
            m_radioState->setSpanHzB(newSpan);
            QString cmd = QString("#SPN$%1;").arg(newSpan);
            qDebug() << "Sending span B command:" << cmd;
            m_tcpClient->sendCAT(cmd);
        }
    });

    connect(m_centerBtnB, &QPushButton::clicked, this, [this]() {
        qDebug() << "Center B clicked: sending FC$;";
        m_tcpClient->sendCAT("FC$;");
    });

    // Install event filter to reposition span buttons when panadapters resize
    m_panadapterA->installEventFilter(this);
    m_panadapterB->installEventFilter(this);

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
        // Update mini-pan too
        m_vfoA->miniPan()->setNotchFilter(enabled, pitch);
        // Update NTCH indicator in VFO processing row
        m_vfoA->setNotch(m_radioState->autoNotchEnabled(), m_radioState->manualNotchEnabled());
    });
    // Also update mini-pan mode when mode changes
    connect(m_radioState, &RadioState::modeChanged, this,
            [this](RadioState::Mode mode) { m_vfoA->miniPan()->setMode(RadioState::modeToString(mode)); });

    // Mini-pan filter passband visualization
    connect(m_radioState, &RadioState::filterBandwidthChanged, this,
            [this](int bw) { m_vfoA->miniPan()->setFilterBandwidth(bw); });
    connect(m_radioState, &RadioState::ifShiftChanged, this,
            [this](int shift) { m_vfoA->miniPan()->setIfShift(shift); });
    connect(m_radioState, &RadioState::cwPitchChanged, this,
            [this](int pitch) { m_vfoA->miniPan()->setCwPitch(pitch); });

    // Mouse control: click to tune
    connect(m_panadapterA, &PanadapterWidget::frequencyClicked, this, [this](qint64 freq) {
        // Guard: only send if connected and frequency is valid
        if (!m_tcpClient->isConnected() || freq <= 0)
            return;
        QString cmd = QString("FA%1;").arg(freq, 11, 10, QChar('0'));
        m_tcpClient->sendCAT(cmd);
        // Request frequency back to update UI (K4 doesn't echo SET commands)
        m_tcpClient->sendCAT("FA;");
    });

    // Mouse control: scroll wheel to adjust frequency using K4's native step
    connect(m_panadapterA, &PanadapterWidget::frequencyScrolled, this, [this](int steps) {
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

    // VFO B Mini-Pan connections (mode-dependent bandwidth)
    connect(m_radioState, &RadioState::modeBChanged, this,
            [this](RadioState::Mode mode) { m_vfoB->miniPan()->setMode(RadioState::modeToString(mode)); });
    connect(m_radioState, &RadioState::filterBandwidthBChanged, this,
            [this](int bw) { m_vfoB->miniPan()->setFilterBandwidth(bw); });

    // Mouse control for VFO B: click to tune
    connect(m_panadapterB, &PanadapterWidget::frequencyClicked, this, [this](qint64 freq) {
        // Guard: only send if connected and frequency is valid
        if (!m_tcpClient->isConnected() || freq <= 0)
            return;
        QString cmd = QString("FB%1;").arg(freq, 11, 10, QChar('0'));
        m_tcpClient->sendCAT(cmd);
        // Request frequency back to update UI (K4 doesn't echo SET commands)
        m_tcpClient->sendCAT("FB;");
    });

    // Mouse control for VFO B: scroll wheel to adjust frequency using K4's native step
    connect(m_panadapterB, &PanadapterWidget::frequencyScrolled, this, [this](int steps) {
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
    dialog.exec();
}

void MainWindow::connectToRadio(const RadioEntry &radio) {
    if (m_tcpClient->isConnected()) {
        m_tcpClient->disconnectFromHost();
    }

    m_currentRadio = radio;
    m_titleLabel->setText("Elecraft K4 - " + radio.name);

    qDebug() << "Connecting to" << radio.host << ":" << radio.port;
    m_tcpClient->connectToHost(radio.host, radio.port, radio.password);
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
        // Apply current volume slider setting
        m_audioEngine->setVolume(m_sideControlPanel->volume() / 100.0f);
    } else {
        qWarning() << "Failed to start audio engine";
    }

    // Query display state to initialize popup button faces (LCD and EXT)
    m_tcpClient->sendCAT("#DPM;");  // Dual panadapter mode (LCD)
    m_tcpClient->sendCAT("#HDPM;"); // Dual panadapter mode (EXT)
    m_tcpClient->sendCAT("#DSM;");  // Display mode (LCD)
    m_tcpClient->sendCAT("#HDSM;"); // Display mode (EXT)
    m_tcpClient->sendCAT("#WFC;");  // Waterfall color
    m_tcpClient->sendCAT("#AVG;");  // Averaging
    m_tcpClient->sendCAT("#PKM;");  // Peak mode
    m_tcpClient->sendCAT("#FXT;");  // Fixed tune toggle
    m_tcpClient->sendCAT("#FXA;");  // Fixed tune mode
    m_tcpClient->sendCAT("#FRZ;");  // Freeze
    m_tcpClient->sendCAT("#VFA;");  // VFO A cursor
    m_tcpClient->sendCAT("#VFB;");  // VFO B cursor
    m_tcpClient->sendCAT("#AR;");   // Auto-ref level
    m_tcpClient->sendCAT("#NB$;");  // DDC Noise Blanker mode
    m_tcpClient->sendCAT("#NBL$;"); // DDC Noise Blanker level
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
    m_modeALabel->setText(RadioState::modeToString(mode));
}

void MainWindow::onModeBChanged(RadioState::Mode mode) {
    m_modeBLabel->setText(RadioState::modeToString(mode));
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
    QString powerStr;
    if (isQrp) {
        // QRP mode: 0.0 - 10.0 W, show one decimal
        powerStr = QString("%1 W").arg(watts, 0, 'f', 1);
    } else {
        // QRO mode: 11-110 W, show as integer
        powerStr = QString("%1 W").arg(static_cast<int>(watts));
    }
    m_powerLabel->setText(powerStr);
    m_sideControlPanel->setPowerReading(watts);
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
    // ATU indicator: visible in orange when ATU is in AUTO mode (2)
    m_atuLabel->setVisible(mode == 2);
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

    // Preamp
    m_vfoA->setPreamp(m_radioState->preampEnabled() && m_radioState->preamp() > 0);

    // Attenuator
    m_vfoA->setAtt(m_radioState->attenuatorEnabled() && m_radioState->attenuatorLevel() > 0);

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

    // Preamp
    m_vfoB->setPreamp(m_radioState->preampEnabledB() && m_radioState->preampB() > 0);

    // Attenuator
    m_vfoB->setAtt(m_radioState->attenuatorEnabledB() && m_radioState->attenuatorLevelB() > 0);

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

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    // Reposition span control buttons when panadapter A resizes
    if (watched == m_panadapterA && event->type() == QEvent::Resize) {
        QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
        int w = resizeEvent->size().width();
        int h = resizeEvent->size().height();

        // Position buttons at lower right, above the frequency label bar (20px)
        // Triangle layout: C centered above, - and + below
        m_spanDownBtn->move(w - 70, h - 45);
        m_spanUpBtn->move(w - 35, h - 45);
        m_centerBtn->move(w - 52, h - 73);
    }

    // Reposition span control buttons when panadapter B resizes
    if (watched == m_panadapterB && event->type() == QEvent::Resize) {
        QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
        int w = resizeEvent->size().width();
        int h = resizeEvent->size().height();

        // Position B buttons at lower right, above the frequency label bar (20px)
        // Triangle layout: C centered above, - and + below
        m_spanDownBtnB->move(w - 70, h - 45);
        m_spanUpBtnB->move(w - 35, h - 45);
        m_centerBtnB->move(w - 52, h - 73);
    }

    return QMainWindow::eventFilter(watched, event);
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

void MainWindow::showBandPopup() {
    // Close menu overlay if visible
    if (m_menuOverlay && m_menuOverlay->isVisible()) {
        m_menuOverlay->hide();
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setMenuActive(false);
        }
    }

    // Close display popup if visible
    if (m_displayPopup && m_displayPopup->isVisible()) {
        m_displayPopup->hidePopup();
    }

    if (m_bandPopup && m_bottomMenuBar) {
        m_bandPopup->showAboveButton(m_bottomMenuBar->bandButton());
    }
}

void MainWindow::toggleDisplayPopup() {
    // Close menu overlay if visible
    if (m_menuOverlay && m_menuOverlay->isVisible()) {
        m_menuOverlay->hide();
        if (m_bottomMenuBar) {
            m_bottomMenuBar->setMenuActive(false);
        }
    }

    // Close band popup if visible
    if (m_bandPopup && m_bandPopup->isVisible()) {
        m_bandPopup->hidePopup();
    }

    if (m_displayPopup && m_bottomMenuBar) {
        if (m_displayPopup->isVisible()) {
            m_displayPopup->hidePopup();
        } else {
            m_displayPopup->showAboveButton(m_bottomMenuBar->displayButton());
            m_bottomMenuBar->setDisplayActive(true);
        }
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
    qDebug() << "Band number updated:" << bandNum;
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
    qDebug() << "KPOD rocker changed to:" << posName;
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
