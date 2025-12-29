#include "opusencoder.h"

OpusEncoder::OpusEncoder(QObject *parent) : QObject(parent), m_encoder(nullptr), m_sampleRate(48000), m_channels(1) {}

OpusEncoder::~OpusEncoder() {
    if (m_encoder) {
        opus_encoder_destroy(m_encoder);
    }
}

bool OpusEncoder::initialize(int sampleRate, int channels, int bitrate) {
    m_sampleRate = sampleRate;
    m_channels = channels;

    int error;
    m_encoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_VOIP, &error);
    if (error != OPUS_OK)
        return false;

    opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(bitrate));
    return true;
}

QByteArray OpusEncoder::encode(const QByteArray &pcmData) {
    if (!m_encoder)
        return QByteArray();

    const int frameSize = 960;
    const int maxPacketSize = 4000;
    QByteArray encoded(maxPacketSize, 0);

    const opus_int16 *pcm = reinterpret_cast<const opus_int16 *>(pcmData.constData());
    int bytes =
        opus_encode(m_encoder, pcm, frameSize, reinterpret_cast<unsigned char *>(encoded.data()), maxPacketSize);

    if (bytes < 0)
        return QByteArray();

    encoded.resize(bytes);
    return encoded;
}
