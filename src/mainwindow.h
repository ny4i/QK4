#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QMenuBar>
#include <QMenu>
#include <QProgressBar>
#include <QTimer>
#include <QStackedWidget>
#include "network/tcpclient.h"
#include "settings/radiosettings.h"
#include "models/radiostate.h"
#include "ui/vfowidget.h"

class PanadapterRhiWidget;
class AudioEngine;
class OpusDecoder;
class OpusEncoder;
class SideControlPanel;
class RightSidePanel;
class BottomMenuBar;
class MenuModel;
class MenuOverlayWidget;
class BandPopupWidget;
class ButtonRowPopup;
class DisplayPopupWidget;
class FnPopupWidget;
class MacroDialog;
class FeatureMenuBar;
class ModePopupWidget;
class KpodDevice;
class TxMeterWidget;
class KPA1500Client;
class CatServer;
class NotificationWidget;
class VfoRowWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    // Panadapter display modes
    enum class PanadapterMode { MainOnly, Dual, SubOnly };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Switch between Main only, Dual (A+B), and Sub only display
    void setPanadapterMode(PanadapterMode mode);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onStateChanged(TcpClient::ConnectionState state);
    void onError(const QString &error);
    void onAuthenticated();
    void onAuthenticationFailed();
    void onCatResponse(const QString &response);
    void onFrequencyChanged(quint64 freq);
    void onFrequencyBChanged(quint64 freq);
    void onModeChanged(RadioState::Mode mode);
    void onModeBChanged(RadioState::Mode mode);
    void onSMeterChanged(double value);
    void onSMeterBChanged(double value);
    void onBandwidthChanged(int bw);
    void onBandwidthBChanged(int bw);
    void onRfPowerChanged(double watts, bool isQrp);
    void onSupplyVoltageChanged(double volts);
    void onSupplyCurrentChanged(double amps);
    void onSwrChanged(double swr);
    void onSplitChanged(bool enabled);
    void onAntennaChanged(int txAnt, int rxAntMain, int rxAntSub);
    void onAntennaNameChanged(int index, const QString &name);
    void onVoxChanged(bool enabled);
    void onQskEnabledChanged(bool enabled);
    void onTestModeChanged(bool enabled);
    void onAtuModeChanged(int mode);
    void onRitXitChanged(bool ritEnabled, bool xitEnabled, int offset);
    void onMessageBankChanged(int bank);
    void onProcessingChanged();
    void onProcessingChangedB();
    void onSpectrumData(int receiver, const QByteArray &data, qint64 centerFreq, qint32 sampleRate, float noiseFloor);
    void onMiniSpectrumData(int receiver, const QByteArray &data);
    void onAudioData(const QByteArray &opusData);
    void showRadioManager();
    void connectToRadio(const RadioEntry &radio);
    void updateDateTime();
    void showMenuOverlay();
    void onMenuValueChangeRequested(int menuId, const QString &action);
    void onBandSelected(const QString &bandName);
    void updateBandSelection(int bandNum);
    void toggleDisplayPopup();
    void toggleBandPopup();
    void toggleFnPopup();
    void toggleMainRxPopup();
    void toggleSubRxPopup();
    void toggleTxPopup();
    void closeAllPopups();

    // KPOD slots
    void onKpodEncoderRotated(int ticks);
    void onKpodRockerChanged(int position);
    void onKpodPollError(const QString &error);
    void onKpodEnabledChanged(bool enabled);

    // KPA1500 slots
    void onKpa1500Connected();
    void onKpa1500Disconnected();
    void onKpa1500Error(const QString &error);
    void onKpa1500EnabledChanged(bool enabled);
    void onKpa1500SettingsChanged();
    void updateKpa1500Status();

    // Error/notification from K4 (ERxx: messages)
    void onErrorNotification(int errorCode, const QString &message);

    // PTT slots
    void onPttPressed();
    void onPttReleased();
    void onMicrophoneFrame(const QByteArray &s16leData);

    // Fn popup / macro slots
    void onFnFunctionTriggered(const QString &functionId);
    void executeMacro(const QString &functionId);
    void openMacroDialog();

