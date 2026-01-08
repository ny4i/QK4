#include "radiosettings.h"

RadioSettings *RadioSettings::instance() {
    static RadioSettings instance;
    return &instance;
}

RadioSettings::RadioSettings(QObject *parent)
    : QObject(parent), m_lastSelectedIndex(-1), m_kpodEnabled(false), m_settings("K4Controller", "K4Controller") {
    load();
}

QVector<RadioEntry> RadioSettings::radios() const {
    return m_radios;
}

void RadioSettings::addRadio(const RadioEntry &radio) {
    m_radios.append(radio);
    save();
    emit radiosChanged();
}

void RadioSettings::removeRadio(int index) {
    if (index >= 0 && index < m_radios.size()) {
        m_radios.removeAt(index);
        if (m_lastSelectedIndex >= m_radios.size()) {
            m_lastSelectedIndex = m_radios.isEmpty() ? -1 : m_radios.size() - 1;
        }
        save();
        emit radiosChanged();
    }
}

void RadioSettings::updateRadio(int index, const RadioEntry &radio) {
    if (index >= 0 && index < m_radios.size()) {
        m_radios[index] = radio;
        save();
        emit radiosChanged();
    }
}

int RadioSettings::lastSelectedIndex() const {
    return m_lastSelectedIndex;
}

void RadioSettings::setLastSelectedIndex(int index) {
    if (m_lastSelectedIndex != index) {
        m_lastSelectedIndex = index;
        save();
    }
}

bool RadioSettings::kpodEnabled() const {
    return m_kpodEnabled;
}

void RadioSettings::setKpodEnabled(bool enabled) {
    if (m_kpodEnabled != enabled) {
        m_kpodEnabled = enabled;
        save();
        emit kpodEnabledChanged(enabled);
    }
}

QString RadioSettings::kpa1500Host() const {
    return m_kpa1500Host;
}

void RadioSettings::setKpa1500Host(const QString &host) {
    if (m_kpa1500Host != host) {
        m_kpa1500Host = host;
        save();
        emit kpa1500SettingsChanged();
    }
}

quint16 RadioSettings::kpa1500Port() const {
    return m_kpa1500Port;
}

void RadioSettings::setKpa1500Port(quint16 port) {
    if (m_kpa1500Port != port) {
        m_kpa1500Port = port;
        save();
        emit kpa1500SettingsChanged();
    }
}

bool RadioSettings::kpa1500Enabled() const {
    return m_kpa1500Enabled;
}

void RadioSettings::setKpa1500Enabled(bool enabled) {
    if (m_kpa1500Enabled != enabled) {
        m_kpa1500Enabled = enabled;
        save();
        emit kpa1500EnabledChanged(enabled);
    }
}

int RadioSettings::kpa1500PollInterval() const {
    return m_kpa1500PollInterval;
}

void RadioSettings::setKpa1500PollInterval(int intervalMs) {
    // Clamp to reasonable range: 100ms - 5000ms
    intervalMs = qBound(100, intervalMs, 5000);
    if (m_kpa1500PollInterval != intervalMs) {
        m_kpa1500PollInterval = intervalMs;
        save();
        emit kpa1500PollIntervalChanged(intervalMs);
    }
}

int RadioSettings::volume() const {
    return m_settings.value("audio/volume", 45).toInt();
}

void RadioSettings::setVolume(int value) {
    value = qBound(0, value, 100);
    m_settings.setValue("audio/volume", value);
    m_settings.sync();
}

int RadioSettings::subVolume() const {
    return m_settings.value("audio/subVolume", 45).toInt();
}

void RadioSettings::setSubVolume(int value) {
    value = qBound(0, value, 100);
    m_settings.setValue("audio/subVolume", value);
    m_settings.sync();
}

int RadioSettings::micGain() const {
    return m_settings.value("audio/micGain", 50).toInt();
}

void RadioSettings::setMicGain(int value) {
    value = qBound(0, value, 100);
    int oldValue = m_settings.value("audio/micGain", 50).toInt();
    if (oldValue != value) {
        m_settings.setValue("audio/micGain", value);
        m_settings.sync();
        emit micGainChanged(value);
    }
}

QString RadioSettings::micDevice() const {
    return m_settings.value("audio/micDevice", "").toString();
}

void RadioSettings::setMicDevice(const QString &deviceId) {
    QString oldDevice = m_settings.value("audio/micDevice", "").toString();
    if (oldDevice != deviceId) {
        m_settings.setValue("audio/micDevice", deviceId);
        m_settings.sync();
        emit micDeviceChanged(deviceId);
    }
}

QString RadioSettings::speakerDevice() const {
    return m_settings.value("audio/speakerDevice", "").toString();
}

void RadioSettings::setSpeakerDevice(const QString &deviceId) {
    QString oldDevice = m_settings.value("audio/speakerDevice", "").toString();
    if (oldDevice != deviceId) {
        m_settings.setValue("audio/speakerDevice", deviceId);
        m_settings.sync();
        emit speakerDeviceChanged(deviceId);
    }
}

bool RadioSettings::rigctldEnabled() const {
    return m_rigctldEnabled;
}

