#ifndef SIDETONEGENERATOR_H
#define SIDETONEGENERATOR_H

#include <QObject>
#include <QAudioSink>
#include <QIODevice>
#include <QByteArray>
#include <QtMath>

class QTimer;

class SidetoneGenerator : public QObject {
    Q_OBJECT
public:
    explicit SidetoneGenerator(QObject *parent = nullptr);
    ~SidetoneGenerator();

    void setFrequency(int hz);
    void setVolume(float volume);
    void setKeyerSpeed(int wpm);

    // Start repeating element while paddle is held (V14 modem-line interface)
    void startDit();
    void startDah();
    void stopElement(); // Call when paddle is released

    // Play a single element without repeat (MIDI interface — K4 keyer handles repeat)
    void playSingleDit();
    void playSingleDah();

signals:
    // Emitted when repeat timer fires (for sending KZ commands)
    void ditRepeated();
    void dahRepeated();

private slots:
    void onRepeatTimer();

private:
    void initAudio();
    void playElement(int durationMs);
    int ditDurationMs() const;
    int dahDurationMs() const;

    enum Element { ElementNone, ElementDit, ElementDah };

    QAudioSink *m_audioSink = nullptr;
    QIODevice *m_pushDevice = nullptr;
    QTimer *m_repeatTimer = nullptr;
    int m_frequency = 600;
    float m_volume = 0.3f;
    int m_keyerWpm = 20;
    double m_phase = 0.0;
    Element m_currentElement = ElementNone;
};

#endif // SIDETONEGENERATOR_H
