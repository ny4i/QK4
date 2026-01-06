#include "optionsdialog.h"
#include "micmeterwidget.h"
#include "../models/radiostate.h"
#include "../hardware/kpoddevice.h"
#include "../settings/radiosettings.h"
#include "../network/kpa1500client.h"
#include "../audio/audioengine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QStringList>
#include <QCheckBox>
#include <QGridLayout>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>

// K4 Color scheme
namespace {
const QString Background = "#1a1a1a";
const QString DarkBackground = "#0d0d0d";
const QString TextWhite = "#FFFFFF";
const QString TextGray = "#999999";
const QString TextAmber = "#FFB000";
const QString BorderColor = "#333333";
} // namespace

OptionsDialog::OptionsDialog(RadioState *radioState, KPA1500Client *kpa1500Client, AudioEngine *audioEngine,
                             KpodDevice *kpodDevice, QWidget *parent)
    : QDialog(parent), m_radioState(radioState), m_kpa1500Client(kpa1500Client), m_audioEngine(audioEngine),
      m_kpodDevice(kpodDevice), m_kpa1500StatusLabel(nullptr), m_micDeviceCombo(nullptr), m_micGainSlider(nullptr),
      m_micGainValueLabel(nullptr), m_micTestBtn(nullptr), m_micMeter(nullptr), m_speakerDeviceCombo(nullptr) {
    setupUi();

    // Connect to KPA1500 client signals for status updates
    if (m_kpa1500Client) {
        connect(m_kpa1500Client, &KPA1500Client::connected, this, &OptionsDialog::onKpa1500ConnectionStateChanged);
        connect(m_kpa1500Client, &KPA1500Client::disconnected, this, &OptionsDialog::onKpa1500ConnectionStateChanged);
        connect(m_kpa1500Client, &KPA1500Client::stateChanged, this, &OptionsDialog::onKpa1500ConnectionStateChanged);
    }

    // Connect to AudioEngine for mic level updates
    if (m_audioEngine) {
        connect(m_audioEngine, &AudioEngine::micLevelChanged, this, &OptionsDialog::onMicLevelChanged);
    }

    // Connect to KPOD device signals for real-time status updates
    if (m_kpodDevice) {
        connect(m_kpodDevice, &KpodDevice::deviceConnected, this, &OptionsDialog::updateKpodStatus);
        connect(m_kpodDevice, &KpodDevice::deviceDisconnected, this, &OptionsDialog::updateKpodStatus);
    }
}

OptionsDialog::~OptionsDialog() {
    // Make sure to stop mic test if dialog is closed
    if (m_micTestActive && m_audioEngine) {
        m_audioEngine->setMicEnabled(false);
    }
}

void OptionsDialog::setupUi() {
    setWindowTitle("Options");
    setMinimumSize(700, 550);
    resize(800, 650);

    // Dark theme styling
    setStyleSheet(QString("QDialog { background-color: %1; }"
                          "QLabel { color: %2; }"
                          "QListWidget { background-color: %3; color: %2; border: 1px solid %4; "
                          "             font-size: 13px; outline: none; }"
                          "QListWidget::item { padding: 10px 15px; border-bottom: 1px solid %4; }"
                          "QListWidget::item:selected { background-color: %5; color: %3; }"
                          "QListWidget::item:hover { background-color: #2a2a2a; }")
                      .arg(Background, TextWhite, DarkBackground, BorderColor, TextAmber));

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Left side: vertical tab list
    m_tabList = new QListWidget(this);
    m_tabList->setFixedWidth(150);
    m_tabList->addItem("About");
    m_tabList->addItem("Audio Input");
    m_tabList->addItem("Audio Output");
    m_tabList->addItem("Network");
    m_tabList->addItem("K-Pod");
    m_tabList->addItem("KPA1500");
    m_tabList->setCurrentRow(0);

    // Right side: stacked pages
    m_pageStack = new QStackedWidget(this);
    m_pageStack->addWidget(createAboutPage());
    m_pageStack->addWidget(createAudioInputPage());
    m_pageStack->addWidget(createAudioOutputPage());
    m_pageStack->addWidget(createNetworkPage());
    m_pageStack->addWidget(createKpodPage());
    m_pageStack->addWidget(createKpa1500Page());

    // Connect tab selection to page switching
    connect(m_tabList, &QListWidget::currentRowChanged, m_pageStack, &QStackedWidget::setCurrentIndex);

    mainLayout->addWidget(m_tabList);
    mainLayout->addWidget(m_pageStack, 1);
}

