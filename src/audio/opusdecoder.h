#ifndef OPUSDECODER_H
#define OPUSDECODER_H

#include <QObject>
#include <opus/opus.h>

class OpusDecoder : public QObject
{
    Q_OBJECT

public:
    explicit OpusDecoder(QObject *parent = nullptr);
    ~OpusDecoder();

    // Initialize decoder (K4 uses 12000Hz stereo)
    bool initialize(int sampleRate = 12000, int channels = 2);

    // Decode K4 audio packet payload, returns main channel (RX1) PCM with gain
    // packet should be the full audio payload including header
    QByteArray decodeK4Packet(const QByteArray &packet);

    // Raw decode for testing
    QByteArray decode(const QByteArray &opusData);

private:
    ::OpusDecoder *m_decoder;
    int m_sampleRate;
    int m_channels;

    // K4-specific gain boost (audio is very quiet)
    static constexpr float K4_GAIN_BOOST = 32.0f;
};

#endif // OPUSDECODER_H
