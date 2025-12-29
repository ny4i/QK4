#ifndef RADIOSTATE_H
#define RADIOSTATE_H

#include <QObject>
#include <QString>
#include <QMap>

class RadioState : public QObject {
    Q_OBJECT

public:
    enum Mode { LSB = 1, USB = 2, CW = 3, FM = 4, AM = 5, DATA = 6, CW_R = 7, DATA_R = 9 };
    Q_ENUM(Mode)

    enum AGCSpeed { AGC_Off = 0, AGC_Slow = 1, AGC_Fast = 2 };
    Q_ENUM(AGCSpeed)

    explicit RadioState(QObject *parent = nullptr);

    // Parse a CAT command response and update state
    void parseCATCommand(const QString &command);

    // Frequency and VFO
    quint64 frequency() const { return m_frequency; }
    quint64 vfoA() const { return m_vfoA; }
    quint64 vfoB() const { return m_vfoB; }
    int tuningStep() const { return m_tuningStep; }

    // Mode and filter
    Mode mode() const { return m_mode; }
    Mode modeB() const { return m_modeB; }
    QString modeString() const;
    int filterBandwidth() const { return m_filterBandwidth; }
    int filterBandwidthB() const { return m_filterBandwidthB; }
    int filterPosition() const { return m_filterPosition; }
    int ifShift() const { return m_ifShift; }
    int shiftHz() const { return m_ifShift * 10; } // Convert raw IS value to Hz (IS0050 = 500 Hz)
    int cwPitch() const { return m_cwPitch; }

    // Power and levels
    double rfPower() const { return m_rfPower; }
    bool isQrpMode() const { return m_isQrpMode; }
    QString rfPowerString() const;
    int micGain() const { return m_micGain; }
    int rfGain() const { return m_rfGain; }
    int squelchLevel() const { return m_squelchLevel; }
    int rfGainB() const { return m_rfGainB; }
    int squelchLevelB() const { return m_squelchLevelB; }

    // Meters
    double sMeter() const { return m_sMeter; }
    double sMeterB() const { return m_sMeterB; }
    QString sMeterString() const;
    QString sMeterStringB() const;
    int powerMeter() const { return m_powerMeter; }
    double swrMeter() const { return m_swrMeter; }

    // TX Meter data (TM command)
    int alcMeter() const { return m_alcMeter; }
    int compressionDb() const { return m_compressionDb; }
    double forwardPower() const { return m_forwardPower; }

    // Power supply info (SIFP command)
    double supplyVoltage() const { return m_supplyVoltage; }
    double supplyCurrent() const { return m_supplyCurrent; }

    // Control states
    bool isTransmitting() const { return m_isTransmitting; }
    bool subReceiverEnabled() const { return m_subReceiverEnabled; }
    bool splitEnabled() const { return m_splitEnabled; }

    // Processing - Main RX
    int noiseBlankerLevel() const { return m_noiseBlankerLevel; }
    bool noiseBlankerEnabled() const { return m_noiseBlankerEnabled; }
    int noiseReductionLevel() const { return m_noiseReductionLevel; }
    bool noiseReductionEnabled() const { return m_noiseReductionEnabled; }
    bool autoNotchFilter() const { return m_autoNotchFilter; }

    // Notch filter
    bool autoNotchEnabled() const { return m_autoNotchEnabled; }
    bool manualNotchEnabled() const { return m_manualNotchEnabled; }
    int manualNotchPitch() const { return m_manualNotchPitch; }
    int preamp() const { return m_preamp; }
    bool preampEnabled() const { return m_preampEnabled; }
    int attenuatorLevel() const { return m_attenuatorLevel; }
    bool attenuatorEnabled() const { return m_attenuatorEnabled; }
    AGCSpeed agcSpeed() const { return m_agcSpeed; }

    // Processing - Sub RX
    int noiseBlankerLevelB() const { return m_noiseBlankerLevelB; }
    bool noiseBlankerEnabledB() const { return m_noiseBlankerEnabledB; }
    int noiseReductionLevelB() const { return m_noiseReductionLevelB; }
    bool noiseReductionEnabledB() const { return m_noiseReductionEnabledB; }
    int preampB() const { return m_preampB; }
    bool preampEnabledB() const { return m_preampEnabledB; }
    int attenuatorLevelB() const { return m_attenuatorLevelB; }
    bool attenuatorEnabledB() const { return m_attenuatorEnabledB; }
    AGCSpeed agcSpeedB() const { return m_agcSpeedB; }

