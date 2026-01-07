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
class KPA1500Client;
class AudioEngine;
class MicMeterWidget;
class KpodDevice;
class RigctldServer;

class OptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OptionsDialog(RadioState *radioState, KPA1500Client *kpa1500Client, AudioEngine *audioEngine,
                           KpodDevice *kpodDevice, RigctldServer *rigctldServer, QWidget *parent = nullptr);
    ~OptionsDialog();

private slots:
    void onKpa1500ConnectionStateChanged();
    void onMicTestToggled(bool checked);
    void onMicLevelChanged(float level);
    void onMicDeviceChanged(int index);
    void onMicGainChanged(int value);
    void updateKpodStatus();

private:
    void setupUi();
    QWidget *createAboutPage();
    QWidget *createKpodPage();
    QWidget *createKpa1500Page();
    QWidget *createAudioInputPage();
    QWidget *createAudioOutputPage();
    QWidget *createNetworkPage();
    QWidget *createRigControlPage();
    void updateKpa1500Status();
    void updateRigctldStatus();
    void populateMicDevices();

    RadioState *m_radioState;
    KPA1500Client *m_kpa1500Client;
    AudioEngine *m_audioEngine;
    KpodDevice *m_kpodDevice;
    RigctldServer *m_rigctldServer;
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

    // KPA1500 settings
    QLabel *m_kpa1500StatusLabel;
    QLineEdit *m_kpa1500HostEdit;
    QLineEdit *m_kpa1500PortEdit;
    QLineEdit *m_kpa1500PollIntervalEdit;
    QCheckBox *m_kpa1500EnableCheckbox;

    // Audio Input settings
    QComboBox *m_micDeviceCombo;
    QSlider *m_micGainSlider;
    QLabel *m_micGainValueLabel;
    QPushButton *m_micTestBtn;
    MicMeterWidget *m_micMeter;
    bool m_micTestActive = false;

    // Audio Output settings
    QComboBox *m_speakerDeviceCombo;

    // Rig Control page elements
    QCheckBox *m_rigctldEnableCheckbox;
    QLineEdit *m_rigctldPortEdit;
    QLabel *m_rigctldStatusLabel;
    QLabel *m_rigctldClientsLabel;

    void populateSpeakerDevices();
    void onSpeakerDeviceChanged(int index);
};

#endif // OPTIONSDIALOG_H
