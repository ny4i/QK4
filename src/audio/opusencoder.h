#ifndef OPUSENCODER_H
#define OPUSENCODER_H

#include <QObject>
#include <opus/opus.h>

class OpusEncoder : public QObject {
    Q_OBJECT

public:
    // Frame size: 20ms at 12kHz = 240 samples
    static constexpr int FRAME_SAMPLES = 240;
    static constexpr int FRAME_BYTES = FRAME_SAMPLES * sizeof(opus_int16); // 480 bytes

    explicit OpusEncoder(QObject *parent = nullptr);
    ~OpusEncoder();

    bool initialize(int sampleRate = 12000, int channels = 1, int bitrate = 24000);
    QByteArray encode(const QByteArray &pcmData);

    int frameSamples() const { return FRAME_SAMPLES; }
    int frameBytes() const { return FRAME_BYTES; }

private:
    ::OpusEncoder *m_encoder;
    int m_sampleRate;
    int m_channels;
};

#endif // OPUSENCODER_H
