#include "audioengine.h"
#include <QMediaDevices>
#include <QAudioDevice>
#include <QDebug>
#include <cmath>

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent), m_audioSink(nullptr), m_audioSinkDevice(nullptr), m_audioSource(nullptr),
      m_audioSourceDevice(nullptr), m_micEnabled(false), m_micPollTimer(nullptr) {

    // Output format: K4 uses 12kHz mono Float32 PCM (for RX audio playback)
    m_outputFormat.setSampleRate(12000);
    m_outputFormat.setChannelCount(1);
    m_outputFormat.setSampleFormat(QAudioFormat::Float);

    // Input format: Use native 48kHz for microphone capture (most hardware supports this)
    // We'll resample to 12kHz before encoding for K4 TX
    m_inputFormat.setSampleRate(48000);
    m_inputFormat.setChannelCount(1);
    m_inputFormat.setSampleFormat(QAudioFormat::Float);

    // Create timer for polling microphone data (more reliable than readyRead signal)
    m_micPollTimer = new QTimer(this);
    m_micPollTimer->setInterval(10); // Poll every 10ms for low latency
    connect(m_micPollTimer, &QTimer::timeout, this, &AudioEngine::onMicDataReady);

    // Setup audio input immediately so mic testing works without radio connection
    setupAudioInput();
}

AudioEngine::~AudioEngine() {
    stop();
}

bool AudioEngine::start() {
    bool outputOk = setupAudioOutput();

    // Setup audio input if not already done (it's also called in constructor)
    bool inputOk = (m_audioSource != nullptr) || setupAudioInput();

    Q_UNUSED(inputOk);

    return outputOk;
}

void AudioEngine::stop() {
    // Stop mic polling timer first
    if (m_micPollTimer) {
        m_micPollTimer->stop();
    }

    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
        m_audioSinkDevice = nullptr;
    }

    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
        m_audioSourceDevice = nullptr;
    }

    m_micBuffer.clear();
}

bool AudioEngine::setupAudioOutput() {
    QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();
    if (outputDevice.isNull()) {
        qWarning() << "AudioEngine: No audio output device available";
        return false;
    }

    if (!outputDevice.isFormatSupported(m_outputFormat)) {
        qWarning() << "AudioEngine: 12kHz output format not supported by device";
        return false;
    }

    m_audioSink = new QAudioSink(outputDevice, m_outputFormat, this);
    // Buffer for ~100ms latency (12000 samples/sec * 4 bytes * 0.1 sec = 4800 bytes)
    m_audioSink->setBufferSize(4800);

    m_audioSinkDevice = m_audioSink->start();
    if (!m_audioSinkDevice) {
        qWarning() << "AudioEngine: Failed to start audio output";
        delete m_audioSink;
        m_audioSink = nullptr;
        return false;
    }

    // Apply current volume setting to the newly created sink
    m_audioSink->setVolume(m_volume);

    return true;
}

bool AudioEngine::setupAudioInput() {
    // Find the input device - use selected device or fall back to default
    QAudioDevice inputDevice;

    if (!m_selectedMicDeviceId.isEmpty()) {
        // Try to find the selected device
        for (const QAudioDevice &device : QMediaDevices::audioInputs()) {
            if (device.id() == m_selectedMicDeviceId) {
                inputDevice = device;
                break;
            }
        }
    }

    // Fall back to default if selected device not found
    if (inputDevice.isNull()) {
        inputDevice = QMediaDevices::defaultAudioInput();
    }

    if (inputDevice.isNull()) {
        qWarning() << "AudioEngine: No audio input device available";
        return false;
    }

    if (!inputDevice.isFormatSupported(m_inputFormat)) {
        qWarning() << "AudioEngine: 48kHz input format not supported by device";
        return false;
    }

    m_audioSource = new QAudioSource(inputDevice, m_inputFormat, this);
    // Buffer for ~100ms of audio at 48kHz (48000 samples/sec * 4 bytes * 0.1 sec = 19200 bytes)
    m_audioSource->setBufferSize(19200);

    // Don't start mic by default - user must enable
    return true;
}

void AudioEngine::playAudio(const QByteArray &pcmData) {
    if (!m_audioSinkDevice || pcmData.isEmpty()) {
        return;
    }

    m_audioSinkDevice->write(pcmData);
}

void AudioEngine::setMicEnabled(bool enabled) {
    if (m_micEnabled == enabled)
        return;

    m_micEnabled = enabled;

    if (!m_audioSource) {
        qWarning() << "AudioEngine: m_audioSource is NULL - mic not available";
        return;
    }

    if (enabled) {
        m_audioSourceDevice = m_audioSource->start();
        if (m_audioSourceDevice) {
            // Use timer-based polling instead of readyRead signal
            // (readyRead doesn't fire reliably on all platforms)
            m_micPollTimer->start();
        } else {
            qWarning() << "AudioEngine: Failed to start microphone device";
        }
    } else {
        m_micPollTimer->stop();
        m_audioSource->stop();
        m_audioSourceDevice = nullptr;
        m_micBuffer.clear(); // Clear any buffered data
    }
}

