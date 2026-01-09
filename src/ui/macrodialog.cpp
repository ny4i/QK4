#include "macrodialog.h"
#include "../settings/radiosettings.h"
#include "fnpopupwidget.h"
#include <QDebug>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>

namespace {
// Layout constants
const int RowHeight = 44;
// Columns use equal stretch factors (33.3% each) - no fixed widths

// Colors
const QColor BackgroundColor(24, 24, 28);  // #18181C
const QColor HeaderColor(34, 34, 40);      // #222228
const QColor RowColor(25, 25, 30);         // Unselected row
const QColor SelectedRowColor(80, 80, 85); // Grey highlight for selected row
const QColor BorderColor(40, 40, 45);      // Row border
const QColor AmberColor(255, 176, 0);      // #FFB000
} // namespace

// ============== MacroItemWidget ==============

MacroItemWidget::MacroItemWidget(const QString &functionId, const QString &displayName, QWidget *parent)
    : QWidget(parent), m_functionId(functionId), m_displayName(displayName) {
    setFixedHeight(RowHeight);
    setCursor(Qt::PointingHandCursor);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 5, 15, 5);
    layout->setSpacing(0); // No spacing - columns use stretch factors

    // Column 1: Function ID (read-only) - 33.3% width
    m_functionLabel = new QLabel(displayName, this);
    m_functionLabel->setStyleSheet("color: #AAAAAA; font-size: 13px;");
    layout->addWidget(m_functionLabel, 1); // stretch factor 1

    // Column 2: Label (editable) - 33.3% width, centered
    m_labelDisplay = new QLabel("Unused", this);
    m_labelDisplay->setAlignment(Qt::AlignCenter);
    m_labelDisplay->setStyleSheet("color: #888888; font-size: 13px;");
    layout->addWidget(m_labelDisplay, 1); // stretch factor 1

    m_labelEdit = new QLineEdit(this);
    m_labelEdit->setMaxLength(12);
    m_labelEdit->setAlignment(Qt::AlignCenter);
    m_labelEdit->setStyleSheet("QLineEdit { background: #333; color: white; border: 1px solid #666; "
                               "border-radius: 3px; padding: 2px 5px; font-size: 13px; }");
    m_labelEdit->hide();
    connect(m_labelEdit, &QLineEdit::editingFinished, this, &MacroItemWidget::finishEditing);
    layout->addWidget(m_labelEdit, 1); // stretch factor 1

    // Column 3: CAT Command (editable) - 33.3% width
    m_commandDisplay = new QLabel("", this);
    m_commandDisplay->setStyleSheet(
        "color: #888888; font-size: 13px; font-family: 'JetBrains Mono', 'Menlo', 'Consolas', monospace;");
    m_commandDisplay->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(m_commandDisplay, 1); // stretch factor 1

    m_commandEdit = new QLineEdit(this);
    m_commandEdit->setMaxLength(64);
    m_commandEdit->setStyleSheet("QLineEdit { background: #333; color: white; border: 1px solid #666; "
                                 "border-radius: 3px; padding: 2px 5px; font-size: 13px; font-family: 'JetBrains "
                                 "Mono', 'Menlo', 'Consolas', monospace; }");
    m_commandEdit->hide();
    connect(m_commandEdit, &QLineEdit::editingFinished, this, &MacroItemWidget::finishEditing);
    layout->addWidget(m_commandEdit, 1); // stretch factor 1

    setLayout(layout);
}

QString MacroItemWidget::label() const {
    return m_label;
}

QString MacroItemWidget::command() const {
    return m_command;
}

void MacroItemWidget::setLabel(const QString &label) {
    m_label = label;
    updateDisplay();
}

void MacroItemWidget::setCommand(const QString &command) {
    m_command = command;
    updateDisplay();
}

void MacroItemWidget::updateDisplay() {
    // Show label or status
    if (m_command.isEmpty()) {
        m_labelDisplay->setText("Unused");
        m_labelDisplay->setStyleSheet("color: #666666; font-size: 13px; font-style: italic;");
    } else if (m_label.isEmpty()) {
        m_labelDisplay->setText("Mapped");
        m_labelDisplay->setStyleSheet("color: " + AmberColor.name() + "; font-size: 13px;");
    } else {
        m_labelDisplay->setText(m_label);
        m_labelDisplay->setStyleSheet("color: " + AmberColor.name() + "; font-size: 13px; font-weight: bold;");
    }

    // Show command
    m_commandDisplay->setText(m_command.isEmpty() ? "" : m_command);
}

