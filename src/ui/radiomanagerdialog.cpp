#include "radiomanagerdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>

RadioManagerDialog::RadioManagerDialog(QWidget *parent) : QDialog(parent), m_currentIndex(-1) {
    setupUi();
    refreshList();
    updateButtonStates();

    connect(RadioSettings::instance(), &RadioSettings::radiosChanged, this, &RadioManagerDialog::refreshList);
}

void RadioManagerDialog::setupUi() {
    setWindowTitle("Server Manager");
    setFixedSize(580, 360);

    // Dark theme for the dialog
    setStyleSheet("QDialog { background-color: #2b2b2b; }");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // Top horizontal section - servers list on left, edit fields on right
    auto *topLayout = new QHBoxLayout();
    topLayout->setSpacing(20);

    // === LEFT SIDE: Available Servers ===
    auto *leftSection = new QVBoxLayout();
    leftSection->setSpacing(8);

    auto *serversTitle = new QLabel("Available Servers", this);
    serversTitle->setStyleSheet("QLabel { color: #FFD700; font-weight: bold; font-size: 14px; }");
    leftSection->addWidget(serversTitle);

    m_radioList = new QListWidget(this);
    m_radioList->setMinimumWidth(180);
    m_radioList->setMaximumWidth(200);
    m_radioList->setStyleSheet("QListWidget { "
                               "  background-color: #3c3c3c; "
                               "  color: #ffffff; "
                               "  border: 1px solid #555555; "
                               "  border-radius: 4px; "
                               "  padding: 4px; "
                               "} "
                               "QListWidget::item { "
                               "  padding: 6px; "
                               "} "
                               "QListWidget::item:selected { "
                               "  background-color: #0078d4; "
                               "  color: #ffffff; "
                               "} "
                               "QListWidget::item:hover { "
                               "  background-color: #4a4a4a; "
                               "}");
    leftSection->addWidget(m_radioList);
    topLayout->addLayout(leftSection);

    // === RIGHT SIDE: Edit Connect ===
    auto *rightSection = new QVBoxLayout();
    rightSection->setSpacing(8);

    auto *editTitle = new QLabel("Edit Connect", this);
    editTitle->setStyleSheet("QLabel { color: #FFD700; font-weight: bold; font-size: 14px; }");
    rightSection->addWidget(editTitle);

    // Form fields - label on LEFT of text box
    auto *formLayout = new QGridLayout();
    formLayout->setHorizontalSpacing(10);
    formLayout->setVerticalSpacing(10);

    QString lineEditStyle = "QLineEdit { "
                            "  background-color: #3c3c3c; "
                            "  color: #ffffff; "
                            "  border: 1px solid #555555; "
                            "  border-radius: 4px; "
                            "  padding: 6px; "
                            "  min-width: 150px; "
                            "}";

    QString labelStyle = "QLabel { color: #cccccc; font-size: 12px; }";

    // Row 0: Name
    auto *nameLabel = new QLabel("Name", this);
    nameLabel->setStyleSheet(labelStyle);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setStyleSheet(lineEditStyle);
    m_nameEdit->setPlaceholderText("Server Name");
    formLayout->addWidget(nameLabel, 0, 0);
    formLayout->addWidget(m_nameEdit, 0, 1);

    // Row 1: Host or IP
    auto *hostLabel = new QLabel("Host or IP", this);
    hostLabel->setStyleSheet(labelStyle);
    m_hostEdit = new QLineEdit(this);
    m_hostEdit->setStyleSheet(lineEditStyle);
    m_hostEdit->setPlaceholderText("192.168.1.100");
    formLayout->addWidget(hostLabel, 1, 0);
    formLayout->addWidget(m_hostEdit, 1, 1);

    // Row 2: Port
    auto *portLabel = new QLabel("Port", this);
    portLabel->setStyleSheet(labelStyle);
    m_portEdit = new QLineEdit(this);
    m_portEdit->setStyleSheet(lineEditStyle);
    m_portEdit->setPlaceholderText("64242");
    m_portEdit->setMaximumWidth(80);
    formLayout->addWidget(portLabel, 2, 0);
    formLayout->addWidget(m_portEdit, 2, 1);

    // Row 3: Password
    auto *passwordLabel = new QLabel("Password", this);
    passwordLabel->setStyleSheet(labelStyle);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setStyleSheet(lineEditStyle);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Password");
    formLayout->addWidget(passwordLabel, 3, 0);
    formLayout->addWidget(m_passwordEdit, 3, 1);

    rightSection->addLayout(formLayout);
    rightSection->addStretch();
    topLayout->addLayout(rightSection);
    topLayout->addStretch();

    mainLayout->addLayout(topLayout);

    // === BOTTOM: Button Row ===
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(16); // More spacing between buttons

    QString buttonStyle = "QPushButton { "
                          "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                          "    stop:0 #4a4a4a, stop:0.4 #3a3a3a, "
                          "    stop:0.6 #353535, stop:1 #2a2a2a); "
                          "  color: #FFFFFF; "
                          "  border: 1px solid #606060; "
                          "  border-radius: 5px; "
                          "  padding: 10px 20px; "
                          "  font-size: 12px; "
                          "  font-weight: bold; "
                          "  min-width: 70px; "
                          "} "
                          "QPushButton:hover { "
                          "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                          "    stop:0 #5a5a5a, stop:0.4 #4a4a4a, "
                          "    stop:0.6 #454545, stop:1 #3a3a3a); "
                          "  border: 1px solid #808080; "
                          "} "
                          "QPushButton:pressed { "
                          "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                          "    stop:0 #2a2a2a, stop:0.4 #353535, "
                          "    stop:0.6 #3a3a3a, stop:1 #4a4a4a); "
                          "  border: 1px solid #909090; "
                          "} "
                          "QPushButton:disabled { "
                          "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                          "    stop:0 #3a3a3a, stop:1 #2a2a2a); "
                          "  color: #666666; "
                          "  border: 1px solid #444444; "
                          "}";

    m_connectButton = new QPushButton("Connect", this);
    m_connectButton->setStyleSheet(buttonStyle);
    buttonLayout->addWidget(m_connectButton);

    m_newButton = new QPushButton("New", this);
    m_newButton->setStyleSheet(buttonStyle);
    buttonLayout->addWidget(m_newButton);

    m_saveButton = new QPushButton("Save", this);
    m_saveButton->setStyleSheet(buttonStyle);
    buttonLayout->addWidget(m_saveButton);

    m_deleteButton = new QPushButton("Delete", this);
    m_deleteButton->setStyleSheet(buttonStyle);
    buttonLayout->addWidget(m_deleteButton);

    // Back button - smaller with curved arrow
    QString backButtonStyle = "QPushButton { "
                              "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                              "    stop:0 #4a4a4a, stop:0.4 #3a3a3a, "
                              "    stop:0.6 #353535, stop:1 #2a2a2a); "
                              "  color: #FFFFFF; "
                              "  border: 1px solid #606060; "
                              "  border-radius: 4px; "
                              "  padding: 4px; "
                              "  font-size: 14px; "
                              "} "
                              "QPushButton:hover { "
                              "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                              "    stop:0 #5a5a5a, stop:0.4 #4a4a4a, "
                              "    stop:0.6 #454545, stop:1 #3a3a3a); "
                              "  border: 1px solid #808080; "
                              "} "
                              "QPushButton:pressed { "
                              "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                              "    stop:0 #2a2a2a, stop:0.4 #353535, "
                              "    stop:0.6 #3a3a3a, stop:1 #4a4a4a); "
                              "  border: 1px solid #909090; "
                              "}";

    m_backButton = new QPushButton(QString::fromUtf8("\xE2\x86\xA9"), this); // ↩ Curved arrow
    m_backButton->setStyleSheet(backButtonStyle);
    m_backButton->setFixedSize(36, 36);
    m_backButton->setToolTip("Back / Exit");
    buttonLayout->addWidget(m_backButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_connectButton, &QPushButton::clicked, this, &RadioManagerDialog::onConnectClicked);
    connect(m_newButton, &QPushButton::clicked, this, &RadioManagerDialog::onNewClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &RadioManagerDialog::onSaveClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &RadioManagerDialog::onDeleteClicked);
    connect(m_backButton, &QPushButton::clicked, this, &RadioManagerDialog::onBackClicked);
    connect(m_radioList, &QListWidget::itemSelectionChanged, this, &RadioManagerDialog::onSelectionChanged);
    connect(m_radioList, &QListWidget::itemDoubleClicked, this, &RadioManagerDialog::onItemDoubleClicked);

    // Update button states when host field changes
    connect(m_hostEdit, &QLineEdit::textChanged, this, [this]() { updateButtonStates(); });
}

