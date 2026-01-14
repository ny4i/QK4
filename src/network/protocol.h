#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QObject>
#include <QByteArray>

// K4 Protocol Constants
namespace K4Protocol {
// Packet markers
static const QByteArray START_MARKER = QByteArray::fromHex("FEFDFCFB");
static const QByteArray END_MARKER = QByteArray::fromHex("FBFCFDFE");

// Payload types (first byte of payload)
enum PayloadType : quint8 {
    CAT = 0x00,    // CAT command (ASCII)
    Audio = 0x01,  // Audio data (Opus)
    PAN = 0x02,    // Panadapter/spectrum data
    MiniPAN = 0x03 // Mini panadapter (unused)
};

// Default K4 ports
static const quint16 DEFAULT_PORT = 9205; // Unencrypted (SHA-384 auth)
static const quint16 TLS_PORT = 9204;     // TLS/PSK encrypted

// Timing constants
static const int PING_INTERVAL_MS = 1000;       // 1 second (matches SIRC update interval)
static const int CONNECTION_TIMEOUT_MS = 10000; // 10 seconds
static const int AUTH_TIMEOUT_MS = 5000;        // 5 seconds for auth response
} // namespace K4Protocol

class Protocol : public QObject {
    Q_OBJECT

public:
    explicit Protocol(QObject *parent = nullptr);

    // Parse incoming raw data, extracts complete K4 packets
    void parse(const QByteArray &data);

    // Build a K4 packet from payload
    static QByteArray buildPacket(const QByteArray &payload);

    // Build a CAT command packet
    static QByteArray buildCATPacket(const QString &command);

    // Build authentication data (SHA-384 hash as hex string)
    static QByteArray buildAuthData(const QString &password);

    // Build TX audio packet (Opus encoded data with K4 audio header)
    // sequence: 0-255, wrapping counter for packet ordering
    // encodeMode: 0=RAW32, 1=RAW16, 2=Opus Int, 3=Opus Float (default)
    static QByteArray buildAudioPacket(const QByteArray &audioData, quint8 sequence, quint8 encodeMode = 0x03);

signals:
    void audioDataReady(const QByteArray &opusData);
    // receiver: 0 = Main (VFO A), 1 = Sub (VFO B)
    void spectrumDataReady(int receiver, const QByteArray &spectrumData, qint64 centerFreq, qint32 sampleRate,
                           float noiseFloor);
    void miniSpectrumDataReady(int receiver, const QByteArray &spectrumData);
    void catResponseReceived(const QString &response);
    void packetReceived(quint8 type, const QByteArray &payload);

private:
    void processPacket(const QByteArray &packet);

    QByteArray m_buffer;
};

#endif // PROTOCOL_H