    // Radio info
    QString radioID() const { return m_radioID; }
    QString radioModel() const { return m_radioModel; }
    QString optionModules() const { return m_optionModules; }
    QMap<QString, QString> firmwareVersions() const { return m_firmwareVersions; }

    // Antenna
    int txAntenna() const { return m_selectedAntenna; }
    int rxAntennaMain() const { return m_receiveAntenna; }
    int rxAntennaSub() const { return m_receiveAntennaSub; }
    QString antennaName(int index) const { return m_antennaNames.value(index, QString("ANT%1").arg(index)); }
    QString txAntennaName() const { return antennaName(m_selectedAntenna); }
    QString rxAntennaMainName() const { return antennaName(m_receiveAntenna); }
    QString rxAntennaSubName() const { return antennaName(m_receiveAntennaSub); }

    // RIT/XIT
    bool ritEnabled() const { return m_ritEnabled; }
    bool xitEnabled() const { return m_xitEnabled; }
    int ritXitOffset() const { return m_ritXitOffset; }

    // Message bank
    int messageBank() const { return m_messageBank; }

    // VOX
    bool voxCW() const { return m_voxCW; }
    bool voxVoice() const { return m_voxVoice; }
    bool voxData() const { return m_voxData; }
    bool voxEnabled() const { return m_voxCW || m_voxVoice || m_voxData; }
    // Returns VOX state for current operating mode
    bool voxForCurrentMode() const {
        switch (m_mode) {
        case CW:
        case CW_R:
            return m_voxCW;
        case DATA:
        case DATA_R:
            return m_voxData;
        default: // LSB, USB, AM, FM = Voice modes
            return m_voxVoice;
        }
    }

    // QSK/VOX Delay (in 10ms increments)
    int qskDelayCW() const { return m_qskDelayCW; }
    int qskDelayVoice() const { return m_qskDelayVoice; }
    int qskDelayData() const { return m_qskDelayData; }
    // Returns delay for current operating mode (in 10ms increments)
    int delayForCurrentMode() const {
        switch (m_mode) {
        case CW:
        case CW_R:
            return m_qskDelayCW;
        case DATA:
        case DATA_R:
            return m_qskDelayData;
        default: // LSB, USB, AM, FM = Voice modes
            return m_qskDelayVoice;
        }
    }

    // Panadapter REF level
    int refLevel() const { return m_refLevel; }

    // Panadapter span (from #SPN command, in Hz)
    int spanHz() const { return m_spanHz; }

    // Setter for optimistic span updates (UI updates immediately, like K4Mobile)
    void setSpanHz(int spanHz) {
        if (spanHz > 0 && spanHz != m_spanHz) {
            m_spanHz = spanHz;
            emit spanChanged(m_spanHz);
        }
    }

    // Static helpers
    static Mode modeFromCode(int code);
    static QString modeToString(Mode mode);

signals:
    void frequencyChanged(quint64 freq);
    void frequencyBChanged(quint64 freq);
    void modeChanged(Mode mode);
    void modeBChanged(Mode mode);
    void filterBandwidthChanged(int bw);
    void filterBandwidthBChanged(int bw);
    void ifShiftChanged(int shiftHz);
    void cwPitchChanged(int pitchHz);
    void sMeterChanged(double value);
    void sMeterBChanged(double value);
    void powerMeterChanged(int watts);
    void transmitStateChanged(bool transmitting);
    void rfPowerChanged(double watts, bool isQrp);
    void supplyVoltageChanged(double volts);
    void supplyCurrentChanged(double amps);
    void swrChanged(double swr);
    void txMeterChanged(int alc, int compression, double fwdPower, double swr);
    void splitChanged(bool enabled);
    void antennaChanged(int txAnt, int rxAntMain, int rxAntSub);
    void antennaNameChanged(int index, const QString &name);
    void ritXitChanged(bool ritEnabled, bool xitEnabled, int offset);
    void messageBankChanged(int bank);
    void processingChanged();        // NB, NR, PA, RA, GT changes for Main RX
    void processingChangedB();       // NB, NR, PA, RA, GT changes for Sub RX
    void refLevelChanged(int level); // Panadapter reference level (#REF command)
    void spanChanged(int spanHz);    // Panadapter span (#SPN command)
    void keyerSpeedChanged(int wpm); // CW keyer speed
    void qskDelayChanged(int delay); // QSK/VOX delay in 10ms increments
    void rfGainChanged(int gain);    // RF gain
    void squelchChanged(int level);  // Squelch level
    void rfGainBChanged(int gain);   // RF gain Sub RX
    void squelchBChanged(int level); // Squelch Sub RX
    void voxChanged(bool enabled);   // VOX state (any mode)
    void notchChanged();             // Manual notch state/pitch changed
    void stateUpdated();

private:
    // Frequency and VFO
    quint64 m_frequency = 0;
    quint64 m_vfoA = 0;
    quint64 m_vfoB = 0;
    int m_tuningStep = 3;

