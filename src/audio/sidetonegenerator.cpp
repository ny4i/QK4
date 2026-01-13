#include "sidetonegenerator.h"
#include <QAudioFormat>
#include <QMediaDevices>
#include <QDebug>
#include <QTimer>
#include <QtMath>

// === ToneIODevice Implementation (kept for compatibility) ===

ToneIODevice::ToneIODevice(QObject *parent) : QIODevice(parent) {}

void ToneIODevice::setFrequency(int hz) {
    m_frequency = hz;
}
void ToneIODevice::setVolume(float volume) {
    m_volume = qBound(0.0f, volume, 1.0f);
}
void ToneIODevice::start() {
    m_playing = true;
    m_phase = 0.0;
}
void ToneIODevice::stop() {
    m_playing = false;
}
qint64 ToneIODevice::readData(char *, qint64) {
    return 0;
}

// === SidetoneGenerator Implementation ===

SidetoneGenerator::SidetoneGenerator(QObject *parent) : QObject(parent) {
    initAudio();

    // Create repeat timer for continuous keying while paddle is held
    m_repeatTimer = new QTimer(this);
    m_repeatTimer->setTimerType(Qt::PreciseTimer);
    connect(m_repeatTimer, &QTimer::timeout, this, &SidetoneGenerator::onRepeatTimer);
}

SidetoneGenerator::~SidetoneGenerator() {
    if (m_audioSink) {
        m_audioSink->stop();
    }
}

void SidetoneGenerator::initAudio() {
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    qDebug() << "SidetoneGenerator: Using audio device:" << device.description();

    if (!device.isFormatSupported(format)) {
        qWarning() << "SidetoneGenerator: Default format not supported, trying nearest";
        format = device.preferredFormat();
    }

    m_audioSink = new QAudioSink(device, format, this);
    m_audioSink->setBufferSize(131072); // 128KB - handles even 5 WPM dah (720ms = ~69KB)

    // Start audio sink immediately and keep it running
    m_pushDevice = m_audioSink->start();
    if (m_pushDevice) {
        qDebug() << "SidetoneGenerator: Audio sink started, ready for playback";
    } else {
        qWarning() << "SidetoneGenerator: Failed to start audio sink:" << m_audioSink->error();
    }

    m_toneDevice = new ToneIODevice(this);
}

void SidetoneGenerator::setFrequency(int hz) {
    m_frequency = hz;
}

void SidetoneGenerator::setVolume(float volume) {
    m_volume = volume;
}

void SidetoneGenerator::setKeyerSpeed(int wpm) {
    m_keyerWpm = qBound(5, wpm, 60);
    qDebug() << "SidetoneGenerator: Keyer speed set to" << m_keyerWpm << "WPM";
}

void SidetoneGenerator::startDit() {
    m_currentElement = ElementDit;
    m_repeatTimer->stop(); // Stop any existing repeat timer
    playElement(ditDurationMs());

    // Start repeat timer: element + inter-element space
    int repeatInterval = ditDurationMs() * 2; // dit + space
    m_repeatTimer->start(repeatInterval);
}

void SidetoneGenerator::startDah() {
    m_currentElement = ElementDah;
    m_repeatTimer->stop(); // Stop any existing repeat timer
    playElement(dahDurationMs());

    // Start repeat timer: element + inter-element space
    int repeatInterval = dahDurationMs() + ditDurationMs(); // dah + space
    m_repeatTimer->start(repeatInterval);
}

void SidetoneGenerator::stopElement() {
    m_currentElement = ElementNone;
    m_repeatTimer->stop();
    // Let current element finish playing naturally - don't reset audio
}

void SidetoneGenerator::onRepeatTimer() {
    if (m_currentElement == ElementDit) {
        playElement(ditDurationMs());
        emit ditRepeated(); // Signal for mainwindow to send KZ.; again
    } else if (m_currentElement == ElementDah) {
        playElement(dahDurationMs());
        emit dahRepeated(); // Signal for mainwindow to send KZ-; again
    }
}

int SidetoneGenerator::ditDurationMs() const {
    return 1200 / m_keyerWpm;
}

int SidetoneGenerator::dahDurationMs() const {
    return ditDurationMs() * 3;
}

void SidetoneGenerator::resetAudio() {
    // Stop and restart audio sink to clear buffer
    if (m_audioSink) {
        m_audioSink->stop();
        m_pushDevice = m_audioSink->start();
    }
    m_phase = 0.0; // Reset phase for clean start
}

void SidetoneGenerator::playElement(int durationMs) {
    if (!m_pushDevice) {
        // Try to restart audio sink if it stopped
        m_pushDevice = m_audioSink->start();
        if (!m_pushDevice) {
            qWarning() << "SidetoneGenerator: Cannot play - no audio device";
            return;
        }
    }

    const int sampleRate = 48000;
    int toneSamples = (sampleRate * durationMs) / 1000;
    int spaceSamples = (sampleRate * ditDurationMs()) / 1000; // Inter-element space = 1 dit
    int totalSamples = toneSamples + spaceSamples;

    // Add short rise/fall time to avoid clicks (3ms each)
    const int riseTimeSamples = (sampleRate * 3) / 1000;
    const int fallTimeSamples = riseTimeSamples;

    QByteArray buffer(totalSamples * sizeof(qint16), 0);
    qint16 *samples = reinterpret_cast<qint16 *>(buffer.data());

    double phaseIncrement = 2.0 * M_PI * m_frequency / sampleRate;

    // Generate tone samples
    for (int i = 0; i < toneSamples; ++i) {
        float envelope = 1.0f;
        if (i < riseTimeSamples) {
            envelope = 0.5f * (1.0f - qCos(M_PI * i / riseTimeSamples));
        } else if (i >= toneSamples - fallTimeSamples) {
            int fallIndex = i - (toneSamples - fallTimeSamples);
            envelope = 0.5f * (1.0f + qCos(M_PI * fallIndex / fallTimeSamples));
        }

        double sample = qSin(m_phase) * m_volume * envelope * 32767.0;
        samples[i] = static_cast<qint16>(sample);
        m_phase += phaseIncrement;
        if (m_phase >= 2.0 * M_PI) {
            m_phase -= 2.0 * M_PI;
        }
    }

    // Silence samples are already zero from QByteArray initialization

    m_pushDevice->write(buffer);
}

// Legacy methods
void SidetoneGenerator::playDit() {
    startDit();
}
void SidetoneGenerator::playDah() {
    startDah();
}
void SidetoneGenerator::start() {}
void SidetoneGenerator::stop() {
    stopElement();
}
void SidetoneGenerator::pushSamples() {}
