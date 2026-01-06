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

    m_pollTimer->start();
    emit deviceConnected();
    qDebug() << "KPOD: Polling started at" << POLL_INTERVAL_MS << "ms interval";
    return true;
}

void KpodDevice::stopPolling() {
    if (m_pollTimer->isActive()) {
        m_pollTimer->stop();
        qDebug() << "KPOD: Polling stopped";
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

    qDebug() << "KPOD: Device opened successfully";
    return true;
}

void KpodDevice::closeDevice() {
    if (m_hidDevice) {
        hid_close(m_hidDevice);
        m_hidDevice = nullptr;
        hid_exit();
        emit deviceDisconnected();
        qDebug() << "KPOD: Device closed";
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

    // Read response (non-blocking with timeout)
    unsigned char buffer[8];
    int readResult = hid_read_timeout(m_hidDevice, buffer, sizeof(buffer), 50);

    if (readResult < 0) {
        // Read error - device may be disconnected
        qWarning() << "KPOD: Read failed, device disconnected?";
        stopPolling();
        emit pollError("Failed to read from KPOD");
        return;
    }

    // Process response if we got a full packet with new event
    if (readResult == 8 && buffer[0] == 'u') {
        processResponse(buffer);
    }
    // readResult == 0 means no data available (normal for non-blocking)
}

void KpodDevice::processResponse(const unsigned char *buffer) {
    // Extract encoder ticks (signed int16, little-endian)
    qint16 encoderTicks = static_cast<qint16>(buffer[1] | (buffer[2] << 8));

    // Emit encoder signal only if there was rotation
    if (encoderTicks != 0) {
        emit encoderRotated(encoderTicks);
    }

    // Extract controls byte
    quint8 controlsByte = buffer[3];

    // Extract rocker position (bits 5-6)
    RockerPosition rocker = static_cast<RockerPosition>((controlsByte >> 5) & 0x03);

    // Emit rocker change only on transitions
    if (rocker != m_lastRockerPosition && rocker != 3) { // 3 would be error state
        m_lastRockerPosition = rocker;
        emit rockerPositionChanged(rocker);
        qDebug() << "KPOD: Rocker position changed to" << rocker;
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

    qDebug() << "KPOD: Hotplug monitoring started (periodic check every" << PRESENCE_CHECK_INTERVAL_MS << "ms)";
}

void KpodDevice::teardownHotplugMonitoring() {
    if (m_presenceTimer) {
        m_presenceTimer->stop();
        delete m_presenceTimer;
        m_presenceTimer = nullptr;
        qDebug() << "KPOD: Hotplug monitoring stopped";
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
    qDebug() << "KPOD: Device connected (hotplug)";

    // Refresh device info
    m_deviceInfo = detectDevice();

    // Emit signal for UI update
    emit deviceConnected();
}

void KpodDevice::onDeviceRemoved() {
    qDebug() << "KPOD: Device disconnected (hotplug)";

    // Stop polling if active
    if (m_pollTimer->isActive()) {
        m_pollTimer->stop();
        qDebug() << "KPOD: Polling stopped due to device removal";
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
