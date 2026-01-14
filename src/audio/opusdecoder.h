#ifndef OPUSDECODER_H
#define OPUSDECODER_H

#include <QObject>
#include <opus/opus.h>

class OpusDecoder : public QObject {
    Q_OBJECT

public:
    explicit OpusDecoder(QObject *parent = nullptr);
    ~OpusDecoder();

    // Initialize decoder (K4 uses 12000Hz stereo)
    bool initialize(int sampleRate = 12000, int channels = 2);

    // Decode K4 audio packet payload, returns mixed Main+Sub PCM with gain
    // packet should be the full audio payload including header
    QByteArray decodeK4Packet(const QByteArray &packet);

    // Raw decode for testing (returns S16LE stereo PCM)
    QByteArray decode(const QByteArray &opusData);

    // Decode to float (returns float32 stereo PCM)
    QByteArray decodeFloat(const QByteArray &opusData);

    // Volume control for channel mixing (0.0 to 1.0)
    void setMainVolume(float volume);
    void setSubVolume(float volume);
    float mainVolume() const { return m_mainVolume; }
    float subVolume() const { return m_subVolume; }

private:
    ::OpusDecoder *m_decoder;
    int m_sampleRate;
    int m_channels;

    // Channel volume controls (0.0 to 1.0)
    float m_mainVolume = 1.0f;
    float m_subVolume = 1.0f;

    // K4-specific gain boost (audio is very quiet)
    static constexpr float K4_GAIN_BOOST = 32.0f;
};

#endif // OPUSDECODER_H