void MacroItemWidget::setSelected(bool selected) {
    m_selected = selected;

    // Update all label colors based on selection (inverted when selected)
    if (selected) {
        // White text on grey background
        m_functionLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: bold;");
        m_labelDisplay->setStyleSheet("color: #FFFFFF; font-size: 13px;");
        m_commandDisplay->setStyleSheet(
            "color: #FFFFFF; font-size: 13px; font-family: 'JetBrains Mono', 'Menlo', 'Consolas', monospace;");
    } else {
        // Grey text on dark background
        m_functionLabel->setStyleSheet("color: #AAAAAA; font-size: 13px;");
        updateDisplay(); // Reset label/command colors based on content
    }

    update();
}

void MacroItemWidget::editLabel() {
    m_labelEdit->setText(m_label);
    m_labelDisplay->hide();
    m_labelEdit->show();
    m_labelEdit->setFocus();
    m_labelEdit->selectAll();
}

void MacroItemWidget::editCommand() {
    m_commandEdit->setText(m_command);
    m_commandDisplay->hide();
    m_commandEdit->show();
    m_commandEdit->setFocus();
    m_commandEdit->selectAll();
}

void MacroItemWidget::finishEditing() {
    if (m_labelEdit->isVisible()) {
        QString newLabel = m_labelEdit->text().trimmed();
        m_labelEdit->hide();
        m_labelDisplay->show();
        if (newLabel != m_label) {
            m_label = newLabel;
            updateDisplay();
            emit labelChanged(m_functionId, m_label);
        }
    }

    if (m_commandEdit->isVisible()) {
        QString newCommand = m_commandEdit->text().trimmed();
        m_commandEdit->hide();
        m_commandDisplay->show();
        if (newCommand != m_command) {
            m_command = newCommand;
            updateDisplay();
            emit commandChanged(m_functionId, m_command);
        }
    }
}

void MacroItemWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_selected) {
        // Selected: full row grey highlight
        painter.fillRect(rect(), SelectedRowColor);
    } else {
        // Unselected: dark background
        painter.fillRect(rect(), RowColor);
    }

    // Bottom border
    painter.setPen(BorderColor);
    painter.drawLine(0, height() - 1, width(), height() - 1);
}

void MacroItemWidget::mousePressEvent(QMouseEvent *event) {
    // Detect which column was clicked based on percentage (33.3% each)
    int x = event->pos().x();
    int columnWidth = width() / 3;

    if (x >= 2 * columnWidth) {
        emit commandClicked(); // Right third = command
    } else {
        emit clicked(); // Left two thirds = function/label
    }
}

// ============== MacroDialog ==============

MacroDialog::MacroDialog(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFocusPolicy(Qt::StrongFocus);

    setupUi();
}