void RadioSettings::setRigctldEnabled(bool enabled) {
    if (m_rigctldEnabled != enabled) {
        m_rigctldEnabled = enabled;
        save();
        emit rigctldEnabledChanged(enabled);
    }
}

quint16 RadioSettings::rigctldPort() const {
    return m_rigctldPort;
}

void RadioSettings::setRigctldPort(quint16 port) {
    // Clamp to valid port range (1024-65535)
    port = qBound(quint16(1024), port, quint16(65535));
    if (m_rigctldPort != port) {
        m_rigctldPort = port;
        save();
        emit rigctldPortChanged(port);
    }
}

QMap<QString, MacroEntry> RadioSettings::macros() const {
    return m_macros;
}

MacroEntry RadioSettings::macro(const QString &functionId) const {
    return m_macros.value(functionId);
}

void RadioSettings::setMacro(const QString &functionId, const QString &label, const QString &command) {
    MacroEntry entry;
    entry.functionId = functionId;
    entry.label = label;
    entry.command = command;

    if (command.isEmpty()) {
        // Remove empty macros
        if (m_macros.contains(functionId)) {
            m_macros.remove(functionId);
            save();
            emit macrosChanged();
        }
    } else {
        // Add or update macro
        bool changed = !m_macros.contains(functionId) || m_macros[functionId].label != label ||
                       m_macros[functionId].command != command;
        if (changed) {
            m_macros[functionId] = entry;
            save();
            emit macrosChanged();
        }
    }
}

void RadioSettings::clearMacro(const QString &functionId) {
    if (m_macros.contains(functionId)) {
        m_macros.remove(functionId);
        save();
        emit macrosChanged();
    }
}

void RadioSettings::load() {
    int count = m_settings.beginReadArray("radios");
    m_radios.clear();
    for (int i = 0; i < count; ++i) {
        m_settings.setArrayIndex(i);
        RadioEntry entry;
        entry.name = m_settings.value("name").toString();
        entry.host = m_settings.value("host").toString();
        entry.password = m_settings.value("password").toString();
        entry.port = m_settings.value("port").toUInt();
        entry.useTls = m_settings.value("useTls", false).toBool();
        entry.identity = m_settings.value("identity").toString();
        m_radios.append(entry);
    }
    m_settings.endArray();

    m_lastSelectedIndex = m_settings.value("lastSelectedIndex", -1).toInt();
    m_kpodEnabled = m_settings.value("kpodEnabled", false).toBool();

    // KPA1500 settings
    m_kpa1500Host = m_settings.value("kpa1500/host", "").toString();
    m_kpa1500Port = m_settings.value("kpa1500/port", 1500).toUInt();
    m_kpa1500Enabled = m_settings.value("kpa1500/enabled", false).toBool();
    m_kpa1500PollInterval = m_settings.value("kpa1500/pollInterval", 800).toInt();

    // Rigctld settings
    m_rigctldEnabled = m_settings.value("rigctld/enabled", false).toBool();
    m_rigctldPort = m_settings.value("rigctld/port", 4532).toUInt();

    // Macro settings
    int macroCount = m_settings.beginReadArray("macros");
    m_macros.clear();
    for (int i = 0; i < macroCount; ++i) {
        m_settings.setArrayIndex(i);
        MacroEntry entry;
        entry.functionId = m_settings.value("functionId").toString();
        entry.label = m_settings.value("label").toString();
        entry.command = m_settings.value("command").toString();
        if (!entry.functionId.isEmpty()) {
            m_macros[entry.functionId] = entry;
        }
    }
    m_settings.endArray();
}

void RadioSettings::save() {
    m_settings.beginWriteArray("radios");
    for (int i = 0; i < m_radios.size(); ++i) {
        m_settings.setArrayIndex(i);
        m_settings.setValue("name", m_radios[i].name);
        m_settings.setValue("host", m_radios[i].host);
        m_settings.setValue("password", m_radios[i].password);
        m_settings.setValue("port", m_radios[i].port);
        m_settings.setValue("useTls", m_radios[i].useTls);
        m_settings.setValue("identity", m_radios[i].identity);
    }
    m_settings.endArray();

    m_settings.setValue("lastSelectedIndex", m_lastSelectedIndex);
    m_settings.setValue("kpodEnabled", m_kpodEnabled);

    // KPA1500 settings
    m_settings.setValue("kpa1500/host", m_kpa1500Host);
    m_settings.setValue("kpa1500/port", m_kpa1500Port);
    m_settings.setValue("kpa1500/enabled", m_kpa1500Enabled);
    m_settings.setValue("kpa1500/pollInterval", m_kpa1500PollInterval);

    // Rigctld settings
    m_settings.setValue("rigctld/enabled", m_rigctldEnabled);
    m_settings.setValue("rigctld/port", m_rigctldPort);

    // Macro settings
    m_settings.beginWriteArray("macros");
    int i = 0;
    for (auto it = m_macros.constBegin(); it != m_macros.constEnd(); ++it, ++i) {
        m_settings.setArrayIndex(i);
        m_settings.setValue("functionId", it->functionId);
        m_settings.setValue("label", it->label);
        m_settings.setValue("command", it->command);
    }
    m_settings.endArray();

    m_settings.sync();
}