void RadioManagerDialog::refreshList() {
    m_radioList->clear();

    const auto radios = RadioSettings::instance()->radios();
    for (const auto &radio : radios) {
        m_radioList->addItem(radio.name.isEmpty() ? radio.host : radio.name);
    }

    int lastIndex = RadioSettings::instance()->lastSelectedIndex();
    if (lastIndex >= 0 && lastIndex < m_radioList->count()) {
        m_radioList->setCurrentRow(lastIndex);
        m_currentIndex = lastIndex;
        populateFieldsFromSelection();
    }

    updateButtonStates();
}

void RadioManagerDialog::onConnectClicked() {
    QString host = m_hostEdit->text().trimmed();
    if (!host.isEmpty()) {
        RadioEntry entry;
        entry.name = m_nameEdit->text().trimmed();
        entry.host = host;
        entry.password = m_passwordEdit->text();
        QString portText = m_portEdit->text().trimmed();
        entry.port = portText.isEmpty() ? 64242 : portText.toUShort();

        if (m_currentIndex >= 0) {
            RadioSettings::instance()->setLastSelectedIndex(m_currentIndex);
        }
        emit connectRequested(entry);
        accept();
    }
}

void RadioManagerDialog::onNewClicked() {
    m_currentIndex = -1;
    clearFields();
    m_radioList->clearSelection();
    m_nameEdit->setFocus();
    updateButtonStates();
}