namespace {
QStringList decodeOptionModules(const QString &om) {
    QStringList options;
    if (om.length() > 0 && om[0] == 'A')
        options << "KAT4 (ATU)";
    if (om.length() > 1 && om[1] == 'P')
        options << "KPA4 (PA)";
    if (om.length() > 2 && om[2] == 'X')
        options << "XVTR";
    if (om.length() > 3 && om[3] == 'S')
        options << "KRX4 (Sub RX)";
    if (om.length() > 4 && om[4] == 'H')
        options << "KHDR4 (HDR)";
    if (om.length() > 5 && om[5] == 'M')
        options << "K40 (Mini)";
    if (om.length() > 6 && om[6] == 'L')
        options << "Linear Amp";
    if (om.length() > 7 && om[7] == '1')
        options << "KPA1500";
    return options;
}
} // namespace

QWidget *OptionsDialog::createAboutPage() {
    auto *page = new QWidget(this);
    page->setStyleSheet(QString("background-color: %1;").arg(Background));

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // Title
    auto *titleLabel = new QLabel("Connected Radio", page);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(TextAmber));
    layout->addWidget(titleLabel);

    // Separator line
    auto *line = new QFrame(page);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QString("background-color: %1;").arg(BorderColor));
    line->setFixedHeight(1);
    layout->addWidget(line);

    // Two-column layout for Radio Info and Installed Options
    auto *infoRow = new QHBoxLayout();
    infoRow->setSpacing(20);

    // Left column widget: Radio ID and Model
    auto *leftWidget = new QWidget(page);
    auto *leftColumn = new QVBoxLayout(leftWidget);
    leftColumn->setContentsMargins(0, 0, 0, 0);
    leftColumn->setSpacing(8);

    // Radio ID
    auto *idLayout = new QHBoxLayout();
    auto *idTitleLabel = new QLabel("Radio ID:", leftWidget);
    idTitleLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    idTitleLabel->setFixedWidth(80);

    QString radioId = m_radioState ? m_radioState->radioID() : "Not connected";
    auto *idValueLabel = new QLabel(radioId, leftWidget);
    idValueLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold;").arg(TextWhite));

    idLayout->addWidget(idTitleLabel);
    idLayout->addWidget(idValueLabel);
    idLayout->addStretch();
    leftColumn->addLayout(idLayout);

    // Radio Model
    auto *modelLayout = new QHBoxLayout();
    auto *modelTitleLabel = new QLabel("Model:", leftWidget);
    modelTitleLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    modelTitleLabel->setFixedWidth(80);

    QString radioModel = m_radioState ? m_radioState->radioModel() : "Unknown";
    auto *modelValueLabel = new QLabel(radioModel, leftWidget);
    modelValueLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold;").arg(TextWhite));

    modelLayout->addWidget(modelTitleLabel);
    modelLayout->addWidget(modelValueLabel);
    modelLayout->addStretch();
    leftColumn->addLayout(modelLayout);
    leftColumn->addStretch();

    // Vertical demarcation line
    auto *vline = new QFrame(page);
    vline->setFrameShape(QFrame::VLine);
    vline->setFrameShadow(QFrame::Plain);
    vline->setStyleSheet(QString("background-color: %1;").arg(BorderColor));
    vline->setFixedWidth(1);

    // Right column widget: Installed Options
    auto *rightWidget = new QWidget(page);
    auto *rightColumn = new QVBoxLayout(rightWidget);
    rightColumn->setContentsMargins(0, 0, 0, 0);
    rightColumn->setSpacing(6);

    auto *optionsTitle = new QLabel("Installed Options", rightWidget);
    optionsTitle->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold;").arg(TextAmber));
    rightColumn->addWidget(optionsTitle);

    if (m_radioState && !m_radioState->optionModules().isEmpty()) {
        QStringList options = decodeOptionModules(m_radioState->optionModules());
        if (options.isEmpty()) {
            auto *noOptionsLabel = new QLabel("No additional options", rightWidget);
            noOptionsLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-style: italic;").arg(TextGray));
            rightColumn->addWidget(noOptionsLabel);
        } else {
            for (const QString &opt : options) {
                auto *optLabel = new QLabel(QString::fromUtf8("\u2022 ") + opt, rightWidget);
                optLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(TextWhite));
                rightColumn->addWidget(optLabel);
            }
        }
    } else {
        auto *noDataLabel = new QLabel("Not connected", rightWidget);
        noDataLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-style: italic;").arg(TextGray));
        rightColumn->addWidget(noDataLabel);
    }
    rightColumn->addStretch();

    // Assemble the info row
    infoRow->addWidget(leftWidget, 1);
    infoRow->addWidget(vline);
    infoRow->addWidget(rightWidget, 1);

    layout->addLayout(infoRow);

    // Software Versions section
    layout->addSpacing(10);

    auto *versionsTitle = new QLabel("Software Versions", page);
    versionsTitle->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;").arg(TextAmber));
    layout->addWidget(versionsTitle);

    auto *line2 = new QFrame(page);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet(QString("background-color: %1;").arg(BorderColor));
    line2->setFixedHeight(1);
    layout->addWidget(line2);

    // Firmware versions list
    if (m_radioState) {
        QMap<QString, QString> versions = m_radioState->firmwareVersions();

        // Component name mappings for readable display
        QMap<QString, QString> componentNames = {
            {"DDC0", "DDC 0"},   {"DDC1", "DDC 1"},    {"DUC", "DUC"},   {"FP", "Front Panel"}, {"DSP", "DSP"},
            {"RFB", "RF Board"}, {"REF", "Reference"}, {"DAP", "DAP"},   {"KSRV", "K Server"},  {"KUI", "K UI"},
            {"KUP", "K Update"}, {"KCFG", "K Config"}, {"R", "Revision"}};

        for (auto it = versions.constBegin(); it != versions.constEnd(); ++it) {
            auto *versionLayout = new QHBoxLayout();

            QString displayName = componentNames.value(it.key(), it.key());
            auto *nameLabel = new QLabel(displayName + ":", page);
            nameLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(TextGray));
            nameLabel->setFixedWidth(120);

            auto *valueLabel = new QLabel(it.value(), page);
            valueLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(TextWhite));

            versionLayout->addWidget(nameLabel);
            versionLayout->addWidget(valueLabel);
            versionLayout->addStretch();
            layout->addLayout(versionLayout);
        }
    } else {
        auto *noDataLabel = new QLabel("Connect to a radio to view version information", page);
        noDataLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-style: italic;").arg(TextGray));
        layout->addWidget(noDataLabel);
    }

    layout->addStretch();
    return page;
}

