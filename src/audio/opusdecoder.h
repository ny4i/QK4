#ifndef OPUSDECODER_H
#define OPUSDECODER_H

#include <QObject>
#include <opus/opus.h>

class OpusDecoder : public QObject {
    Q_OBJECT

public:
    enum MixSource { MixA = 0, MixB = 1, MixAB = 2, MixNegA = 3 };

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

    // SUB RX mute control (when sub receiver is off, sub channel is silent)
    void setSubMuted(bool muted);

    // Audio mix routing (MX command - how main/sub maps to L/R when SUB is on)
    void setAudioMix(int left, int right); // MixSource values

    // Balance mode control (0=NOR, 1=BAL)
    void setBalanceMode(int mode);
    void setBalanceOffset(int offset); // -50 to +50

private:
    // Mix interleaved stereo floats to stereo output with volume/balance control
    // stereoFloats: interleaved [L0, R0, L1, R1, ...] normalized to ±1.0
    // sampleCount: number of samples per channel (total floats = sampleCount * 2)
    // gainBoost: multiplier (1.0 for raw, K4_GAIN_BOOST for compressed formats)
    // Returns interleaved stereo Float32 [L0, R0, L1, R1, ...]
    QByteArray mixToStereo(const float *stereoFloats, int sampleCount, float gainBoost);

    ::OpusDecoder *m_decoder;
    int m_sampleRate;
    int m_channels;

    // Channel volume controls (0.0 to 1.0)
    float m_mainVolume = 1.0f;
    float m_subVolume = 1.0f;

    // SUB RX mute state (true = sub muted, sub channel is silent)
    bool m_subMuted = true; // Starts muted (SUB RX is off at startup)

    // Audio mix routing (MX command) - default A.B (main left, sub right)
    MixSource m_mixLeft = MixA;
    MixSource m_mixRight = MixB;

    // Balance mode (0=NOR: independent volume, 1=BAL: L/R balance)
    int m_balanceMode = 0;
    int m_balanceOffset = 0; // -50 to +50

    // Normalization constants
    static constexpr float NORMALIZE_16BIT = 1.0f / 32768.0f;
    static constexpr float NORMALIZE_32BIT = 1.0f / 2147483648.0f;

    // K4-specific gain boost (Opus and S32LE audio is very quiet)
    // Note: EM1 (S16LE RAW) is already at full scale and doesn't need boost
    static constexpr float K4_GAIN_BOOST = 32.0f;
};

#endif // OPUSDECODER_H
