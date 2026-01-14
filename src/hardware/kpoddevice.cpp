#include "kpoddevice.h"
#include <hidapi/hidapi.h>
#include <QDebug>

#ifdef Q_OS_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

KpodDevice::KpodDevice(QObject *parent)
    : QObject(parent), m_hidDevice(nullptr), m_pollTimer(new QTimer(this)), m_lastRockerPosition(RockerCenter) {
    m_deviceInfo = detectDevice();

    // Setup poll timer
    m_pollTimer->setInterval(POLL_INTERVAL_MS);
    connect(m_pollTimer, &QTimer::timeout, this, &KpodDevice::poll);

    // Setup hotplug monitoring for device arrival/removal
    setupHotplugMonitoring();
}

KpodDevice::~KpodDevice() {
    stopPolling();
    teardownHotplugMonitoring();
}

bool KpodDevice::isDetected() const {
    return m_deviceInfo.detected;
}

KpodDeviceInfo KpodDevice::deviceInfo() const {
    return m_deviceInfo;
}

bool KpodDevice::startPolling() {
    if (m_pollTimer->isActive()) {
        return true; // Already polling
    }

    if (!openDevice()) {
        emit pollError("Failed to open KPOD device");
        return false;
    }

    // Reset state tracking
    m_lastRockerPosition = RockerCenter;
    m_lastButtonState = 0;
    m_holdEmitted = false;

    m_pollTimer->start();
    emit deviceConnected();
    return true;
}

void KpodDevice::stopPolling() {
    if (m_pollTimer->isActive()) {
        m_pollTimer->stop();
    }
    closeDevice();
}

bool KpodDevice::isPolling() const {
    return m_pollTimer->isActive();
}

KpodDevice::RockerPosition KpodDevice::rockerPosition() const {
    return m_lastRockerPosition;
}

bool KpodDevice::openDevice() {
    if (m_hidDevice) {
        return true; // Already open
    }

    if (hid_init() != 0) {
        qWarning() << "KPOD: Failed to initialize hidapi";
        return false;
    }

    m_hidDevice = hid_open(VENDOR_ID, PRODUCT_ID, nullptr);
    if (!m_hidDevice) {
        qWarning() << "KPOD: Failed to open device";
        hid_exit();
        return false;
    }

    // Set non-blocking mode for responsive polling
    hid_set_nonblocking(m_hidDevice, 1);

    return true;
}

void KpodDevice::closeDevice() {
    if (m_hidDevice) {
        hid_close(m_hidDevice);
        m_hidDevice = nullptr;
        hid_exit();
        emit deviceDisconnected();
    }
}

void KpodDevice::poll() {
    if (!m_hidDevice) {
        stopPolling();
        emit pollError("Device handle invalid");
        return;
    }

    // Send update request command
    unsigned char cmd[8] = {'u', 0, 0, 0, 0, 0, 0, 0};
    int writeResult = hid_write(m_hidDevice, cmd, sizeof(cmd));

    if (writeResult < 0) {
        // Write failed - device may be disconnected
        qWarning() << "KPOD: Write failed, device disconnected?";
        stopPolling();
        emit pollError("Failed to write to KPOD");
        return;
    }

    // Read response (short timeout to avoid blocking event loop)
    unsigned char buffer[8];
    int readResult = hid_read_timeout(m_hidDevice, buffer, sizeof(buffer), 5);

    if (readResult < 0) {
        // Read error - device may be disconnected
        qWarning() << "KPOD: Read failed, device disconnected?";
        stopPolling();
        emit pollError("Failed to read from KPOD");
        return;
    }

    // Process response if we got data
    // Note: According to spec, cmd='u' only for new events, but we always
    // check controls byte for button/rocker state changes
    if (readResult == 8) {
        processResponse(buffer);
    }
    // readResult == 0 means no data available (normal for non-blocking)
}

void KpodDevice::processResponse(const unsigned char *buffer) {
    // KPOD Protocol (from spec):
    // - buffer[0] = cmd: 'u' (0x75) if new event, 0 if no event
    // - buffer[1-2] = ticks: signed 16-bit encoder count (little-endian)
    // - buffer[3] = controls: button/tap-hold/rocker state (ONLY valid when cmd='u')
    // - buffer[4-7] = spare

    unsigned char cmd = buffer[0];

    // Protocol: cmd='u' means new event, cmd=0 means no event
    // However, if we have a pending button press and see cmd=0, that's a release
    if (cmd != 'u') {
        // Check for implicit button release (button was pressed, now no event)
        if (m_lastButtonState != 0) {
            if (!m_holdEmitted) {
                emit buttonTapped(m_lastButtonState);
            }
            m_lastButtonState = 0;
            m_holdEmitted = false;
        }
        return;
    }

    // Extract encoder ticks (signed int16, little-endian)
    qint16 encoderTicks = static_cast<qint16>(buffer[1] | (buffer[2] << 8));

    // Extract controls byte (now guaranteed to be valid since cmd == 'u')
    quint8 controls = buffer[3];

    // Parse controls byte:
    // bit 7: unused
    // bit 6-5: Rocker (00=center/VFO B, 01=right/RIT-XIT, 10=left/VFO A, 11=error)
    // bit 4: Tap/Hold (0=tap, 1=hold)
    // bit 3-0: Button (0x01=btn1, 0x02=btn2, ..., 0x08=btn8)
    quint8 buttonNum = controls & 0x0F;
    bool isHold = (controls >> 4) & 0x01;
    RockerPosition rocker = static_cast<RockerPosition>((controls >> 5) & 0x03);

    // Emit encoder signal if there was rotation
    if (encoderTicks != 0) {
        emit encoderRotated(encoderTicks);
    }

    // Button state machine:
    // - KPOD sends hold=false initially, then hold=true after hold threshold
    // - If hold becomes true while pressed, emit buttonHeld
    // - If released while hold=false, emit buttonTapped
    if (buttonNum != 0 && m_lastButtonState == 0) {
        // Button just pressed - start tracking
        m_lastButtonState = buttonNum;
        m_holdEmitted = false;
        if (isHold) {
            // Already a hold at first press (unusual but handle it)
            emit buttonHeld(buttonNum);
            m_holdEmitted = true;
        }
    } else if (buttonNum != 0 && m_lastButtonState != 0) {
        // Button still pressed - check if hold flag became true
        if (isHold && !m_holdEmitted) {
            emit buttonHeld(m_lastButtonState);
            m_holdEmitted = true;
        }
    } else if (buttonNum == 0 && m_lastButtonState != 0) {
        // Button released - emit tap only if it wasn't a hold
        if (!m_holdEmitted) {
            emit buttonTapped(m_lastButtonState);
        }
        m_lastButtonState = 0;
        m_holdEmitted = false;
    }

    // Rocker position - update on change (rocker=3 is error state)
    if (rocker != m_lastRockerPosition && rocker != 3) {
        m_lastRockerPosition = rocker;
        emit rockerPositionChanged(rocker);
    }
}

