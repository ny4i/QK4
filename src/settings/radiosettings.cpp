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
}

void RadioSettings::save() {
    m_settings.beginWriteArray("radios");
    for (int i = 0; i < m_radios.size(); ++i) {
        m_settings.setArrayIndex(i);
        m_settings.setValue("name", m_radios[i].name);
        m_settings.setValue("host", m_radios[i].host);
        m_settings.setValue("password", m_radios[i].password);
        m_settings.setValue("port", m_radios[i].port);
    }
    m_settings.endArray();

    m_settings.setValue("lastSelectedIndex", m_lastSelectedIndex);
    m_settings.setValue("kpodEnabled", m_kpodEnabled);

    // KPA1500 settings
    m_settings.setValue("kpa1500/host", m_kpa1500Host);
    m_settings.setValue("kpa1500/port", m_kpa1500Port);
    m_settings.setValue("kpa1500/enabled", m_kpa1500Enabled);
    m_settings.setValue("kpa1500/pollInterval", m_kpa1500PollInterval);

    m_settings.sync();
}