    // Mode and filter
    Mode m_mode = USB;
    Mode m_modeB = USB;
    int m_filterBandwidth = 2400;
    int m_filterBandwidthB = 2400;
    int m_filterPosition = 2;
    int m_ifShift = -1; // IF shift position (0-99, 50=centered) - init to -1 to ensure first emit
    int m_cwPitch = -1; // Init to -1 to ensure first emit

    // Power and levels
    double m_rfPower = 50.0;
    bool m_isQrpMode = false;
    int m_micGain = 50;
    int m_rfGain = -999;      // Init to invalid to ensure first emit
    int m_squelchLevel = -1;  // Init to invalid to ensure first emit
    int m_rfGainB = -999;     // Sub RX RF gain
    int m_squelchLevelB = -1; // Sub RX squelch
    int m_keyerSpeed = -1;    // WPM - init to -1 to ensure first emit

    // Meters
    double m_sMeter = 0.0;
    double m_sMeterB = 0.0;
    int m_powerMeter = 0;
    double m_swrMeter = 1.0;
    int m_alcMeter = 0;
    int m_compressionDb = 0;
    double m_forwardPower = 0.0;

    // Power supply (from SIFP)
    double m_supplyVoltage = 0.0;
    double m_supplyCurrent = 0.0;

    // Control states
    bool m_isTransmitting = false;
    bool m_subReceiverEnabled = false;
    bool m_splitEnabled = false;

    // Processing - Main RX
    int m_noiseBlankerLevel = 0;
    bool m_noiseBlankerEnabled = false;
    int m_noiseBlankerFilterWidth = 0;
    int m_noiseReductionLevel = 0;
    bool m_noiseReductionEnabled = false;
    bool m_autoNotchFilter = false;

    // Notch filter
    bool m_autoNotchEnabled = false;
    bool m_manualNotchEnabled = false;
    int m_manualNotchPitch = 1000; // 150-5000 Hz, default 1000

    int m_preamp = 0;
    bool m_preampEnabled = false;
    int m_attenuatorLevel = 0;
    bool m_attenuatorEnabled = false;
    AGCSpeed m_agcSpeed = AGC_Slow;

    // Processing - Sub RX
    int m_noiseBlankerLevelB = 0;
    bool m_noiseBlankerEnabledB = false;
    int m_noiseReductionLevelB = 0;
    bool m_noiseReductionEnabledB = false;
    int m_preampB = 0;
    bool m_preampEnabledB = false;
    int m_attenuatorLevelB = 0;
    bool m_attenuatorEnabledB = false;
    AGCSpeed m_agcSpeedB = AGC_Slow;

    // Antenna
    int m_selectedAntenna = 1;
    int m_receiveAntenna = 1;
    int m_receiveAntennaSub = 1;
    int m_atuMode = 1;
    QMap<int, QString> m_antennaNames;

    // RIT/XIT
    bool m_ritEnabled = false;
    bool m_xitEnabled = false;
    int m_ritXitOffset = 0;

    // Message bank
    int m_messageBank = 1;

    // VOX
    bool m_voxCW = false;
    bool m_voxVoice = false;
    bool m_voxData = false;

    // QSK/VOX Delay per mode (in 10ms increments)
    int m_qskDelayCW = -1;
    int m_qskDelayVoice = -1;
    int m_qskDelayData = -1;

    // Panadapter REF level
    int m_refLevel = -110; // Default -110 dBm

    // Panadapter span (from #SPN command)
    int m_spanHz = 10000; // Default 10 kHz

    // Radio info
    QString m_radioID;
    QString m_radioModel;
    QString m_optionModules;
    QMap<QString, QString> m_firmwareVersions;
};

#endif // RADIOSTATE_H
