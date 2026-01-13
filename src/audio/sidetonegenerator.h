#ifndef SIDETONEGENERATOR_H
#define SIDETONEGENERATOR_H

#include <QObject>
#include <QAudioSink>
#include <QIODevice>
#include <QBuffer>
#include <QByteArray>
#include <QtMath>

class ToneIODevice : public QIODevice {
    Q_OBJECT
public:
    explicit ToneIODevice(QObject *parent = nullptr);

    void setFrequency(int hz);
    void setVolume(float volume);
    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *, qint64) override { return 0; }

private:
    int m_frequency = 600;
    float m_volume = 0.3f;
    double m_phase = 0.0;
    bool m_playing = false;
    static constexpr int SAMPLE_RATE = 48000;
};

class QTimer;

class SidetoneGenerator : public QObject {
    Q_OBJECT
public:
    explicit SidetoneGenerator(QObject *parent = nullptr);
    ~SidetoneGenerator();

    void setFrequency(int hz);
    void setVolume(float volume);
    void setKeyerSpeed(int wpm);

    // Start repeating element while paddle is held
    void startDit();
    void startDah();
    void stopElement(); // Call when paddle is released

    // Legacy single-shot methods
    void playDit();
    void playDah();

signals:
    // Emitted when repeat timer fires (for sending KZ commands)
    void ditRepeated();
    void dahRepeated();

public slots:
    void start();
    void stop();

private slots:
    void onRepeatTimer();
    void pushSamples();

private:
    void initAudio();
    void resetAudio();
    void playElement(int durationMs);
    int ditDurationMs() const;
    int dahDurationMs() const;

    enum Element { ElementNone, ElementDit, ElementDah };

    QAudioSink *m_audioSink = nullptr;
    ToneIODevice *m_toneDevice = nullptr;
    QIODevice *m_pushDevice = nullptr;
    QTimer *m_repeatTimer = nullptr;
    int m_frequency = 600;
    float m_volume = 0.3f;
    int m_keyerWpm = 20;
    double m_phase = 0.0;
    Element m_currentElement = ElementNone;
};

#endif // SIDETONEGENERATOR_H
