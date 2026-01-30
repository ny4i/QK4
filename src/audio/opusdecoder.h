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
    // Mix interleaved stereo floats to mono with volume control and gain
    // stereoFloats: interleaved [L0, R0, L1, R1, ...] normalized to ±1.0
    // sampleCount: number of samples per channel (total floats = sampleCount * 2)
    // gainBoost: multiplier (1.0 for raw, K4_GAIN_BOOST for compressed formats)
    QByteArray mixStereoToMono(const float *stereoFloats, int sampleCount, float gainBoost);

    ::OpusDecoder *m_decoder;
    int m_sampleRate;
    int m_channels;

    // Channel volume controls (0.0 to 1.0)
    float m_mainVolume = 1.0f;
    float m_subVolume = 1.0f;

    // Normalization constants
    static constexpr float NORMALIZE_16BIT = 1.0f / 32768.0f;
    static constexpr float NORMALIZE_32BIT = 1.0f / 2147483648.0f;

    // K4-specific gain boost (Opus and S32LE audio is very quiet)
    // Note: EM1 (S16LE RAW) is already at full scale and doesn't need boost
    static constexpr float K4_GAIN_BOOST = 32.0f;
};

#endif // OPUSDECODER_H
