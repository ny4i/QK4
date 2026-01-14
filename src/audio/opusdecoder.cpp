#include "opusdecoder.h"
#include <QDebug>

OpusDecoder::OpusDecoder(QObject *parent) : QObject(parent), m_decoder(nullptr), m_sampleRate(12000), m_channels(2) {}

OpusDecoder::~OpusDecoder() {
    if (m_decoder) {
        opus_decoder_destroy(m_decoder);
    }
}

bool OpusDecoder::initialize(int sampleRate, int channels) {
    m_sampleRate = sampleRate;
    m_channels = channels;

    int error;
    m_decoder = opus_decoder_create(sampleRate, channels, &error);

    if (error != OPUS_OK) {
        qWarning() << "OpusDecoder: Failed to create decoder:" << opus_strerror(error);
        return false;
    }

    return true;
}

QByteArray OpusDecoder::decodeK4Packet(const QByteArray &packet) {
    // K4 Audio Packet Structure:
    // Byte 0: TYPE = 1 (Audio)
    // Byte 1: VER = Version number
    // Byte 2: SEQ = Sequence number
    // Byte 3: Encode Mode (0=RAW32, 1=RAW16, 2=Opus Int, 3=Opus Float)
    // Bytes 4-5: Frame size (little-endian UInt16) - samples per channel
    // Byte 6: Sample rate code (0 = 12000 Hz)
    // Byte 7+: Audio data (format depends on encode mode)

    if (packet.size() < 8) {
        return QByteArray();
    }

    // Verify packet type
    if (static_cast<unsigned char>(packet[0]) != 0x01) {
        return QByteArray();
    }

    unsigned char encodeMode = static_cast<unsigned char>(packet[3]);

    // Extract frame size (little-endian UInt16 at bytes 4-5)
    quint16 frameSize = static_cast<unsigned char>(packet[4]) | (static_cast<unsigned char>(packet[5]) << 8);
    Q_UNUSED(frameSize)

    // Extract audio data starting at byte 7
    QByteArray audioData = packet.mid(7);

    if (audioData.isEmpty()) {
        return QByteArray();
    }

    // Decode based on encode mode
    int monoSampleCount = 0;
    QByteArray monoPcm;

    switch (encodeMode) {
    case 0x00: // EM0 - RAW 32-bit float stereo PCM
    {
        // Data is interleaved float32 stereo [L0, R0, L1, R1, ...]
        const float *stereoFloats = reinterpret_cast<const float *>(audioData.constData());
        int totalFloats = audioData.size() / sizeof(float);
        monoSampleCount = totalFloats / 2;

        monoPcm.resize(monoSampleCount * sizeof(float));
        float *monoFloats = reinterpret_cast<float *>(monoPcm.data());

        for (int i = 0; i < monoSampleCount; i++) {
            float mainSample = stereoFloats[i * 2];    // Left channel (Main RX / VFO A)
            float subSample = stereoFloats[i * 2 + 1]; // Right channel (Sub RX / VFO B)

            float mainWithVolume = mainSample * m_mainVolume * K4_GAIN_BOOST;
            float subWithVolume = subSample * m_subVolume * K4_GAIN_BOOST;

            monoFloats[i] = qBound(-1.0f, mainWithVolume + subWithVolume, 1.0f);
        }
        break;
    }

    case 0x01: // EM1 - RAW 16-bit S16LE stereo PCM
    {
        // Data is interleaved S16LE stereo [L0, R0, L1, R1, ...]
        const qint16 *stereoSamples = reinterpret_cast<const qint16 *>(audioData.constData());
        int totalSamples = audioData.size() / sizeof(qint16);
        monoSampleCount = totalSamples / 2;

        monoPcm.resize(monoSampleCount * sizeof(float));
        float *monoFloats = reinterpret_cast<float *>(monoPcm.data());

        for (int i = 0; i < monoSampleCount; i++) {
            qint16 mainSample = stereoSamples[i * 2];    // Left channel (Main RX / VFO A)
            qint16 subSample = stereoSamples[i * 2 + 1]; // Right channel (Sub RX / VFO B)

            float mainNormalized = static_cast<float>(mainSample) / 32768.0f;
            float subNormalized = static_cast<float>(subSample) / 32768.0f;

            float mainWithVolume = mainNormalized * m_mainVolume * K4_GAIN_BOOST;
            float subWithVolume = subNormalized * m_subVolume * K4_GAIN_BOOST;

            monoFloats[i] = qBound(-1.0f, mainWithVolume + subWithVolume, 1.0f);
        }
        break;
    }

    case 0x02: // EM2 - Opus encoded, decode with opus_decode() (returns S16LE)
    {
        QByteArray stereoPcm = decode(audioData);
        if (stereoPcm.isEmpty()) {
            return QByteArray();
        }

        const qint16 *stereoSamples = reinterpret_cast<const qint16 *>(stereoPcm.constData());
        int totalSamples = stereoPcm.size() / sizeof(qint16);
        monoSampleCount = totalSamples / 2;

        monoPcm.resize(monoSampleCount * sizeof(float));
        float *monoFloats = reinterpret_cast<float *>(monoPcm.data());

        for (int i = 0; i < monoSampleCount; i++) {
            qint16 mainSample = stereoSamples[i * 2];    // Left channel (Main RX / VFO A)
            qint16 subSample = stereoSamples[i * 2 + 1]; // Right channel (Sub RX / VFO B)

            float mainNormalized = static_cast<float>(mainSample) / 32768.0f;
            float subNormalized = static_cast<float>(subSample) / 32768.0f;

            float mainWithVolume = mainNormalized * m_mainVolume * K4_GAIN_BOOST;
            float subWithVolume = subNormalized * m_subVolume * K4_GAIN_BOOST;

            monoFloats[i] = qBound(-1.0f, mainWithVolume + subWithVolume, 1.0f);
        }
        break;
    }

    case 0x03: // EM3 - Opus encoded, decode with opus_decode_float() (returns float)
    {
        QByteArray stereoPcm = decodeFloat(audioData);
        if (stereoPcm.isEmpty()) {
            return QByteArray();
        }

        const float *stereoFloats = reinterpret_cast<const float *>(stereoPcm.constData());
        int totalFloats = stereoPcm.size() / sizeof(float);
        monoSampleCount = totalFloats / 2;

        monoPcm.resize(monoSampleCount * sizeof(float));
        float *monoFloats = reinterpret_cast<float *>(monoPcm.data());

        for (int i = 0; i < monoSampleCount; i++) {
            float mainSample = stereoFloats[i * 2];    // Left channel (Main RX / VFO A)
            float subSample = stereoFloats[i * 2 + 1]; // Right channel (Sub RX / VFO B)

            float mainWithVolume = mainSample * m_mainVolume * K4_GAIN_BOOST;
            float subWithVolume = subSample * m_subVolume * K4_GAIN_BOOST;

            monoFloats[i] = qBound(-1.0f, mainWithVolume + subWithVolume, 1.0f);
        }
        break;
    }

    default:
        qWarning() << "OpusDecoder: Unknown encode mode:" << encodeMode;
        return QByteArray();
    }

    return monoPcm;
}

