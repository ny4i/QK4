#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QAudioSink>
#include <QAudioSource>
#include <QAudioFormat>
#include <QIODevice>
#include <QBuffer>

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

signals:
    void microphoneData(const QByteArray &pcmData);

private slots:
    void onMicDataReady();

private:
    bool setupAudioOutput();
    bool setupAudioInput();

    QAudioFormat m_format;

    // Audio output (speaker)
    QAudioSink *m_audioSink;
    QIODevice *m_audioSinkDevice;

    // Audio input (microphone)
    QAudioSource *m_audioSource;
    QIODevice *m_audioSourceDevice;
    bool m_micEnabled;

    // Volume control
    float m_volume = 1.0f;
};

#endif // AUDIOENGINE_H