QWidget *OptionsDialog::createKpodPage() {
    auto *page = new QWidget(this);
    page->setStyleSheet(QString("background-color: %1;").arg(Background));

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // Status indicator
    auto *statusLayout = new QHBoxLayout();
    auto *statusLabel = new QLabel("Status:", page);
    statusLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    statusLabel->setFixedWidth(80);

    m_kpodStatusLabel = new QLabel("Not Detected", page);
    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(m_kpodStatusLabel);
    statusLayout->addStretch();
    layout->addLayout(statusLayout);

    // Separator line
    auto *line = new QFrame(page);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QString("background-color: %1;").arg(BorderColor));
    line->setFixedHeight(1);
    layout->addWidget(line);

    // Device Summary title
    auto *titleLabel = new QLabel("Device Summary", page);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(TextAmber));
    layout->addWidget(titleLabel);

    // Device info table using grid layout
    auto *tableWidget = new QWidget(page);
    auto *tableLayout = new QGridLayout(tableWidget);
    tableLayout->setContentsMargins(0, 10, 0, 10);
    tableLayout->setHorizontalSpacing(20);
    tableLayout->setVerticalSpacing(8);

    // Table styling
    QString headerStyle = QString("color: %1; font-size: 12px; font-weight: bold; padding: 5px;").arg(TextGray);

    // Create labels with property names
    QStringList properties = {"Product Name", "Manufacturer",     "Vendor ID", "Product ID",
                              "Device Type",  "Firmware Version", "Device ID"};
    QVector<QLabel **> valueLabels = {&m_kpodProductLabel,   &m_kpodManufacturerLabel, &m_kpodVendorIdLabel,
                                      &m_kpodProductIdLabel, &m_kpodDeviceTypeLabel,   &m_kpodFirmwareLabel,
                                      &m_kpodDeviceIdLabel};

    for (int row = 0; row < properties.size(); ++row) {
        auto *propLabel = new QLabel(properties[row], tableWidget);
        propLabel->setStyleSheet(headerStyle);

        *valueLabels[row] = new QLabel("N/A", tableWidget);

        tableLayout->addWidget(propLabel, row, 0, Qt::AlignLeft);
        tableLayout->addWidget(*valueLabels[row], row, 1, Qt::AlignLeft);
    }

    tableLayout->setColumnStretch(1, 1);
    layout->addWidget(tableWidget);

    // Another separator
    auto *line2 = new QFrame(page);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet(QString("background-color: %1;").arg(BorderColor));
    line2->setFixedHeight(1);
    layout->addWidget(line2);

    // Enable checkbox
    m_kpodEnableCheckbox = new QCheckBox("Enable K-Pod", page);
    m_kpodEnableCheckbox->setChecked(RadioSettings::instance()->kpodEnabled());

    connect(m_kpodEnableCheckbox, &QCheckBox::toggled, this,
            [](bool checked) { RadioSettings::instance()->setKpodEnabled(checked); });

    layout->addWidget(m_kpodEnableCheckbox);

    // Help text
    m_kpodHelpLabel = new QLabel("Connect a K-Pod device to enable this feature.", page);
    m_kpodHelpLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-style: italic;").arg(TextGray));
    m_kpodHelpLabel->setWordWrap(true);
    layout->addWidget(m_kpodHelpLabel);

    layout->addStretch();

    // Initialize with current status
    updateKpodStatus();

    return page;
}