private:
    void setupMenuBar();
    void setupUi();
    void setupTopStatusBar(QWidget *parent);
    void setupVfoSection(QWidget *parent);
    void setupSpectrumPlaceholder(QWidget *parent);
    void updateConnectionState(TcpClient::ConnectionState state);
    QString formatFrequency(quint64 freq);

    TcpClient *m_tcpClient;
    RadioState *m_radioState;
    QTimer *m_clockTimer;

    // Audio
    AudioEngine *m_audioEngine;
    OpusDecoder *m_opusDecoder;
    OpusEncoder *m_opusEncoder;

    // PTT state
    bool m_pttActive = false;
    quint8 m_txSequence = 0;

    // Top status bar
    QLabel *m_titleLabel;
    QLabel *m_dateTimeLabel;
    QLabel *m_powerLabel;
    QLabel *m_swrLabel;
    QLabel *m_voltageLabel;
    QLabel *m_currentLabel;
    QLabel *m_connectionStatusLabel;
    QLabel *m_kpa1500StatusLabel;

    // VFO widgets (modular, reusable components)
    VFOWidget *m_vfoA;
    VFOWidget *m_vfoB;

    // NOTE: TX meters are now integrated into VFOWidgets as multifunction S/Po meters
    // (see VFOWidget::m_txMeter - displays S-meter when RX, Po when TX)

    // Mode labels (in center section, not in VFOWidget)
    QLabel *m_modeALabel;
    QLabel *m_modeBLabel;

    // RX Antenna labels (in antenna row below VFOs)
    QLabel *m_rxAntALabel;
    QLabel *m_rxAntBLabel;

    // Center section - first row with absolute positioning
    VfoRowWidget *m_vfoRow;

    // Center section labels (pointers to VfoRowWidget children)
    QLabel *m_vfoASquare;
    QLabel *m_txTriangle;  // Left triangle (pointing at A) - shown when split OFF
    QLabel *m_txTriangleB; // Right triangle (pointing at B) - shown when split ON
    QLabel *m_txIndicator;
    QLabel *m_vfoBSquare;
    QLabel *m_splitLabel;
    QLabel *m_bSetLabel;
    QLabel *m_subLabel; // SUB indicator (green when sub RX enabled)
    QLabel *m_divLabel; // DIV indicator (green when diversity enabled)
    QLabel *m_msgBankLabel;
    QWidget *m_ritXitBox;
    QLabel *m_ritLabel;
    QLabel *m_xitLabel;
    QLabel *m_ritXitValueLabel;
    QLabel *m_atuLabel;
    QLabel *m_filterALabel; // VFO A filter position (FIL1/FIL2/FIL3)
    QLabel *m_filterBLabel; // VFO B filter position (FIL1/FIL2/FIL3)

    // Memory buttons (M1-M4, REC, STORE, RCL)
    QPushButton *m_m1Btn;
    QPushButton *m_m2Btn;
    QPushButton *m_m3Btn;
    QPushButton *m_m4Btn;
    QPushButton *m_recBtn;
    QPushButton *m_storeBtn;
    QPushButton *m_rclBtn;
    QLabel *m_voxLabel;
    QLabel *m_qskLabel;
    QLabel *m_testLabel;
    QLabel *m_txAntennaLabel;

    // Spectrum/Waterfall displays (QRhiWidget - Metal/DirectX/Vulkan)
    PanadapterRhiWidget *m_panadapterA; // VFO A (Main RX)
    PanadapterRhiWidget *m_panadapterB; // VFO B (Sub RX) - for future use
    QWidget *m_spectrumContainer;

    // Span control buttons (overlay on panadapter A)
    QPushButton *m_spanUpBtn;
    QPushButton *m_spanDownBtn;
    QPushButton *m_centerBtn;

    // Span control buttons (overlay on panadapter B)
    QPushButton *m_spanUpBtnB;
    QPushButton *m_spanDownBtnB;
    QPushButton *m_centerBtnB;

    // VFO indicator badges (bottom-left corner of waterfall)
    QLabel *m_vfoIndicatorA;
    QLabel *m_vfoIndicatorB;

    // Panadapter display mode
    PanadapterMode m_panadapterMode = PanadapterMode::MainOnly;

    // Control panels (L-shaped layout)
    SideControlPanel *m_sideControlPanel;
    RightSidePanel *m_rightSidePanel;
    BottomMenuBar *m_bottomMenuBar;

    // Menu system
    MenuModel *m_menuModel;
    MenuOverlayWidget *m_menuOverlay;
    BandPopupWidget *m_bandPopup;
    DisplayPopupWidget *m_displayPopup;
    FnPopupWidget *m_fnPopup;
    MacroDialog *m_macroDialog;
    ButtonRowPopup *m_mainRxPopup;
    ButtonRowPopup *m_subRxPopup;
    ButtonRowPopup *m_txPopup;
    FeatureMenuBar *m_featureMenuBar;
    ModePopupWidget *m_modePopup;

    RadioEntry m_currentRadio;
    int m_currentBandNum = -1; // Current band number for VFO A (BN command)

    // KPOD device
    KpodDevice *m_kpodDevice;

    // KPA1500 amplifier client
    KPA1500Client *m_kpa1500Client;

    // CAT server for external app integration (WSJT-X, MacLoggerDX, etc.)
    CatServer *m_catServer;

    // Notification popup for K4 error/status messages (ERxx:)
    NotificationWidget *m_notificationWidget;
};

#endif // MAINWINDOW_H
