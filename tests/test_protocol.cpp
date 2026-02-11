#include <QTest>
#include <QSignalSpy>
#include <QCryptographicHash>
#include <QtEndian>
#include "network/protocol.h"

class TestProtocol : public QObject {
    Q_OBJECT

private:
    // Helper: wrap a payload in START_MARKER + big-endian length + payload + END_MARKER
    static QByteArray wrapPacket(const QByteArray &payload) {
        QByteArray pkt;
        pkt.append(K4Protocol::START_MARKER);
        quint32 len = payload.size();
        char lenBytes[4];
        qToBigEndian(len, reinterpret_cast<uchar *>(lenBytes));
        pkt.append(lenBytes, 4);
        pkt.append(payload);
        pkt.append(K4Protocol::END_MARKER);
        return pkt;
    }

    // Helper: build a CAT payload (0x00 0x00 0x00 + ASCII)
    static QByteArray catPayload(const QString &cmd) {
        QByteArray p;
        p.append(static_cast<char>(K4Protocol::CAT));
        p.append('\x00');
        p.append('\x00');
        p.append(cmd.toLatin1());
        return p;
    }

private slots:
    // =========================================================================
    // buildPacket
    // =========================================================================
    void testBuildPacket_structure() {
        QByteArray payload("HELLO");
        QByteArray pkt = Protocol::buildPacket(payload);

        // Start marker (4 bytes)
        QCOMPARE(pkt.left(4), K4Protocol::START_MARKER);

        // Length field (big-endian uint32) = 5
        quint32 len = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(pkt.constData() + 4));
        QCOMPARE(len, static_cast<quint32>(5));

        // Payload
        QCOMPARE(pkt.mid(8, 5), payload);

        // End marker (4 bytes)
        QCOMPARE(pkt.right(4), K4Protocol::END_MARKER);

