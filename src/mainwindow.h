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

class PanadapterWidget;
class AudioEngine;
class OpusDecoder;
class SideControlPanel;
class RightSidePanel;
class BottomMenuBar;
class MenuModel;
class MenuOverlayWidget;
class BandPopupWidget;
class KpodDevice;
class KPA1500Client;

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
    void showBandPopup();
    void onBandSelected(const QString &bandName);
    void updateBandSelection(int bandNum);

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

    // Mode labels (in center section, not in VFOWidget)
    QLabel *m_modeALabel;
    QLabel *m_modeBLabel;

    // RX Antenna labels (in antenna row below VFOs)
    QLabel *m_rxAntALabel;
    QLabel *m_rxAntBLabel;

    // Center section
    QLabel *m_vfoASquare;
    QLabel *m_txTriangle;  // Left triangle (pointing at A) - shown when split OFF
    QLabel *m_txTriangleB; // Right triangle (pointing at B) - shown when split ON
    QLabel *m_txIndicator;
    QLabel *m_vfoBSquare;
    QLabel *m_splitLabel;
    QLabel *m_msgBankLabel;
    QWidget *m_ritXitBox;
    QLabel *m_ritLabel;
    QLabel *m_xitLabel;
    QLabel *m_ritXitValueLabel;
    QLabel *m_voxLabel;
    QLabel *m_txAntennaLabel;

    // Spectrum/Waterfall displays
    PanadapterWidget *m_panadapterA; // VFO A (Main RX)
    PanadapterWidget *m_panadapterB; // VFO B (Sub RX) - for future use
    QWidget *m_spectrumContainer;

    // Span control buttons (overlay on panadapter A)
    QPushButton *m_spanUpBtn;
    QPushButton *m_spanDownBtn;
    QPushButton *m_centerBtn;

    // Span control buttons (overlay on panadapter B)
    QPushButton *m_spanUpBtnB;
    QPushButton *m_spanDownBtnB;
    QPushButton *m_centerBtnB;

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

    RadioEntry m_currentRadio;
    int m_currentBandNum = -1; // Current band number for VFO A (BN command)

    // KPOD device
    KpodDevice *m_kpodDevice;

    // KPA1500 amplifier client
    KPA1500Client *m_kpa1500Client;
};

#endif // MAINWINDOW_H