void OptionsDialog::updateKpodStatus() {
    if (!m_kpodDevice)
        return;

    KpodDeviceInfo info = m_kpodDevice->deviceInfo();
    bool detected = info.detected;

    // Styling
    QString valueStyle = QString("color: %1; font-size: 12px; padding: 5px;").arg(TextWhite);
    QString notDetectedStyle = QString("color: %1; font-size: 12px; font-style: italic; padding: 5px;").arg(TextGray);

    // Update status label
    QString statusText = detected ? "Detected" : "Not Detected";
    QString statusColor = detected ? "#00FF00" : "#FF6666";
    m_kpodStatusLabel->setText(statusText);
    m_kpodStatusLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold;").arg(statusColor));

    // Update device info labels
    auto setLabel = [&](QLabel *label, const QString &value) {
        QString displayValue = value.isEmpty() ? "N/A" : value;
        label->setText(displayValue);
        label->setStyleSheet(displayValue == "N/A" ? notDetectedStyle : valueStyle);
    };

    setLabel(m_kpodProductLabel, detected ? info.productName : "");
    setLabel(m_kpodManufacturerLabel, detected ? info.manufacturer : "");
    setLabel(m_kpodVendorIdLabel,
             detected ? QString("%1 (0x%2)").arg(info.vendorId).arg(info.vendorId, 4, 16, QChar('0')).toUpper() : "");
    setLabel(m_kpodProductIdLabel,
             detected ? QString("%1 (0x%2)").arg(info.productId).arg(info.productId, 4, 16, QChar('0')).toUpper() : "");
    setLabel(m_kpodDeviceTypeLabel, detected ? "USB HID (Human Interface Device)" : "");
    setLabel(m_kpodFirmwareLabel, detected ? info.firmwareVersion : "");
    setLabel(m_kpodDeviceIdLabel, detected ? info.deviceId : "");

    // Update checkbox enabled state and styling
    m_kpodEnableCheckbox->setEnabled(detected);
    if (detected) {
        m_kpodEnableCheckbox->setStyleSheet(
            QString("QCheckBox { color: %1; font-size: 13px; spacing: 8px; }"
                    "QCheckBox::indicator { width: 18px; height: 18px; }"
                    "QCheckBox::indicator:unchecked { border: 2px solid %2; background: %3; border-radius: 3px; }"
                    "QCheckBox::indicator:checked { border: 2px solid %4; background: %4; border-radius: 3px; }"
                    "QCheckBox::indicator:checked::after { content: ''; }")
                .arg(TextWhite, TextGray, DarkBackground, TextAmber));
    } else {
        m_kpodEnableCheckbox->setStyleSheet(
            QString("QCheckBox { color: %1; font-size: 13px; spacing: 8px; }"
                    "QCheckBox::indicator { width: 18px; height: 18px; }"
                    "QCheckBox::indicator:unchecked { border: 2px solid %1; background: %2; border-radius: 3px; }")
                .arg(TextGray, DarkBackground));
    }

    // Update help text
    m_kpodHelpLabel->setText(detected ? "When enabled, the K-Pod VFO knob and buttons will control the radio."
                                      : "Connect a K-Pod device to enable this feature.");
}