        // Total size: 4 + 4 + 5 + 4 = 17
        QCOMPARE(pkt.size(), 17);
    }

    void testBuildPacket_emptyPayload() {
        QByteArray pkt = Protocol::buildPacket(QByteArray());
        QCOMPARE(pkt.size(), 12); // 4 + 4 + 0 + 4
        quint32 len = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(pkt.constData() + 4));
        QCOMPARE(len, static_cast<quint32>(0));
    }

    // =========================================================================
    // buildCATPacket
    // =========================================================================
    void testBuildCATPacket() {
        QByteArray pkt = Protocol::buildCATPacket("FA00014060000;");

        // Unwrap: skip start(4) + length(4) to get payload
        quint32 payloadLen = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(pkt.constData() + 4));
        QByteArray payload = pkt.mid(8, payloadLen);

        // Payload: 0x00 0x00 0x00 + "FA00014060000;"
        QCOMPARE(static_cast<quint8>(payload[0]), static_cast<quint8>(K4Protocol::CAT));
        QCOMPARE(static_cast<quint8>(payload[1]), static_cast<quint8>(0x00));
        QCOMPARE(static_cast<quint8>(payload[2]), static_cast<quint8>(0x00));
        QCOMPARE(payload.mid(3), QByteArray("FA00014060000;"));
    }

    // =========================================================================
    // buildAuthData
    // =========================================================================
    void testBuildAuthData_sha384() {
        QByteArray result = Protocol::buildAuthData("testpassword");

        // Independently compute SHA-384
        QByteArray expected = QCryptographicHash::hash("testpassword", QCryptographicHash::Sha384).toHex().toLower();
        QCOMPARE(result, expected);

        // SHA-384 hex string is 96 characters
        QCOMPARE(result.size(), 96);
    }

    void testBuildAuthData_emptyPassword() {
        QByteArray result = Protocol::buildAuthData("");
        QByteArray expected = QCryptographicHash::hash(QByteArray(), QCryptographicHash::Sha384).toHex().toLower();
        QCOMPARE(result, expected);
    }

    // =========================================================================
    // buildAudioPacket
    // =========================================================================
    void testBuildAudioPacket_headerBytes() {
        QByteArray audio(100, '\xAB');
        quint8 seq = 42;
        quint8 mode = 0x03; // Opus Float

        QByteArray pkt = Protocol::buildAudioPacket(audio, seq, mode);

        // Unwrap packet
        quint32 payloadLen = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(pkt.constData() + 4));
        QByteArray payload = pkt.mid(8, payloadLen);

        // Header: type(1) + ver(1) + seq(1) + mode(1) + frameSize(2 LE) + sampleRate(1) = 7 bytes
        QVERIFY(payload.size() == 7 + 100);

        QCOMPARE(static_cast<quint8>(payload[0]), static_cast<quint8>(K4Protocol::Audio)); // 0x01
        QCOMPARE(static_cast<quint8>(payload[1]), static_cast<quint8>(0x01)); // version
        QCOMPARE(static_cast<quint8>(payload[2]), seq);
        QCOMPARE(static_cast<quint8>(payload[3]), mode);

        // Frame size: 240 little-endian
        quint16 frameSize = static_cast<quint8>(payload[4]) | (static_cast<quint8>(payload[5]) << 8);
        QCOMPARE(frameSize, static_cast<quint16>(240));

        // Sample rate code: 0x00 (12kHz)
        QCOMPARE(static_cast<quint8>(payload[6]), static_cast<quint8>(0x00));

        // Audio data follows
        QCOMPARE(payload.mid(7), audio);
    }

    void testBuildAudioPacket_defaultMode() {
        QByteArray audio(10, '\x00');
        QByteArray pkt = Protocol::buildAudioPacket(audio, 0);

        quint32 payloadLen = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(pkt.constData() + 4));
        QByteArray payload = pkt.mid(8, payloadLen);

        // Default mode is 0x03 (Opus Float)
        QCOMPARE(static_cast<quint8>(payload[3]), static_cast<quint8>(0x03));
    }

    void testBuildAudioPacket_sequenceWraps() {
        QByteArray audio(1, '\x00');
        QByteArray pkt = Protocol::buildAudioPacket(audio, 255);

        quint32 payloadLen = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(pkt.constData() + 4));
        QByteArray payload = pkt.mid(8, payloadLen);
        QCOMPARE(static_cast<quint8>(payload[2]), static_cast<quint8>(255));
    }

    // =========================================================================
    // parse → catResponseReceived
    // =========================================================================
    void testParse_catResponse() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::catResponseReceived);

        QByteArray packet = wrapPacket(catPayload("FA00014060000;"));
        proto.parse(packet);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("FA00014060000;"));
    }

    // =========================================================================
    // parse → audioDataReady
    // =========================================================================
    void testParse_audioPacket() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::audioDataReady);

        // Build audio payload: header (7 bytes) + data
        QByteArray payload;
        payload.append(static_cast<char>(K4Protocol::Audio)); // type
        payload.append(static_cast<char>(0x01));              // version
        payload.append(static_cast<char>(0x00));              // sequence
        payload.append(static_cast<char>(0x03));              // mode
        payload.append(static_cast<char>(0xF0));              // frame size low
        payload.append(static_cast<char>(0x00));              // frame size high
        payload.append(static_cast<char>(0x00));              // sample rate
        payload.append("OPUSDATA", 8);                        // audio data

        proto.parse(wrapPacket(payload));

        QCOMPARE(spy.count(), 1);
        // audioDataReady emits the entire payload (header + data)
        QByteArray emitted = spy.at(0).at(0).toByteArray();
        QCOMPARE(emitted, payload);
    }

    // =========================================================================
    // parse → spectrumDataReady (PAN packet)
    // =========================================================================
    void testParse_panPacket() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::spectrumDataReady);

        // Build PAN payload
        QByteArray payload(K4Protocol::PanPacket::HEADER_SIZE + 4, '\x00');
        payload[K4Protocol::PanPacket::TYPE_OFFSET] = static_cast<char>(K4Protocol::PAN);
        payload[K4Protocol::PanPacket::RECEIVER_OFFSET] = static_cast<char>(0); // Main RX

        // Center freq: 14060000 Hz (little-endian int64)
        qint64 centerFreq = 14060000;
        qToLittleEndian(centerFreq, reinterpret_cast<uchar *>(payload.data() + K4Protocol::PanPacket::CENTER_FREQ_OFFSET));

        // Sample rate: 48000 (little-endian int32)
        qint32 sampleRate = 48000;
        qToLittleEndian(sampleRate, reinterpret_cast<uchar *>(payload.data() + K4Protocol::PanPacket::SAMPLE_RATE_OFFSET));

        // Noise floor: -1200 (raw) = -120.0 dB
        qint32 noiseFloorRaw = -1200;
        qToLittleEndian(noiseFloorRaw, reinterpret_cast<uchar *>(payload.data() + K4Protocol::PanPacket::NOISE_FLOOR_OFFSET));

        // Bin data
        payload[K4Protocol::PanPacket::BINS_OFFSET] = 0x10;
        payload[K4Protocol::PanPacket::BINS_OFFSET + 1] = 0x20;
        payload[K4Protocol::PanPacket::BINS_OFFSET + 2] = 0x30;
        payload[K4Protocol::PanPacket::BINS_OFFSET + 3] = 0x40;

        proto.parse(wrapPacket(payload));

        QCOMPARE(spy.count(), 1);
        auto args = spy.at(0);
        QCOMPARE(args.at(0).toInt(), 0);                    // receiver = Main
        QCOMPARE(args.at(2).toLongLong(), qint64(14060000)); // centerFreq
        QCOMPARE(args.at(3).toInt(), 48000);                 // sampleRate
        QCOMPARE(args.at(4).toFloat(), -120.0f);             // noiseFloor

        QByteArray bins = args.at(1).toByteArray();
        QCOMPARE(bins.size(), 4);
    }

    // =========================================================================
    // parse → miniSpectrumDataReady (MiniPAN packet)
    // =========================================================================
    void testParse_miniPanPacket() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::miniSpectrumDataReady);

        QByteArray payload(K4Protocol::MiniPanPacket::HEADER_SIZE + 3, '\x00');
        payload[K4Protocol::MiniPanPacket::TYPE_OFFSET] = static_cast<char>(K4Protocol::MiniPAN);
        payload[K4Protocol::MiniPanPacket::RECEIVER_OFFSET] = static_cast<char>(1); // Sub RX

        // Bin data
        payload[K4Protocol::MiniPanPacket::BINS_OFFSET] = 0xAA;
        payload[K4Protocol::MiniPanPacket::BINS_OFFSET + 1] = 0xBB;
        payload[K4Protocol::MiniPanPacket::BINS_OFFSET + 2] = 0xCC;

        proto.parse(wrapPacket(payload));

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1); // receiver = Sub
        QByteArray bins = spy.at(0).at(1).toByteArray();
        QCOMPARE(bins.size(), 3);
        QCOMPARE(static_cast<quint8>(bins[0]), static_cast<quint8>(0xAA));
    }

    // =========================================================================
    // parse with split data (reassembly)
    // =========================================================================
    void testParse_splitPacket() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::catResponseReceived);

        QByteArray packet = wrapPacket(catPayload("MD3;"));

        // Split at arbitrary point (half the packet)
        int splitPoint = packet.size() / 2;
        QByteArray part1 = packet.left(splitPoint);
        QByteArray part2 = packet.mid(splitPoint);

        proto.parse(part1);
        QCOMPARE(spy.count(), 0); // Not yet complete

        proto.parse(part2);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("MD3;"));
    }

    // =========================================================================
    // parse with garbage prefix
    // =========================================================================
    void testParse_garbageBeforeStartMarker() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::catResponseReceived);

        QByteArray garbage(16, '\xFF');
        QByteArray packet = wrapPacket(catPayload("RX;"));

        proto.parse(garbage + packet);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("RX;"));
    }

    // =========================================================================
    // parse with bad end marker
    // =========================================================================
    void testParse_badEndMarker() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::catResponseReceived);

        // Build a packet with corrupted end marker
        QByteArray payload = catPayload("BAD;");
        QByteArray badPacket;
        badPacket.append(K4Protocol::START_MARKER);
        quint32 len = payload.size();
        char lenBytes[4];
        qToBigEndian(len, reinterpret_cast<uchar *>(lenBytes));
        badPacket.append(lenBytes, 4);
        badPacket.append(payload);
        badPacket.append(QByteArray(4, '\x00')); // Bad end marker

        // Followed by a good packet
        QByteArray goodPacket = wrapPacket(catPayload("GOOD;"));

        proto.parse(badPacket + goodPacket);

        // Only the good packet should be received
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("GOOD;"));
    }

    // =========================================================================
    // parse buffer overflow protection
    // =========================================================================
    void testParse_bufferOverflow() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::catResponseReceived);

        // Feed >1MB of data without valid packets
        QByteArray hugeData(K4Protocol::MAX_BUFFER_SIZE + 1, '\xAA');
        proto.parse(hugeData);

        // Buffer should be cleared; no crash
        QCOMPARE(spy.count(), 0);

        // After overflow, protocol should still work
        proto.parse(wrapPacket(catPayload("OK;")));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("OK;"));
    }

    // =========================================================================
    // parse empty payload
    // =========================================================================
    void testParse_emptyPayload() {
        Protocol proto;
        QSignalSpy catSpy(&proto, &Protocol::catResponseReceived);
        QSignalSpy audioSpy(&proto, &Protocol::audioDataReady);
        QSignalSpy specSpy(&proto, &Protocol::spectrumDataReady);

        // Empty payload packet
        proto.parse(wrapPacket(QByteArray()));

        // processPacket returns early on empty payload - no signals
        QCOMPARE(catSpy.count(), 0);
        QCOMPARE(audioSpy.count(), 0);
        QCOMPARE(specSpy.count(), 0);
    }

    // =========================================================================
    // parse multiple packets in one chunk
    // =========================================================================
    void testParse_multiplePackets() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::catResponseReceived);

        QByteArray data;
        data.append(wrapPacket(catPayload("FA00014060000;")));
        data.append(wrapPacket(catPayload("MD3;")));
        data.append(wrapPacket(catPayload("BW0240;")));

        proto.parse(data);

        QCOMPARE(spy.count(), 3);
        QCOMPARE(spy.at(0).at(0).toString(), QString("FA00014060000;"));
        QCOMPARE(spy.at(1).at(0).toString(), QString("MD3;"));
        QCOMPARE(spy.at(2).at(0).toString(), QString("BW0240;"));
    }

    // =========================================================================
    // parse: packetReceived signal fires for all types
    // =========================================================================
    void testParse_packetReceivedSignal() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::packetReceived);

        proto.parse(wrapPacket(catPayload("TEST;")));

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).value<quint8>(), static_cast<quint8>(K4Protocol::CAT));
    }

    // =========================================================================
    // CAT payload too short (<=3 bytes) → no catResponseReceived
    // =========================================================================
    void testParse_catPayloadTooShort() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::catResponseReceived);

        // 3-byte CAT payload: type + 2 zeros, no ASCII data
        QByteArray shortPayload;
        shortPayload.append(static_cast<char>(K4Protocol::CAT));
        shortPayload.append('\x00');
        shortPayload.append('\x00');

        proto.parse(wrapPacket(shortPayload));

        // catResponseReceived requires payload.size() > 3
        QCOMPARE(spy.count(), 0);
    }

    // =========================================================================
    // Audio payload too short → no audioDataReady
    // =========================================================================
    void testParse_audioPayloadTooShort() {
        Protocol proto;
        QSignalSpy spy(&proto, &Protocol::audioDataReady);

        // Audio payload with only header, no data (exactly HEADER_SIZE bytes)
        QByteArray shortPayload(K4Protocol::AudioPacket::HEADER_SIZE, '\x00');
        shortPayload[0] = static_cast<char>(K4Protocol::Audio);

        proto.parse(wrapPacket(shortPayload));

        // audioDataReady requires payload.size() > HEADER_SIZE
        QCOMPARE(spy.count(), 0);
    }
};

QTEST_MAIN(TestProtocol)
#include "test_protocol.moc"
