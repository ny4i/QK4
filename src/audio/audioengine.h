#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QAudioSink>
#include <QAudioSource>
#include <QAudioFormat>
#include <QIODevice>
#include <QTimer>

class AudioEngine : public QObject {
    Q_OBJECT

public:
    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine();

    bool start();
    void stop();
    void playAudio(const QByteArray &pcmData);

    void setMicEnabled(bool enabled);
    bool isMicEnabled() const { return m_micEnabled; }

    void setVolume(float volume); // 0.0 to 1.0
    float volume() const { return m_volume; }

    // Microphone settings
    void setMicGain(float gain); // 0.0 to 1.0
    float micGain() const { return m_micGain; }

    void setMicDevice(const QString &deviceId);
    QString micDeviceId() const;

    // Get list of available input devices (for settings UI)
    static QList<QPair<QString, QString>> availableInputDevices(); // (id, description)

    // Output device settings
    void setOutputDevice(const QString &deviceId);
    QString outputDeviceId() const;

    // Get list of available output devices (for settings UI)
    static QList<QPair<QString, QString>> availableOutputDevices(); // (id, description)

signals:
    void microphoneData(const QByteArray &pcmData);    // Raw Float32 mic data (variable size)
    void microphoneFrame(const QByteArray &s16leData); // Complete frame (240 samples, S16LE @ 12kHz)
    void micLevelChanged(float level);                 // RMS level 0.0-1.0 for meter display

private slots:
    void onMicDataReady();

private:
    bool setupAudioOutput();
    bool setupAudioInput();

    // Resample 48kHz Float32 samples to 12kHz (4:1 decimation with averaging)
    QByteArray resample48kTo12k(const QByteArray &input48k);

    // Audio output format: 12kHz mono Float32 (K4 RX audio)
    QAudioFormat m_outputFormat;

    // Audio input format: 48kHz mono Float32 (native macOS rate, resampled to 12kHz)
    QAudioFormat m_inputFormat;

    // Audio output (speaker)
    QAudioSink *m_audioSink;
    QIODevice *m_audioSinkDevice;

    // Audio input (microphone)
    QAudioSource *m_audioSource;
    QIODevice *m_audioSourceDevice;
    bool m_micEnabled;
    QString m_selectedMicDeviceId;    // Empty = use system default
    QString m_selectedOutputDeviceId; // Empty = use system default

    // Volume control
    float m_volume = 1.0f;

    // Microphone gain control
    float m_micGain = 0.25f; // Default 25% (macOS mic input is typically hot)

    // Audio buffer sizes for ~100ms latency
    // Output: 12kHz * 4 bytes/sample * 0.1 sec = 4800 bytes
    // Input: 48kHz * 4 bytes/sample * 0.1 sec = 19200 bytes
    static constexpr int OUTPUT_BUFFER_SIZE = 4800;
    static constexpr int INPUT_BUFFER_SIZE = 19200;

    // Microphone gain scaling factor (gain slider 0-1 maps to 0-2x, so 0.5 = unity)
    static constexpr float MIC_GAIN_SCALE = 2.0f;

    // Microphone frame buffering for Opus encoding
    // Buffer accumulates S16LE samples at 12kHz until we have a complete frame
    static constexpr int FRAME_SAMPLES = 240;                                // 20ms at 12kHz
    static constexpr int FRAME_BYTES_S16LE = FRAME_SAMPLES * sizeof(qint16); // 480 bytes
    QByteArray m_micBuffer;

    // Timer for polling microphone data (more reliable than readyRead signal)
    QTimer *m_micPollTimer;
};

#endif // AUDIOENGINE_H