QWidget *OptionsDialog::createKpa1500Page() {
    auto *page = new QWidget(this);
    page->setStyleSheet(QString("background-color: %1;").arg(Background));

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // Title
    auto *titleLabel = new QLabel("KPA1500 Amplifier", page);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(TextAmber));
    layout->addWidget(titleLabel);

    // Status indicator
    auto *statusLayout = new QHBoxLayout();
    auto *statusTitleLabel = new QLabel("Status:", page);
    statusTitleLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    statusTitleLabel->setFixedWidth(80);

    m_kpa1500StatusLabel = new QLabel(page);
    updateKpa1500Status(); // Set initial status

    statusLayout->addWidget(statusTitleLabel);
    statusLayout->addWidget(m_kpa1500StatusLabel);
    statusLayout->addStretch();
    layout->addLayout(statusLayout);

    // Separator line
    auto *line = new QFrame(page);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QString("background-color: %1;").arg(BorderColor));
    line->setFixedHeight(1);
    layout->addWidget(line);

    // Connection Settings section
    auto *sectionLabel = new QLabel("Connection Settings", page);
    sectionLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;").arg(TextWhite));
    layout->addWidget(sectionLabel);

    // IP Address input
    auto *hostLayout = new QHBoxLayout();
    auto *hostLabel = new QLabel("IP Address:", page);
    hostLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    hostLabel->setFixedWidth(80);

    m_kpa1500HostEdit = new QLineEdit(page);
    m_kpa1500HostEdit->setPlaceholderText("e.g., 192.168.1.100");
    m_kpa1500HostEdit->setStyleSheet(QString("QLineEdit { background-color: %1; color: %2; border: 1px solid %3; "
                                             "           padding: 6px; font-size: 13px; border-radius: 3px; }"
                                             "QLineEdit:focus { border-color: %4; }")
                                         .arg(DarkBackground, TextWhite, BorderColor, TextAmber));
    m_kpa1500HostEdit->setText(RadioSettings::instance()->kpa1500Host());

    hostLayout->addWidget(hostLabel);
    hostLayout->addWidget(m_kpa1500HostEdit, 1);
    layout->addLayout(hostLayout);

    // Port input
    auto *portLayout = new QHBoxLayout();
    auto *portLabel = new QLabel("Port:", page);
    portLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    portLabel->setFixedWidth(80);

    m_kpa1500PortEdit = new QLineEdit(page);
    m_kpa1500PortEdit->setPlaceholderText("1500");
    m_kpa1500PortEdit->setFixedWidth(100);
    m_kpa1500PortEdit->setStyleSheet(QString("QLineEdit { background-color: %1; color: %2; border: 1px solid %3; "
                                             "           padding: 6px; font-size: 13px; border-radius: 3px; }"
                                             "QLineEdit:focus { border-color: %4; }")
                                         .arg(DarkBackground, TextWhite, BorderColor, TextAmber));
    m_kpa1500PortEdit->setText(QString::number(RadioSettings::instance()->kpa1500Port()));

    auto *portHint = new QLabel("(default: 1500)", page);
    portHint->setStyleSheet(QString("color: %1; font-size: 11px;").arg(TextGray));

    portLayout->addWidget(portLabel);
    portLayout->addWidget(m_kpa1500PortEdit);
    portLayout->addWidget(portHint);
    portLayout->addStretch();
    layout->addLayout(portLayout);

    // Polling interval input
    auto *pollLayout = new QHBoxLayout();
    auto *pollLabel = new QLabel("Poll Interval:", page);
    pollLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    pollLabel->setFixedWidth(80);

    m_kpa1500PollIntervalEdit = new QLineEdit(page);
    m_kpa1500PollIntervalEdit->setPlaceholderText("800");
    m_kpa1500PollIntervalEdit->setFixedWidth(80);
    m_kpa1500PollIntervalEdit->setStyleSheet(
        QString("QLineEdit { background-color: %1; color: %2; border: 1px solid %3; "
                "           padding: 6px; font-size: 13px; border-radius: 3px; }"
                "QLineEdit:focus { border-color: %4; }")
            .arg(DarkBackground, TextWhite, BorderColor, TextAmber));
    m_kpa1500PollIntervalEdit->setText(QString::number(RadioSettings::instance()->kpa1500PollInterval()));

    auto *pollUnitLabel = new QLabel("ms", page);
    pollUnitLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextWhite));

    auto *pollHint = new QLabel("(100-5000, default: 800)", page);
    pollHint->setStyleSheet(QString("color: %1; font-size: 11px;").arg(TextGray));

    pollLayout->addWidget(pollLabel);
    pollLayout->addWidget(m_kpa1500PollIntervalEdit);
    pollLayout->addWidget(pollUnitLabel);
    pollLayout->addSpacing(10);
    pollLayout->addWidget(pollHint);
    pollLayout->addStretch();
    layout->addLayout(pollLayout);

    // Separator line
    auto *line2 = new QFrame(page);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet(QString("background-color: %1;").arg(BorderColor));
    line2->setFixedHeight(1);
    layout->addWidget(line2);

    // Enable checkbox
    m_kpa1500EnableCheckbox = new QCheckBox("Enable KPA1500", page);
    m_kpa1500EnableCheckbox->setStyleSheet(
        QString("QCheckBox { color: %1; font-size: 13px; spacing: 8px; }"
                "QCheckBox::indicator { width: 18px; height: 18px; }"
                "QCheckBox::indicator:unchecked { border: 2px solid %2; background: %3; border-radius: 3px; }"
                "QCheckBox::indicator:checked { border: 2px solid %4; background: %4; border-radius: 3px; }")
            .arg(TextWhite, TextGray, DarkBackground, TextAmber));
    m_kpa1500EnableCheckbox->setChecked(RadioSettings::instance()->kpa1500Enabled());
    layout->addWidget(m_kpa1500EnableCheckbox);

    // Help text
    auto *kpa1500HelpLabel =
        new QLabel("When enabled, K4Controller will connect to the KPA1500 amplifier for status monitoring.", page);
    kpa1500HelpLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-style: italic;").arg(TextGray));
    kpa1500HelpLabel->setWordWrap(true);
    layout->addWidget(kpa1500HelpLabel);

    // Connect signals to save settings
    connect(m_kpa1500HostEdit, &QLineEdit::editingFinished, this,
            [this]() { RadioSettings::instance()->setKpa1500Host(m_kpa1500HostEdit->text()); });

    connect(m_kpa1500PortEdit, &QLineEdit::editingFinished, this, [this]() {
        bool ok;
        quint16 port = m_kpa1500PortEdit->text().toUShort(&ok);
        if (ok && port > 0) {
            RadioSettings::instance()->setKpa1500Port(port);
        }
    });

    connect(m_kpa1500PollIntervalEdit, &QLineEdit::editingFinished, this, [this]() {
        bool ok;
        int interval = m_kpa1500PollIntervalEdit->text().toInt(&ok);
        if (ok && interval >= 100 && interval <= 5000) {
            RadioSettings::instance()->setKpa1500PollInterval(interval);
        } else {
            // Reset to current value if invalid
            m_kpa1500PollIntervalEdit->setText(QString::number(RadioSettings::instance()->kpa1500PollInterval()));
        }
    });

    connect(m_kpa1500EnableCheckbox, &QCheckBox::toggled, this,
            [](bool checked) { RadioSettings::instance()->setKpa1500Enabled(checked); });

    layout->addStretch();
    return page;
}

