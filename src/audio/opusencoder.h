#ifndef OPUSENCODER_H
#define OPUSENCODER_H

#include <QObject>
#include <opus/opus.h>

class OpusEncoder : public QObject
{
    Q_OBJECT

public:
    explicit OpusEncoder(QObject *parent = nullptr);
    ~OpusEncoder();

    bool initialize(int sampleRate = 48000, int channels = 1, int bitrate = 24000);
    QByteArray encode(const QByteArray &pcmData);

private:
    ::OpusEncoder *m_encoder;
    int m_sampleRate;
    int m_channels;
};

#endif // OPUSENCODER_H