void MacroDialog::setupUi() {
    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Content area (left side - macro list)
    m_contentWidget = new QWidget(this);
    m_contentWidget->setStyleSheet("background-color: #18181C;");

    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // Header
    m_headerLabel = new QLabel("MACROS", m_contentWidget);
    m_headerLabel->setStyleSheet("background-color: #222228; color: #666; font-size: 12px; "
                                 "font-weight: bold; padding: 8px 15px;");
    contentLayout->addWidget(m_headerLabel);

    // Column headers
    QWidget *columnHeader = new QWidget(m_contentWidget);
    columnHeader->setStyleSheet("background-color: #1E1E24;");
    columnHeader->setFixedHeight(28);

    QHBoxLayout *headerLayout = new QHBoxLayout(columnHeader);
    headerLayout->setContentsMargins(15, 5, 15, 5);
    headerLayout->setSpacing(0); // No spacing - columns use stretch factors

    QLabel *funcHeader = new QLabel("Function", columnHeader);
    funcHeader->setStyleSheet("color: #888; font-size: 11px; font-weight: bold;");
    headerLayout->addWidget(funcHeader, 1); // stretch factor 1

    QLabel *labelHeader = new QLabel("Label", columnHeader);
    labelHeader->setAlignment(Qt::AlignCenter);
    labelHeader->setStyleSheet("color: #888; font-size: 11px; font-weight: bold;");
    headerLayout->addWidget(labelHeader, 1); // stretch factor 1

    QLabel *cmdHeader = new QLabel("CAT Command", columnHeader);
    cmdHeader->setStyleSheet("color: #888; font-size: 11px; font-weight: bold;");
    headerLayout->addWidget(cmdHeader, 1); // stretch factor 1

    contentLayout->addWidget(columnHeader);

    // Scroll area for macro items
    m_scrollArea = new QScrollArea(m_contentWidget);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }"
                                "QScrollBar:vertical { background: #18181C; width: 8px; }"
                                "QScrollBar::handle:vertical { background: #444; border-radius: 4px; }"
                                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

    m_listContainer = new QWidget();
    m_listContainer->setStyleSheet("background: transparent;");
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(0);
    m_listLayout->addStretch();

    m_scrollArea->setWidget(m_listContainer);
    contentLayout->addWidget(m_scrollArea);

    // Install event filter for wheel events
    m_scrollArea->installEventFilter(this);
    m_scrollArea->viewport()->installEventFilter(this);

    mainLayout->addWidget(m_contentWidget, 1);

    // Navigation panel (right side)
    QWidget *navPanel = new QWidget(this);
    navPanel->setFixedWidth(130);
    navPanel->setStyleSheet("background-color: #222228;");

    QVBoxLayout *navOuterLayout = new QVBoxLayout(navPanel);
    navOuterLayout->setContentsMargins(8, 12, 8, 12);
    navOuterLayout->setSpacing(8);

    QString buttonStyle = "QPushButton { background-color: #3A3A45; color: white; border: none; "
                          "border-radius: 6px; font-size: 16px; font-weight: bold; }"
                          "QPushButton:pressed { background-color: #505060; }";

    // Row 1: Up and Down buttons
    QHBoxLayout *row1 = new QHBoxLayout();
    row1->setSpacing(8);

    m_upBtn = new QPushButton("\xE2\x96\xB2", navPanel); // Up arrow
    m_upBtn->setFixedSize(54, 44);
    m_upBtn->setStyleSheet(buttonStyle);
    connect(m_upBtn, &QPushButton::clicked, this, &MacroDialog::navigateUp);
    row1->addWidget(m_upBtn);

    m_downBtn = new QPushButton("\xE2\x96\xBC", navPanel); // Down arrow
    m_downBtn->setFixedSize(54, 44);
    m_downBtn->setStyleSheet(buttonStyle);
    connect(m_downBtn, &QPushButton::clicked, this, &MacroDialog::navigateDown);
    row1->addWidget(m_downBtn);

    navOuterLayout->addLayout(row1);
    navOuterLayout->addStretch();

    // Row 2: EDIT and Back buttons
    QHBoxLayout *row2 = new QHBoxLayout();
    row2->setSpacing(8);

    m_editBtn = new QPushButton("EDIT", navPanel);
    m_editBtn->setFixedSize(54, 44);
    m_editBtn->setStyleSheet("QPushButton { background-color: #3A3A45; color: #888; border: none; "
                             "border-radius: 6px; font-size: 10px; font-weight: bold; }"
                             "QPushButton:pressed { background-color: #505060; }");
    connect(m_editBtn, &QPushButton::clicked, this, &MacroDialog::selectCurrent);
    row2->addWidget(m_editBtn);

    m_backBtn = new QPushButton("\xE2\x86\xA9", navPanel); // Back arrow
    m_backBtn->setFixedSize(54, 44);
    m_backBtn->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                             "stop:0 #4a4a4a, stop:0.4 #3a3a3a, stop:0.6 #353535, stop:1 #2a2a2a);"
                             "color: white; border: 1px solid #606060; border-radius: 6px; "
                             "font-size: 16px; font-weight: bold; }"
                             "QPushButton:pressed { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                             "stop:0 #2a2a2a, stop:0.4 #353535, stop:0.6 #3a3a3a, stop:1 #4a4a4a); }");
    connect(m_backBtn, &QPushButton::clicked, this, &MacroDialog::closeDialog);
    row2->addWidget(m_backBtn);

    navOuterLayout->addLayout(row2);

    mainLayout->addWidget(navPanel);

    setLayout(mainLayout);
}