void OptionsDialog::updateKpa1500Status() {
    if (!m_kpa1500StatusLabel) {
        return;
    }

    bool isConnected = m_kpa1500Client && m_kpa1500Client->isConnected();

    if (isConnected) {
        m_kpa1500StatusLabel->setText("Connected");
        m_kpa1500StatusLabel->setStyleSheet(QString("color: #00FF00; font-size: 13px; font-weight: bold;"));
    } else {
        m_kpa1500StatusLabel->setText("Not Connected");
        m_kpa1500StatusLabel->setStyleSheet(QString("color: #FF6666; font-size: 13px; font-weight: bold;"));
    }
}

void OptionsDialog::onKpa1500ConnectionStateChanged() {
    updateKpa1500Status();
}

QWidget *OptionsDialog::createAudioInputPage() {
    auto *page = new QWidget(this);
    page->setStyleSheet(QString("background-color: %1;").arg(Background));

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // Title
    auto *titleLabel = new QLabel("Audio Input", page);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(TextAmber));
    layout->addWidget(titleLabel);

    // Separator line
    auto *line = new QFrame(page);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QString("background-color: %1;").arg(BorderColor));
    line->setFixedHeight(1);
    layout->addWidget(line);

    // === Microphone Device Selection ===
    auto *deviceLabel = new QLabel("Microphone:", page);
    deviceLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    layout->addWidget(deviceLabel);

    m_micDeviceCombo = new QComboBox(page);
    m_micDeviceCombo->setStyleSheet(
        QString("QComboBox { background-color: %1; color: %2; border: 1px solid %3; "
                "           padding: 6px; font-size: 13px; border-radius: 3px; }"
                "QComboBox:focus { border-color: %4; }"
                "QComboBox::drop-down { border: none; width: 20px; }"
                "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; "
                "           border-right: 5px solid transparent; border-top: 5px solid %2; }"
                "QComboBox QAbstractItemView { background-color: %1; color: %2; selection-background-color: %4; }")
            .arg(DarkBackground, TextWhite, BorderColor, TextAmber));
    populateMicDevices();
    connect(m_micDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OptionsDialog::onMicDeviceChanged);
    layout->addWidget(m_micDeviceCombo);

    layout->addSpacing(10);

    // === Microphone Gain ===
    auto *gainLayout = new QHBoxLayout();
    auto *gainLabel = new QLabel("Mic Gain:", page);
    gainLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    gainLabel->setFixedWidth(80);
    gainLayout->addWidget(gainLabel);

    m_micGainSlider = new QSlider(Qt::Horizontal, page);
    m_micGainSlider->setRange(0, 100);
    m_micGainSlider->setValue(RadioSettings::instance()->micGain());
    m_micGainSlider->setStyleSheet(
        QString("QSlider::groove:horizontal { background: %1; height: 6px; border-radius: 3px; }"
                "QSlider::handle:horizontal { background: %2; width: 16px; margin: -5px 0; border-radius: 8px; }"
                "QSlider::sub-page:horizontal { background: %2; border-radius: 3px; }")
            .arg(BorderColor, TextAmber));
    connect(m_micGainSlider, &QSlider::valueChanged, this, &OptionsDialog::onMicGainChanged);
    gainLayout->addWidget(m_micGainSlider, 1);

    m_micGainValueLabel = new QLabel(QString("%1%").arg(m_micGainSlider->value()), page);
    m_micGainValueLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextWhite));
    m_micGainValueLabel->setFixedWidth(40);
    m_micGainValueLabel->setAlignment(Qt::AlignRight);
    gainLayout->addWidget(m_micGainValueLabel);

    layout->addLayout(gainLayout);

    auto *gainHelpLabel = new QLabel("Adjust the microphone input level. 50% is unity gain.", page);
    gainHelpLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-style: italic;").arg(TextGray));
    layout->addWidget(gainHelpLabel);

    layout->addSpacing(15);

    // === Microphone Test Section ===
    auto *line2 = new QFrame(page);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet(QString("background-color: %1;").arg(BorderColor));
    line2->setFixedHeight(1);
    layout->addWidget(line2);

    auto *testSectionLabel = new QLabel("Microphone Test", page);
    testSectionLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;").arg(TextWhite));
    layout->addWidget(testSectionLabel);

    auto *testHelpLabel =
        new QLabel("Click the Test button to activate the microphone and check the input level.", page);
    testHelpLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(TextGray));
    testHelpLabel->setWordWrap(true);
    layout->addWidget(testHelpLabel);

    layout->addSpacing(5);

    // Mic Level Meter
    auto *meterLayout = new QHBoxLayout();
    auto *meterLabel = new QLabel("Level:", page);
    meterLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    meterLabel->setFixedWidth(50);
    meterLayout->addWidget(meterLabel);

    m_micMeter = new MicMeterWidget(page);
    meterLayout->addWidget(m_micMeter, 1);

    layout->addLayout(meterLayout);

    layout->addSpacing(10);

    // Test Button
    m_micTestBtn = new QPushButton("Test Microphone", page);
    m_micTestBtn->setCheckable(true);
    m_micTestBtn->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; border: 1px solid %3; "
                                        "             padding: 10px 20px; font-size: 13px; border-radius: 4px; }"
                                        "QPushButton:hover { background-color: #2a2a2a; }"
                                        "QPushButton:checked { background-color: %4; color: %1; border-color: %4; }")
                                    .arg(DarkBackground, TextWhite, BorderColor, TextAmber));
    connect(m_micTestBtn, &QPushButton::toggled, this, &OptionsDialog::onMicTestToggled);
    layout->addWidget(m_micTestBtn);

    layout->addStretch();
    return page;
}

