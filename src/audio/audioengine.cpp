#include "audioengine.h"
#include <QMediaDevices>
#include <QAudioDevice>
#include <QDebug>

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent), m_audioSink(nullptr), m_audioSinkDevice(nullptr), m_audioSource(nullptr),
      m_audioSourceDevice(nullptr), m_micEnabled(false) {
    // K4 uses 12kHz mono Float32 PCM (after decoding and de-interleaving)
    m_format.setSampleRate(12000);
    m_format.setChannelCount(1);
    m_format.setSampleFormat(QAudioFormat::Float);
}

AudioEngine::~AudioEngine() {
    stop();
}

bool AudioEngine::start() {
    bool outputOk = setupAudioOutput();
    bool inputOk = setupAudioInput();

    if (outputOk) {
        qDebug() << "AudioEngine: Audio output started";
    }
    if (inputOk) {
        qDebug() << "AudioEngine: Audio input ready (mic disabled by default)";
    }

    return outputOk;
}

void AudioEngine::stop() {
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
}

bool AudioEngine::setupAudioOutput() {
    QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();
    if (outputDevice.isNull()) {
        qWarning() << "AudioEngine: No audio output device available";
        return false;
    }

    if (!outputDevice.isFormatSupported(m_format)) {
        qWarning() << "AudioEngine: Audio format not supported by output device";
        return false;
    }

    qDebug() << "AudioEngine: Using output device:" << outputDevice.description();

    m_audioSink = new QAudioSink(outputDevice, m_format, this);
    m_audioSink->setBufferSize(12000 * 4 * 1); // 1 second buffer (12000 samples * 4 bytes float * 1 channel)

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
    QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    if (inputDevice.isNull()) {
        qWarning() << "AudioEngine: No audio input device available";
        return false;
    }

    if (!inputDevice.isFormatSupported(m_format)) {
        qWarning() << "AudioEngine: Audio format not supported by input device";
        return false;
    }

    qDebug() << "AudioEngine: Using input device:" << inputDevice.description();

    m_audioSource = new QAudioSource(inputDevice, m_format, this);
    m_audioSource->setBufferSize(48000 * 2 * 1);

    // Don't start mic by default - user must enable
    return true;
}

void AudioEngine::playAudio(const QByteArray &pcmData) {
    if (!m_audioSinkDevice || pcmData.isEmpty()) {
        return;
    }

    qint64 written = m_audioSinkDevice->write(pcmData);
    if (written != pcmData.size()) {
        qDebug() << "AudioEngine: Wrote" << written << "of" << pcmData.size() << "bytes";
    }
}

void AudioEngine::setMicEnabled(bool enabled) {
    if (m_micEnabled == enabled)
        return;

    m_micEnabled = enabled;

    if (!m_audioSource)
        return;

    if (enabled) {
        m_audioSourceDevice = m_audioSource->start();
        if (m_audioSourceDevice) {
            connect(m_audioSourceDevice, &QIODevice::readyRead, this, &AudioEngine::onMicDataReady);
            qDebug() << "AudioEngine: Microphone enabled";
        }
    } else {
        if (m_audioSourceDevice) {
            disconnect(m_audioSourceDevice, &QIODevice::readyRead, this, &AudioEngine::onMicDataReady);
        }
        m_audioSource->stop();
        m_audioSourceDevice = nullptr;
        qDebug() << "AudioEngine: Microphone disabled";
    }
}

void AudioEngine::onMicDataReady() {
    if (!m_audioSourceDevice || !m_micEnabled)
        return;

    QByteArray data = m_audioSourceDevice->readAll();
    if (!data.isEmpty()) {
        emit microphoneData(data);
    }
}

void AudioEngine::setVolume(float volume) {
    m_volume = qBound(0.0f, volume, 1.0f);
    if (m_audioSink) {
        m_audioSink->setVolume(m_volume);
    }
}