void MacroDialog::populateItems() {
    // Clear existing
    for (auto *widget : m_itemWidgets) {
        m_listLayout->removeWidget(widget);
        delete widget;
    }
    m_itemWidgets.clear();

    // Define all macro slots with display names
    struct MacroSlot {
        QString id;
        QString displayName;
    };

    QVector<MacroSlot> macroSlots = {
        // PF buttons
        {MacroIds::PF1, "PF1"},
        {MacroIds::PF2, "PF2"},
        {MacroIds::PF3, "PF3"},
        {MacroIds::PF4, "PF4"},
        // Fn.F1-F8
        {MacroIds::FnF1, "Fn.F1"},
        {MacroIds::FnF2, "Fn.F2"},
        {MacroIds::FnF3, "Fn.F3"},
        {MacroIds::FnF4, "Fn.F4"},
        {MacroIds::FnF5, "Fn.F5"},
        {MacroIds::FnF6, "Fn.F6"},
        {MacroIds::FnF7, "Fn.F7"},
        {MacroIds::FnF8, "Fn.F8"},
        // REM ANT
        {MacroIds::RemAnt, "REM ANT"},
        // KPOD buttons (tap/hold pairs)
        {MacroIds::Kpod1T, "K-pod.1T"},
        {MacroIds::Kpod1H, "K-pod.1H"},
        {MacroIds::Kpod2T, "K-pod.2T"},
        {MacroIds::Kpod2H, "K-pod.2H"},
        {MacroIds::Kpod3T, "K-pod.3T"},
        {MacroIds::Kpod3H, "K-pod.3H"},
        {MacroIds::Kpod4T, "K-pod.4T"},
        {MacroIds::Kpod4H, "K-pod.4H"},
        {MacroIds::Kpod5T, "K-pod.5T"},
        {MacroIds::Kpod5H, "K-pod.5H"},
        {MacroIds::Kpod6T, "K-pod.6T"},
        {MacroIds::Kpod6H, "K-pod.6H"},
        {MacroIds::Kpod7T, "K-pod.7T"},
        {MacroIds::Kpod7H, "K-pod.7H"},
        {MacroIds::Kpod8T, "K-pod.8T"},
        {MacroIds::Kpod8H, "K-pod.8H"},
    };

    auto settings = RadioSettings::instance();

    for (const auto &slot : macroSlots) {
        auto *widget = new MacroItemWidget(slot.id, slot.displayName, m_listContainer);

        // Load saved macro
        MacroEntry macro = settings->macro(slot.id);
        widget->setLabel(macro.label);
        widget->setCommand(macro.command);

        connect(widget, &MacroItemWidget::clicked, this, [this, widget]() {
            int index = m_itemWidgets.indexOf(widget);
            if (index >= 0) {
                if (m_selectedIndex == index && !m_editMode) {
                    // Click on selected item enters edit mode for label
                    setEditMode(true, false);
                } else {
                    // Finish editing on OLD widget before changing selection
                    if (m_editMode && m_selectedIndex >= 0 && m_selectedIndex < m_itemWidgets.size()) {
                        m_itemWidgets[m_selectedIndex]->finishEditing();
                        m_editMode = false;
                        m_editBtn->setText("EDIT");
                    }
                    m_selectedIndex = index;
                    updateSelection();
                }
            }
        });

        connect(widget, &MacroItemWidget::commandClicked, this, [this, widget]() {
            int index = m_itemWidgets.indexOf(widget);
            if (index >= 0) {
                if (m_selectedIndex == index) {
                    // Click on command column enters edit mode for command
                    setEditMode(true, true);
                } else {
                    // Finish editing on OLD widget before changing selection
                    if (m_editMode && m_selectedIndex >= 0 && m_selectedIndex < m_itemWidgets.size()) {
                        m_itemWidgets[m_selectedIndex]->finishEditing();
                        m_editMode = false;
                        m_editBtn->setText("EDIT");
                    }
                    m_selectedIndex = index;
                    updateSelection();
                    // Then enter edit mode for command
                    setEditMode(true, true);
                }
            }
        });

        connect(widget, &MacroItemWidget::labelChanged, this, &MacroDialog::onLabelChanged);
        connect(widget, &MacroItemWidget::commandChanged, this, &MacroDialog::onCommandChanged);

        m_listLayout->insertWidget(m_listLayout->count() - 1, widget);
        m_itemWidgets.append(widget);
    }

    m_selectedIndex = 0;
    m_editMode = false;
    updateSelection();
}