void OptionsDialog::populateMicDevices() {
    if (!m_micDeviceCombo)
        return;

    m_micDeviceCombo->clear();

    auto devices = AudioEngine::availableInputDevices();
    QString savedDevice = RadioSettings::instance()->micDevice();
    int selectedIndex = 0;

    for (int i = 0; i < devices.size(); i++) {
        const auto &device = devices[i];
        m_micDeviceCombo->addItem(device.second, device.first);

        // Find the saved device
        if (device.first == savedDevice) {
            selectedIndex = i;
        }
    }

    m_micDeviceCombo->setCurrentIndex(selectedIndex);
}

void OptionsDialog::onMicDeviceChanged(int index) {
    if (!m_micDeviceCombo || index < 0)
        return;

    QString deviceId = m_micDeviceCombo->currentData().toString();
    RadioSettings::instance()->setMicDevice(deviceId);

    if (m_audioEngine) {
        m_audioEngine->setMicDevice(deviceId);
    }
}

void OptionsDialog::onMicGainChanged(int value) {
    if (m_micGainValueLabel) {
        m_micGainValueLabel->setText(QString("%1%").arg(value));
    }

    RadioSettings::instance()->setMicGain(value);

    if (m_audioEngine) {
        m_audioEngine->setMicGain(value / 100.0f);
    }
}