KpodDeviceInfo KpodDevice::detectDevice() {
    KpodDeviceInfo info;

    if (hid_init() != 0) {
        return info;
    }

    struct hid_device_info *devs = hid_enumerate(VENDOR_ID, PRODUCT_ID);
    struct hid_device_info *cur_dev = devs;

    if (cur_dev) {
        info.detected = true;
        info.vendorId = cur_dev->vendor_id;
        info.productId = cur_dev->product_id;

        if (cur_dev->product_string) {
            info.productName = QString::fromWCharArray(cur_dev->product_string);
        }
        if (cur_dev->manufacturer_string) {
            info.manufacturer = QString::fromWCharArray(cur_dev->manufacturer_string);
        }
        if (cur_dev->path) {
            info.devicePath = QString::fromUtf8(cur_dev->path);
        }

        // Try to open device to get firmware version and device ID
        hid_device *dev = hid_open(VENDOR_ID, PRODUCT_ID, nullptr);
        if (dev) {
            unsigned char buf[8];

            // Get Device ID (command '=')
            memset(buf, 0, sizeof(buf));
            buf[0] = '=';
            if (hid_write(dev, buf, sizeof(buf)) > 0) {
                if (hid_read_timeout(dev, buf, sizeof(buf), 100) == 8) {
                    QString idStr;
                    for (int i = 1; i < 8 && buf[i] != 0; ++i) {
                        idStr += QChar(buf[i]);
                    }
                    info.deviceId = idStr.trimmed();
                }
            }

            // Get Firmware Version (command 'v')
            memset(buf, 0, sizeof(buf));
            buf[0] = 'v';
            if (hid_write(dev, buf, sizeof(buf)) > 0) {
                if (hid_read_timeout(dev, buf, sizeof(buf), 100) == 8) {
                    qint16 versionBcd = static_cast<qint16>(buf[1] | (buf[2] << 8));
                    int major = versionBcd / 100;
                    int minor = versionBcd % 100;
                    info.firmwareVersion = QString("%1.%2").arg(major).arg(minor, 2, 10, QChar('0'));
                }
            }

            hid_close(dev);
        }
    }

    hid_free_enumeration(devs);
    hid_exit();

    return info;
}

// ============== Hotplug Monitoring ==============
//
// Note: We use periodic hid_enumerate() instead of IOKit's IOHIDManager callbacks
// because hidapi internally uses IOHIDManager on macOS. Having two IOHIDManagers
// for the same device causes crashes due to resource conflicts.
//
// The periodic check is very lightweight - hid_enumerate() only reads USB
// descriptors from the OS kernel, no device I/O.

void KpodDevice::setupHotplugMonitoring() {
    // Create a timer for periodic device presence checking
    m_presenceTimer = new QTimer(this);
    m_presenceTimer->setInterval(PRESENCE_CHECK_INTERVAL_MS);
    connect(m_presenceTimer, &QTimer::timeout, this, &KpodDevice::checkDevicePresence);
    m_presenceTimer->start();
}

void KpodDevice::teardownHotplugMonitoring() {
    if (m_presenceTimer) {
        m_presenceTimer->stop();
        delete m_presenceTimer;
        m_presenceTimer = nullptr;
    }
}

void KpodDevice::checkDevicePresence() {
    // Quick check using hid_enumerate - very lightweight, no device I/O
    if (hid_init() != 0) {
        return;
    }

    struct hid_device_info *devs = hid_enumerate(VENDOR_ID, PRODUCT_ID);
    bool nowDetected = (devs != nullptr);
    hid_free_enumeration(devs);
    hid_exit();

    bool wasDetected = m_deviceInfo.detected;

    if (!wasDetected && nowDetected) {
        // Device just arrived
        onDeviceArrived();
    } else if (wasDetected && !nowDetected) {
        // Device just departed
        onDeviceRemoved();
    }
}

void KpodDevice::onDeviceArrived() {
    // Refresh device info
    m_deviceInfo = detectDevice();

    // Emit signal for UI update
    emit deviceConnected();
}

void KpodDevice::onDeviceRemoved() {
    // Stop polling if active
    if (m_pollTimer->isActive()) {
        m_pollTimer->stop();
    }

    // Close device handle if open
    if (m_hidDevice) {
        hid_close(m_hidDevice);
        m_hidDevice = nullptr;
        hid_exit();
    }

    // Update device info
    m_deviceInfo.detected = false;

    // Emit signal for UI update
    emit deviceDisconnected();
}
