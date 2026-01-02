#include "protocol.h"
#include <QCryptographicHash>
#include <QtEndian>
#include <QDebug>

Protocol::Protocol(QObject *parent) : QObject(parent) {}

void Protocol::parse(const QByteArray &data) {
    m_buffer.append(data);

    // Process all complete packets in the buffer
    // All K4 data comes wrapped in binary packets with START/END markers
    while (true) {
        // Look for start marker
        int startPos = m_buffer.indexOf(K4Protocol::START_MARKER);
        if (startPos == -1) {
            // No start marker found, clear buffer up to last 3 bytes
            // (in case partial marker is at the end)
            if (m_buffer.size() > 3) {
                m_buffer = m_buffer.right(3);
            }
            break;
        }

        // Discard any data before the start marker
        if (startPos > 0) {
            m_buffer = m_buffer.mid(startPos);
        }

        // Check if we have enough data for the header (4 marker + 4 length = 8 bytes)
        if (m_buffer.size() < 8) {
            break;
        }

        // Read payload length (big-endian uint32)
        quint32 payloadLength = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(m_buffer.constData() + 4));

        // Calculate total packet size: start(4) + length(4) + payload + end(4)
        int totalPacketSize = 4 + 4 + payloadLength + 4;

        // Check if we have the complete packet
        if (m_buffer.size() < totalPacketSize) {
            break;
        }

        // Verify end marker
        QByteArray endMarker = m_buffer.mid(totalPacketSize - 4, 4);
        if (endMarker != K4Protocol::END_MARKER) {
            // Invalid packet, skip past the start marker and try again
            qWarning() << "Invalid K4 packet: bad end marker";
            m_buffer = m_buffer.mid(4);
            continue;
        }

        // Extract payload
        QByteArray payload = m_buffer.mid(8, payloadLength);

        // Remove processed packet from buffer
        m_buffer = m_buffer.mid(totalPacketSize);

        // Process the complete packet
        processPacket(payload);
    }
}

void Protocol::processPacket(const QByteArray &payload) {
    if (payload.isEmpty()) {
        return;
    }

    quint8 type = static_cast<quint8>(payload[0]);
    emit packetReceived(type, payload);

    switch (type) {
    case K4Protocol::CAT: {
        // CAT response: [0x00][0x00][0x00][ASCII data]
        if (payload.size() > 3) {
            QString response = QString::fromLatin1(payload.mid(3));
            emit catResponseReceived(response);
        }
        break;
    }
    case K4Protocol::Audio: {
        // Audio packet: [0x01][version][seq][mode][...opus data]
        if (payload.size() > 6) {
            emit audioDataReady(payload);
        }
        break;
    }
    case K4Protocol::PAN: {
        // PAN Type 2 payload structure (from K4-Remote Protocol Rev. A1 draft):
        // Byte 0: Type (0x02 for PAN)
        // Byte 1: RX (0=Main, 1=Sub)
        // Bytes 2-3: PAN Data Byte Length (uint16)
        // Bytes 4-7: Reserved
        // Bytes 8-15: Center Frequency (int64 LE, Hz)
        // Bytes 16-19: Sample Rate (int32 LE) - tier indicator
        // Bytes 20-23: Noise Floor (int32 LE, divide by 10 for dB)
        // Bytes 24+: Compressed bin data
        if (payload.size() > 24) {
            int receiver = static_cast<quint8>(payload[1]); // 0=Main, 1=Sub
            qint64 centerFreq = qFromLittleEndian<qint64>(reinterpret_cast<const uchar *>(payload.constData() + 8));
            qint32 sampleRate = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(payload.constData() + 16));
            qint32 noiseFloorRaw = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(payload.constData() + 20));
            float noiseFloor = noiseFloorRaw / 10.0f;

            QByteArray bins = payload.mid(24);
            emit spectrumDataReady(receiver, bins, centerFreq, sampleRate, noiseFloor);
        }
        break;
    }
    case K4Protocol::MiniPAN: {
        // MiniPAN Type 3 payload structure (discovered via packet analysis):
        // Byte 0: TYPE (0x03)
        // Byte 1: VER (0x00)
        // Byte 2: SEQ (sequence)
        // Byte 3: Reserved (0x00)
        // Byte 4: RX (0=Main, 1=Sub)
        // Byte 5+: Mini PAN Data
        if (payload.size() > 5) {
            int receiver = static_cast<quint8>(payload[4]); // RX byte
            QByteArray bins = payload.mid(5);
            emit miniSpectrumDataReady(receiver, bins);
        }
        break;
    }
    default:
        qDebug() << "Unknown K4 packet type:" << type;
        break;
    }
}

QByteArray Protocol::buildPacket(const QByteArray &payload) {
    QByteArray packet;
    packet.reserve(payload.size() + 12);

    // Start marker
    packet.append(K4Protocol::START_MARKER);

    // Payload length (big-endian)
    quint32 length = payload.size();
    char lengthBytes[4];
    qToBigEndian(length, reinterpret_cast<uchar *>(lengthBytes));
    packet.append(lengthBytes, 4);

    // Payload
    packet.append(payload);

    // End marker
    packet.append(K4Protocol::END_MARKER);

    return packet;
}

QByteArray Protocol::buildCATPacket(const QString &command) {
    QByteArray payload;
    payload.append(static_cast<char>(K4Protocol::CAT));
    payload.append('\x00');
    payload.append('\x00');
    payload.append(command.toLatin1());

    return buildPacket(payload);
}

QByteArray Protocol::buildAuthData(const QString &password) {
    // SHA-384 hash of password
    QByteArray passwordData = password.toUtf8();
    QByteArray hash = QCryptographicHash::hash(passwordData, QCryptographicHash::Sha384);

    // Convert to lowercase hex string
    QByteArray hexString = hash.toHex().toLower();

    return hexString;
}