void MacroDialog::loadFromSettings() {
    populateItems();
}

void MacroDialog::saveToSettings() {
    // Saves happen automatically via onLabelChanged/onCommandChanged
}

void MacroDialog::show() {
    loadFromSettings();
    QWidget::show();
    raise();
    setFocus();
}

void MacroDialog::hide() {
    // Finish any active editing
    for (auto *widget : m_itemWidgets) {
        widget->finishEditing();
    }
    QWidget::hide();
    emit closed();
}

void MacroDialog::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.fillRect(rect(), BackgroundColor);
}

void MacroDialog::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Up:
        navigateUp();
        break;
    case Qt::Key_Down:
        navigateDown();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        selectCurrent();
        break;
    case Qt::Key_Escape:
        if (m_editMode) {
            setEditMode(false);
        } else {
            closeDialog();
        }
        break;
    case Qt::Key_Tab:
        // In edit mode, Tab switches between label and command
        if (m_editMode) {
            setEditMode(true, !m_editingCommand);
        }
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void MacroDialog::wheelEvent(QWheelEvent *event) {
    if (m_editMode) {
        event->ignore();
        return;
    }

    int delta = event->angleDelta().y();
    if (delta > 0) {
        navigateUp();
    } else if (delta < 0) {
        navigateDown();
    }
    event->accept();
}

bool MacroDialog::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Wheel && !m_editMode) {
        auto *wheelEvent = static_cast<QWheelEvent *>(event);
        int delta = wheelEvent->angleDelta().y();
        if (delta > 0) {
            navigateUp();
        } else if (delta < 0) {
            navigateDown();
        }
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void MacroDialog::navigateUp() {
    if (m_editMode) {
        return;
    }
    if (m_selectedIndex > 0) {
        m_selectedIndex--;
        updateSelection();
        ensureSelectedVisible();
    }
}

void MacroDialog::navigateDown() {
    if (m_editMode) {
        return;
    }
    if (m_selectedIndex < m_itemWidgets.size() - 1) {
        m_selectedIndex++;
        updateSelection();
        ensureSelectedVisible();
    }
}

void MacroDialog::selectCurrent() {
    if (m_editMode) {
        // Finish editing and go back to browse mode
        setEditMode(false);
    } else {
        // Enter edit mode for label
        setEditMode(true, false);
    }
}

void MacroDialog::closeDialog() {
    hide();
}

void MacroDialog::onLabelChanged(const QString &functionId, const QString &label) {
    auto settings = RadioSettings::instance();
    MacroEntry macro = settings->macro(functionId);
    settings->setMacro(functionId, label, macro.command);
    qDebug() << "Macro label updated:" << functionId << "->" << label;
}

void MacroDialog::onCommandChanged(const QString &functionId, const QString &command) {
    auto settings = RadioSettings::instance();
    MacroEntry macro = settings->macro(functionId);
    settings->setMacro(functionId, macro.label, command);
    qDebug() << "Macro command updated:" << functionId << "->" << command;
}

void MacroDialog::updateSelection() {
    for (int i = 0; i < m_itemWidgets.size(); i++) {
        m_itemWidgets[i]->setSelected(i == m_selectedIndex);
    }
}

void MacroDialog::ensureSelectedVisible() {
    if (m_selectedIndex >= 0 && m_selectedIndex < m_itemWidgets.size()) {
        m_scrollArea->ensureWidgetVisible(m_itemWidgets[m_selectedIndex], 0, RowHeight);
    }
}

void MacroDialog::setEditMode(bool editing, bool editCommand) {
    m_editMode = editing;
    m_editingCommand = editCommand;

    if (m_selectedIndex >= 0 && m_selectedIndex < m_itemWidgets.size()) {
        auto *widget = m_itemWidgets[m_selectedIndex];
        if (editing) {
            if (editCommand) {
                widget->editCommand();
            } else {
                widget->editLabel();
            }
            m_editBtn->setText("DONE");
        } else {
            widget->finishEditing();
            m_editBtn->setText("EDIT");
        }
    }
}