QByteArray AudioEngine::resample48kTo12k(const QByteArray &input48k) {
    // Simple 4:1 decimation with averaging filter
    // 48kHz / 4 = 12kHz
    const float *inputSamples = reinterpret_cast<const float *>(input48k.constData());
    int inputCount = input48k.size() / sizeof(float);
    int outputCount = inputCount / 4;

    QByteArray output12k;
    output12k.reserve(outputCount * sizeof(float));

    for (int i = 0; i < outputCount; i++) {
        // Average 4 samples for simple low-pass filtering
        int srcIdx = i * 4;
        float sum = 0.0f;
        int count = 0;
        for (int j = 0; j < 4 && (srcIdx + j) < inputCount; j++) {
            sum += inputSamples[srcIdx + j];
            count++;
        }
        float avg = (count > 0) ? (sum / count) : 0.0f;
        output12k.append(reinterpret_cast<const char *>(&avg), sizeof(float));
    }

    return output12k;
}

void AudioEngine::onMicDataReady() {
    if (!m_audioSourceDevice || !m_micEnabled)
        return;

    QByteArray data48k = m_audioSourceDevice->readAll();
    if (data48k.isEmpty()) {
        // No data available yet - this is normal, just wait for next poll
        return;
    }

    // Resample from 48kHz to 12kHz
    QByteArray data12k = resample48kTo12k(data48k);

    // Emit raw resampled data for any listeners that want it
    emit microphoneData(data12k);

    // Convert Float32 to S16LE, apply gain, and buffer for frame-based emission
    const float *floatData = reinterpret_cast<const float *>(data12k.constData());
    int floatSamples = data12k.size() / sizeof(float);

    // Calculate RMS level AFTER gain for meter display (shows what will be transmitted)
    float sumSquares = 0.0f;

    // Convert and append to buffer with gain applied
    for (int i = 0; i < floatSamples; i++) {
        // Apply mic gain and clamp
        float sample = qBound(-1.0f, floatData[i] * m_micGain * 2.0f, 1.0f); // *2 so 50% = unity
        qint16 s16Sample = static_cast<qint16>(sample * 32767.0f);

        // Accumulate for RMS calculation (after gain)
        sumSquares += sample * sample;

        // Append as little-endian bytes
        m_micBuffer.append(reinterpret_cast<const char *>(&s16Sample), sizeof(qint16));
    }

    // Emit RMS level for meter display
    float rmsLevel = (floatSamples > 0) ? std::sqrt(sumSquares / floatSamples) : 0.0f;
    emit micLevelChanged(rmsLevel);

    // Emit complete frames (240 samples = 480 bytes S16LE each)
    while (m_micBuffer.size() >= FRAME_BYTES_S16LE) {
        QByteArray frame = m_micBuffer.left(FRAME_BYTES_S16LE);
        m_micBuffer.remove(0, FRAME_BYTES_S16LE);
        emit microphoneFrame(frame);
    }
}

void AudioEngine::setVolume(float volume) {
    m_volume = qBound(0.0f, volume, 1.0f);
    if (m_audioSink) {
        m_audioSink->setVolume(m_volume);
    }
}

void AudioEngine::setMicGain(float gain) {
    m_micGain = qBound(0.0f, gain, 1.0f);
}

void AudioEngine::setMicDevice(const QString &deviceId) {
    if (m_selectedMicDeviceId != deviceId) {
        m_selectedMicDeviceId = deviceId;

        // If mic is currently enabled, we need to restart it with the new device
        bool wasEnabled = m_micEnabled;
        if (wasEnabled) {
            setMicEnabled(false);
        }

        // Recreate the audio source with the new device
        if (m_audioSource) {
            delete m_audioSource;
            m_audioSource = nullptr;
        }
        setupAudioInput();

        if (wasEnabled) {
            setMicEnabled(true);
        }
    }
}

QString AudioEngine::micDeviceId() const {
    return m_selectedMicDeviceId;
}

QList<QPair<QString, QString>> AudioEngine::availableInputDevices() {
    QList<QPair<QString, QString>> devices;

    // Add "System Default" as the first option
    devices.append(qMakePair(QString(""), QString("System Default")));

    // Add all available input devices
    for (const QAudioDevice &device : QMediaDevices::audioInputs()) {
        devices.append(qMakePair(QString(device.id()), device.description()));
    }

    return devices;
}