void OptionsDialog::onMicTestToggled(bool checked) {
    m_micTestActive = checked;

    if (m_micTestBtn) {
        m_micTestBtn->setText(checked ? "Stop Test" : "Test Microphone");
    }

    if (m_audioEngine) {
        m_audioEngine->setMicEnabled(checked);
    }

    // Reset meter when stopping
    if (!checked && m_micMeter) {
        m_micMeter->setLevel(0.0f);
    }
}

void OptionsDialog::onMicLevelChanged(float level) {
    if (m_micTestActive && m_micMeter) {
        // Scale level for better visualization (RMS tends to be low)
        float scaledLevel = qBound(0.0f, level * 5.0f, 1.0f);
        m_micMeter->setLevel(scaledLevel);
    }
}

QWidget *OptionsDialog::createAudioOutputPage() {
    auto *page = new QWidget(this);
    page->setStyleSheet(QString("background-color: %1;").arg(Background));

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // Title
    auto *titleLabel = new QLabel("Audio Output", page);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(TextAmber));
    layout->addWidget(titleLabel);

    // Separator line
    auto *line = new QFrame(page);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QString("background-color: %1;").arg(BorderColor));
    line->setFixedHeight(1);
    layout->addWidget(line);

    // === Speaker Device Selection ===
    auto *deviceLabel = new QLabel("Speaker:", page);
    deviceLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    layout->addWidget(deviceLabel);

    m_speakerDeviceCombo = new QComboBox(page);
    m_speakerDeviceCombo->setStyleSheet(
        QString("QComboBox { background-color: %1; color: %2; border: 1px solid %3; "
                "           padding: 6px; font-size: 13px; border-radius: 3px; }"
                "QComboBox:focus { border-color: %4; }"
                "QComboBox::drop-down { border: none; width: 20px; }"
                "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; "
                "           border-right: 5px solid transparent; border-top: 5px solid %2; }"
                "QComboBox QAbstractItemView { background-color: %1; color: %2; selection-background-color: %4; }")
            .arg(DarkBackground, TextWhite, BorderColor, TextAmber));
    populateSpeakerDevices();
    connect(m_speakerDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OptionsDialog::onSpeakerDeviceChanged);
    layout->addWidget(m_speakerDeviceCombo);

    layout->addSpacing(10);

    // Help text
    auto *helpLabel = new QLabel("Select the audio output device for radio receive audio. "
                                 "Volume is controlled by the MAIN and SUB sliders on the side panel.",
                                 page);
    helpLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-style: italic;").arg(TextGray));
    helpLabel->setWordWrap(true);
    layout->addWidget(helpLabel);

    layout->addStretch();
    return page;
}

void OptionsDialog::populateSpeakerDevices() {
    if (!m_speakerDeviceCombo)
        return;

    m_speakerDeviceCombo->clear();

    auto devices = AudioEngine::availableOutputDevices();
    QString savedDevice = RadioSettings::instance()->speakerDevice();
    int selectedIndex = 0;

    for (int i = 0; i < devices.size(); i++) {
        const auto &device = devices[i];
        m_speakerDeviceCombo->addItem(device.second, device.first);

        // Find the saved device
        if (device.first == savedDevice) {
            selectedIndex = i;
        }
    }

    m_speakerDeviceCombo->setCurrentIndex(selectedIndex);
}

void OptionsDialog::onSpeakerDeviceChanged(int index) {
    if (!m_speakerDeviceCombo || index < 0)
        return;

    QString deviceId = m_speakerDeviceCombo->currentData().toString();
    RadioSettings::instance()->setSpeakerDevice(deviceId);

    if (m_audioEngine) {
        m_audioEngine->setOutputDevice(deviceId);
    }
}

QWidget *OptionsDialog::createNetworkPage() {
    auto *page = new QWidget(this);
    page->setStyleSheet(QString("background-color: %1;").arg(Background));

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // Title
    auto *titleLabel = new QLabel("Network", page);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(TextAmber));
    layout->addWidget(titleLabel);

    // Separator line
    auto *line = new QFrame(page);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QString("background-color: %1;").arg(BorderColor));
    line->setFixedHeight(1);
    layout->addWidget(line);

    // Placeholder message
    auto *placeholderLabel = new QLabel("Network and connection settings will be configured here.", page);
    placeholderLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(TextGray));
    placeholderLabel->setWordWrap(true);
    layout->addWidget(placeholderLabel);

    layout->addStretch();
    return page;
}
