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
    // Byte 3: Encode Mode (0x03 = Opus Float)
    // Bytes 4-5: Frame size (little-endian UInt16) - samples per channel
    // Byte 6: Sample rate code (0 = 12000 Hz)
    // Byte 7+: Opus encoded stereo audio data (left = VFO A, right = VFO B)

    if (packet.size() < 8) {
        return QByteArray();
    }

    // Verify packet type
    if (static_cast<unsigned char>(packet[0]) != 0x01) {
        return QByteArray();
    }

    // Verify encode mode is EM3 (0x03 = Opus Float 32-bit)
    unsigned char encodeMode = static_cast<unsigned char>(packet[3]);
    if (encodeMode != 0x03) {
        static bool warned = false;
        if (!warned) {
            qWarning() << "OpusDecoder: Unexpected encode mode:" << encodeMode << "(expected 0x03 for EM3 Opus Float)";
            warned = true;
        }
        return QByteArray(); // Skip non-EM3 packets
    }

    // Extract frame size (little-endian UInt16 at bytes 4-5)
    quint16 frameSize = static_cast<unsigned char>(packet[4]) | (static_cast<unsigned char>(packet[5]) << 8);

    // Extract Opus data starting at byte 7
    QByteArray opusData = packet.mid(7);

    if (opusData.isEmpty()) {
        return QByteArray();
    }

    // Decode Opus to stereo PCM
    QByteArray stereoPcm = decode(opusData);
    if (stereoPcm.isEmpty()) {
        return QByteArray();
    }

    // De-interleave and mix stereo channels with volume control
    // Stereo is interleaved as [L0, R0, L1, R1, ...]
    // Left channel = Main RX (VFO A), Right channel = Sub RX (VFO B)
    // Each sample is 16-bit (2 bytes), so stereo frame = 4 bytes per sample pair

    const qint16 *stereoSamples = reinterpret_cast<const qint16 *>(stereoPcm.constData());
    int totalStereoSamples = stereoPcm.size() / sizeof(qint16);
    int monoSampleCount = totalStereoSamples / 2;

    // Output: float32 PCM mixed from both channels (with gain boost and volume)
    QByteArray monoPcm(monoSampleCount * sizeof(float), 0);
    float *monoFloats = reinterpret_cast<float *>(monoPcm.data());

    for (int i = 0; i < monoSampleCount; i++) {
        // Extract both channels and convert to float
        qint16 mainSample = stereoSamples[i * 2];    // Left channel (Main RX / VFO A)
        qint16 subSample = stereoSamples[i * 2 + 1]; // Right channel (Sub RX / VFO B)

        float mainNormalized = static_cast<float>(mainSample) / 32768.0f;
        float subNormalized = static_cast<float>(subSample) / 32768.0f;

        // Apply per-channel volume and K4 gain boost, then mix
        float mainWithVolume = mainNormalized * m_mainVolume * K4_GAIN_BOOST;
        float subWithVolume = subNormalized * m_subVolume * K4_GAIN_BOOST;

        // Mix both channels (simple sum - both at 100% would double volume)
        monoFloats[i] = mainWithVolume + subWithVolume;

        // Clamp to prevent clipping
        if (monoFloats[i] > 1.0f)
            monoFloats[i] = 1.0f;
        if (monoFloats[i] < -1.0f)
            monoFloats[i] = -1.0f;
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