void RadioManagerDialog::onSaveClicked() {
    QString name = m_nameEdit->text().trimmed();
    QString host = m_hostEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    QString portText = m_portEdit->text().trimmed();

    if (name.isEmpty()) {
        name = host; // Use host as name if no name provided
    }

    if (host.isEmpty()) {
        return; // Can't save without host
    }

    RadioEntry entry;
    entry.name = name;
    entry.host = host;
    entry.password = password;
    entry.port = portText.isEmpty() ? 64242 : portText.toUShort();

    if (m_currentIndex >= 0 && m_currentIndex < RadioSettings::instance()->radios().size()) {
        // Update existing
        RadioSettings::instance()->updateRadio(m_currentIndex, entry);
    } else {
        // Add new
        RadioSettings::instance()->addRadio(entry);
        m_currentIndex = RadioSettings::instance()->radios().size() - 1;
    }

    // Re-select the saved item
    if (m_currentIndex >= 0 && m_currentIndex < m_radioList->count()) {
        m_radioList->setCurrentRow(m_currentIndex);
    }
    updateButtonStates();
}

void RadioManagerDialog::onDeleteClicked() {
    if (m_currentIndex >= 0 && m_currentIndex < RadioSettings::instance()->radios().size()) {
        RadioSettings::instance()->removeRadio(m_currentIndex);
        clearFields();
        m_currentIndex = -1;
    }
    updateButtonStates();
}

void RadioManagerDialog::onBackClicked() {
    reject();
}

void RadioManagerDialog::onSelectionChanged() {
    int row = m_radioList->currentRow();
    if (row >= 0) {
        m_currentIndex = row;
        populateFieldsFromSelection();
    }
    updateButtonStates();
}

void RadioManagerDialog::onItemDoubleClicked(QListWidgetItem *item) {
    Q_UNUSED(item)
    onConnectClicked();
}

void RadioManagerDialog::updateButtonStates() {
    bool hasSelection = m_currentIndex >= 0 && m_currentIndex < RadioSettings::instance()->radios().size();
    bool hasHost = !m_hostEdit->text().trimmed().isEmpty();

    m_connectButton->setEnabled(hasHost);
    m_deleteButton->setEnabled(hasSelection);
    m_saveButton->setEnabled(hasHost);
}

void RadioManagerDialog::clearFields() {
    m_nameEdit->clear();
    m_hostEdit->clear();
    m_portEdit->clear();
    m_passwordEdit->clear();
}

void RadioManagerDialog::populateFieldsFromSelection() {
    if (m_currentIndex >= 0 && m_currentIndex < RadioSettings::instance()->radios().size()) {
        const RadioEntry &radio = RadioSettings::instance()->radios().at(m_currentIndex);
        m_nameEdit->setText(radio.name);
        m_hostEdit->setText(radio.host);
        m_portEdit->setText(QString::number(radio.port));
        m_passwordEdit->setText(radio.password);
    }
}

RadioEntry RadioManagerDialog::selectedRadio() const {
    if (m_currentIndex >= 0) {
        return RadioSettings::instance()->radios().at(m_currentIndex);
    }
    return RadioEntry();
}

bool RadioManagerDialog::hasSelection() const {
    return m_currentIndex >= 0;
}
