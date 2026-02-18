#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>

class RadioState;
class AudioEngine;
class MicMeterWidget;
class KpodDevice;
class CatServer;
class HalikeyDevice;

class OptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OptionsDialog(RadioState *radioState, AudioEngine *audioEngine, KpodDevice *kpodDevice,
                           CatServer *catServer, HalikeyDevice *halikeyDevice, QWidget *parent = nullptr);
    ~OptionsDialog();

private slots:
    void onMicTestToggled(bool checked);
    void onMicLevelChanged(float level);
    void onMicDeviceChanged(int index);
    void onMicGainChanged(int value);
    void updateKpodStatus();
    void updateCwKeyerStatus();
    void onCwKeyerConnectClicked();
    void onCwKeyerRefreshClicked();

private:
    void setupUi();
    QWidget *createAboutPage();
    QWidget *createKpodPage();
    QWidget *createAudioInputPage();
    QWidget *createAudioOutputPage();
    QWidget *createRigControlPage();
    QWidget *createCwKeyerPage();
    void updateCatServerStatus();
    void populateMicDevices();

    RadioState *m_radioState;
    AudioEngine *m_audioEngine;
    KpodDevice *m_kpodDevice;
    CatServer *m_catServer;
    HalikeyDevice *m_halikeyDevice;
    QListWidget *m_tabList;
    QStackedWidget *m_pageStack;

    // KPOD page elements (for real-time updates)
    QCheckBox *m_kpodEnableCheckbox;
    QLabel *m_kpodStatusLabel;
    QLabel *m_kpodProductLabel;
    QLabel *m_kpodManufacturerLabel;
    QLabel *m_kpodVendorIdLabel;
    QLabel *m_kpodProductIdLabel;
    QLabel *m_kpodDeviceTypeLabel;
    QLabel *m_kpodFirmwareLabel;
    QLabel *m_kpodDeviceIdLabel;
    QLabel *m_kpodHelpLabel;

    // Audio Input settings
    QComboBox *m_micDeviceCombo;
    QSlider *m_micGainSlider;
    QLabel *m_micGainValueLabel;
    QPushButton *m_micTestBtn;
    MicMeterWidget *m_micMeter;
    bool m_micTestActive = false;

    // Audio Output settings
    QComboBox *m_speakerDeviceCombo;

    // CAT Server page elements
    QCheckBox *m_catServerEnableCheckbox;
    QLineEdit *m_catServerPortEdit;
    QLabel *m_catServerStatusLabel;
    QLabel *m_catServerClientsLabel;

    void populateSpeakerDevices();
    void onSpeakerDeviceChanged(int index);

    // CW Keyer page elements
    QComboBox *m_cwKeyerDeviceTypeCombo = nullptr;
    QLabel *m_cwKeyerDescLabel = nullptr;
    QComboBox *m_cwKeyerPortCombo;
    QPushButton *m_cwKeyerRefreshBtn;
    QPushButton *m_cwKeyerConnectBtn;
    QLabel *m_cwKeyerStatusLabel;
    QSlider *m_sidetoneVolumeSlider = nullptr;
    QLabel *m_sidetoneVolumeValueLabel = nullptr;
    void populateCwKeyerPorts();
    void updateCwKeyerDescription();
};

#endif // OPTIONSDIALOG_H