void OpusDecoder::setMainVolume(float volume) {
    m_mainVolume = qBound(0.0f, volume, 1.0f);
}

void OpusDecoder::setSubVolume(float volume) {
    m_subVolume = qBound(0.0f, volume, 1.0f);
}

QByteArray OpusDecoder::decode(const QByteArray &opusData) {
    if (!m_decoder)
        return QByteArray();

    // Max frame size for 12kHz = 120ms * 12000 = 1440 samples per channel
    const int maxFrameSize = 1440;
    QVector<opus_int16> pcm(maxFrameSize * m_channels);

    int samples = opus_decode(m_decoder, reinterpret_cast<const unsigned char *>(opusData.constData()), opusData.size(),
                              pcm.data(), maxFrameSize, 0);

    if (samples < 0) {
        qWarning() << "OpusDecoder: Decode failed:" << opus_strerror(samples);
        return QByteArray();
    }

    return QByteArray(reinterpret_cast<const char *>(pcm.constData()), samples * m_channels * sizeof(opus_int16));
}

QByteArray OpusDecoder::decodeFloat(const QByteArray &opusData) {
    if (!m_decoder)
        return QByteArray();

    // Max frame size for 12kHz = 120ms * 12000 = 1440 samples per channel
    const int maxFrameSize = 1440;
    QVector<float> pcm(maxFrameSize * m_channels);

    int samples = opus_decode_float(m_decoder, reinterpret_cast<const unsigned char *>(opusData.constData()),
                                    opusData.size(), pcm.data(), maxFrameSize, 0);

    if (samples < 0) {
        qWarning() << "OpusDecoder: Float decode failed:" << opus_strerror(samples);
        return QByteArray();
    }

    return QByteArray(reinterpret_cast<const char *>(pcm.constData()), samples * m_channels * sizeof(float));
}
