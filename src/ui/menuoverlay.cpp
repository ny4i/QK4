#include "menuoverlay.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QDebug>

// ============== MenuItemWidget ==============

MenuItemWidget::MenuItemWidget(MenuItem *item, QWidget *parent) : QWidget(parent), m_item(item) {
    setFixedHeight(40);
    setCursor(Qt::PointingHandCursor);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 5, 15, 5);
    layout->setSpacing(10);

    // Name label
    m_nameLabel = new QLabel(item->name, this);
    m_nameLabel->setStyleSheet("color: #AAAAAA; font-size: 14px;"); // Initial unselected state
    layout->addWidget(m_nameLabel, 1);

    // Lock icon (for read-only items)
    m_lockLabel = new QLabel(this);
    if (item->isReadOnly()) {
        m_lockLabel->setText("\xF0\x9F\x94\x92"); // Lock emoji
    }
    m_lockLabel->setFixedWidth(20);
    layout->addWidget(m_lockLabel);

    // Value label
    m_valueLabel = new QLabel(item->displayValue(), this);
    m_valueLabel->setStyleSheet("color: #888888; font-size: 14px; font-weight: bold;"); // Initial unselected state
    m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_valueLabel->setMinimumWidth(80);
    layout->addWidget(m_valueLabel);

    setLayout(layout);
}

void MenuItemWidget::setSelected(bool selected) {
    m_selected = selected;
    updateLabelColors();
    update();
}

void MenuItemWidget::updateLabelColors() {
    // Update label colors based on selection and edit state
    // Two-zone highlighting: left zone (name) and right zone (value) have different colors
    if (m_selected) {
        if (m_editing) {
            // EDITING MODE: name on grey, value on off-white
            m_nameLabel->setStyleSheet("color: #CCCCCC; font-size: 14px;"); // Light text on grey
            m_valueLabel->setStyleSheet(
                "color: #333333; font-size: 14px; font-weight: bold;"); // Dark text on off-white
        } else {
            // BROWSE MODE: name on off-white, value on grey
            m_nameLabel->setStyleSheet("color: #333333; font-size: 14px;"); // Dark text on off-white
            m_valueLabel->setStyleSheet("color: #FFFFFF; font-size: 14px; font-weight: bold;"); // White text on grey
        }
    } else {
        // Unselected: grey text on dark background
        m_nameLabel->setStyleSheet("color: #AAAAAA; font-size: 14px;");
        m_valueLabel->setStyleSheet("color: #888888; font-size: 14px; font-weight: bold;");
    }
}

void MenuItemWidget::setEditMode(bool editing) {
    m_editing = editing;
    updateLabelColors();
    update();
}

void MenuItemWidget::updateDisplay() {
    m_valueLabel->setText(m_item->displayValue());
}

void MenuItemWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Demarcation at 70% width (separates menu item name from value)
    int demarcX = static_cast<int>(width() * 0.7);

    if (m_selected) {
        if (m_editing) {
            // EDITING MODE: Colors swapped to indicate editing value
            // Left zone (item name): Grey
            painter.fillRect(QRect(0, 0, demarcX, height()), QColor(80, 80, 85));
            // Right zone (value): Off-white to show it's being edited
            painter.fillRect(QRect(demarcX, 0, width() - demarcX, height()), QColor(220, 220, 220));
        } else {
            // BROWSE MODE: Normal highlighting
            // Left zone (item name): Off-white to show selection
            painter.fillRect(QRect(0, 0, demarcX, height()), QColor(220, 220, 220));
            // Right zone (value): Grey
            painter.fillRect(QRect(demarcX, 0, width() - demarcX, height()), QColor(80, 80, 85));
        }
        // Draw vertical demarcation line
        painter.setPen(QColor(60, 60, 65));
        painter.drawLine(demarcX, 0, demarcX, height());
    } else {
        // Unselected: dark background
        painter.fillRect(rect(), QColor(25, 25, 30));
    }

    // Bottom border
    painter.setPen(QColor(40, 40, 45));
    painter.drawLine(0, height() - 1, width(), height() - 1);
}

void MenuItemWidget::mousePressEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    emit clicked();
}

// ============== MenuOverlayWidget ==============

MenuOverlayWidget::MenuOverlayWidget(MenuModel *model, QWidget *parent) : QWidget(parent), m_model(model) {
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFocusPolicy(Qt::StrongFocus);

    setupUi();

    connect(m_model, &MenuModel::menuValueChanged, this, &MenuOverlayWidget::onMenuValueChanged);
}

void MenuOverlayWidget::setupUi() {
    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Content area (left side - menu list)
    m_contentWidget = new QWidget(this);
    m_contentWidget->setStyleSheet("background-color: #18181C;");

    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // Category header
    m_categoryLabel = new QLabel("MENU", m_contentWidget);
    m_categoryLabel->setStyleSheet("background-color: #222228; color: #666; font-size: 12px; "
                                   "font-weight: bold; padding: 8px 15px;");
    contentLayout->addWidget(m_categoryLabel);

    // Scroll area for menu items
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

    // Install event filter to capture wheel events from scroll area
    m_scrollArea->installEventFilter(this);
    m_scrollArea->viewport()->installEventFilter(this);

    mainLayout->addWidget(m_contentWidget, 1);

    // Navigation panel (right side) - using grid layout for 2-column arrangement
    QWidget *navPanel = new QWidget(this);
    navPanel->setFixedWidth(130);
    navPanel->setStyleSheet("background-color: #222228;");

    QVBoxLayout *navOuterLayout = new QVBoxLayout(navPanel);
    navOuterLayout->setContentsMargins(8, 12, 8, 12);
    navOuterLayout->setSpacing(8);

    QString buttonStyle = "QPushButton { background-color: #3A3A45; color: white; border: none; "
                          "border-radius: 6px; font-size: 16px; font-weight: bold; }"
                          "QPushButton:pressed { background-color: #505060; }";

    // Row 1: Up and Down buttons side by side
    QHBoxLayout *row1 = new QHBoxLayout();
    row1->setSpacing(8);

    m_upBtn = new QPushButton("\xE2\x96\xB2", navPanel); // ▲
    m_upBtn->setFixedSize(54, 44);
    m_upBtn->setStyleSheet(buttonStyle);
    connect(m_upBtn, &QPushButton::clicked, this, &MenuOverlayWidget::navigateUp);
    row1->addWidget(m_upBtn);

    m_downBtn = new QPushButton("\xE2\x96\xBC", navPanel); // ▼
    m_downBtn->setFixedSize(54, 44);
    m_downBtn->setStyleSheet(buttonStyle);
    connect(m_downBtn, &QPushButton::clicked, this, &MenuOverlayWidget::navigateDown);
    row1->addWidget(m_downBtn);

    navOuterLayout->addLayout(row1);
    navOuterLayout->addStretch();

    // Row 2: NORM and Back buttons side by side
    QHBoxLayout *row3 = new QHBoxLayout();
    row3->setSpacing(8);

    m_normBtn = new QPushButton("NORM", navPanel);
    m_normBtn->setFixedSize(54, 44);
    m_normBtn->setStyleSheet("QPushButton { background-color: #3A3A45; color: #888; border: none; "
                             "border-radius: 6px; font-size: 10px; font-weight: bold; }"
                             "QPushButton:pressed { background-color: #505060; }");
    connect(m_normBtn, &QPushButton::clicked, this, &MenuOverlayWidget::resetToDefault);
    row3->addWidget(m_normBtn);

    m_backBtn = new QPushButton("\xE2\x86\xA9", navPanel); // ↩
    m_backBtn->setFixedSize(54, 44);
    m_backBtn->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                             "stop:0 #4a4a4a, stop:0.4 #3a3a3a, stop:0.6 #353535, stop:1 #2a2a2a);"
                             "color: white; border: 1px solid #606060; border-radius: 6px; "
                             "font-size: 16px; font-weight: bold; }"
                             "QPushButton:pressed { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                             "stop:0 #2a2a2a, stop:0.4 #353535, stop:0.6 #3a3a3a, stop:1 #4a4a4a); }");
    connect(m_backBtn, &QPushButton::clicked, this, &MenuOverlayWidget::closeOverlay);
    row3->addWidget(m_backBtn);

    navOuterLayout->addLayout(row3);

    mainLayout->addWidget(navPanel);

    setLayout(mainLayout);
}

void MenuOverlayWidget::populateItems() {
    // Clear existing
    for (auto *widget : m_itemWidgets) {
        m_listLayout->removeWidget(widget);
        delete widget;
    }
    m_itemWidgets.clear();

    // Add menu items
    QVector<MenuItem *> items = m_model->getAllItems();
    for (MenuItem *item : items) {
        MenuItemWidget *widget = new MenuItemWidget(item, m_listContainer);
        connect(widget, &MenuItemWidget::clicked, this, [this, widget]() {
            int index = m_itemWidgets.indexOf(widget);
            if (index >= 0) {
                if (m_selectedIndex == index && !m_editMode) {
                    // Click on already-selected item enters edit mode
                    setEditMode(true);
                } else {
                    m_selectedIndex = index;
                    setEditMode(false);
                    updateSelection();
                }
            }
        });
        m_listLayout->insertWidget(m_listLayout->count() - 1, widget); // Before stretch
        m_itemWidgets.append(widget);
    }

    m_selectedIndex = 0;
    m_editMode = false;
    updateSelection();
    updateButtonLabels();
    updateNormButton();
}

void MenuOverlayWidget::show() {
    populateItems();
    QWidget::show();
    setFocus();
}

void MenuOverlayWidget::hide() {
    QWidget::hide();
    emit closed();
}

void MenuOverlayWidget::refresh() {
    for (auto *widget : m_itemWidgets) {
        widget->updateDisplay();
    }
    updateNormButton();
}

void MenuOverlayWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(24, 24, 28));
}

void MenuOverlayWidget::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Up:
        navigateUp();
        break;
    case Qt::Key_Down:
        navigateDown();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        selectCurrent();
        break;
    case Qt::Key_Escape:
        if (m_editMode) {
            setEditMode(false);
        } else {
            closeOverlay();
        }
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        if (!m_itemWidgets.isEmpty() && m_selectedIndex < m_itemWidgets.size()) {
            MenuItem *item = m_itemWidgets[m_selectedIndex]->menuItem();
            emit menuValueChangeRequested(item->id, "+");
        }
        break;
    case Qt::Key_Minus:
        if (!m_itemWidgets.isEmpty() && m_selectedIndex < m_itemWidgets.size()) {
            MenuItem *item = m_itemWidgets[m_selectedIndex]->menuItem();
            emit menuValueChangeRequested(item->id, "-");
        }
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void MenuOverlayWidget::mousePressEvent(QMouseEvent *event) {
    // Close if clicking outside content area (but not on nav panel)
    QRect contentRect = m_contentWidget->geometry();
    if (!contentRect.contains(event->pos()) && event->pos().x() < contentRect.x()) {
        closeOverlay();
    }
}

void MenuOverlayWidget::wheelEvent(QWheelEvent *event) {
    int delta = event->angleDelta().y();
    if (delta == 0)
        return;

    if (m_editMode) {
        // In edit mode, wheel changes value
        if (!m_itemWidgets.isEmpty() && m_selectedIndex < m_itemWidgets.size()) {
            MenuItem *item = m_itemWidgets[m_selectedIndex]->menuItem();
            if (delta > 0) {
                emit menuValueChangeRequested(item->id, "+");
            } else {
                emit menuValueChangeRequested(item->id, "-");
            }
        }
    } else {
        // In browse mode, wheel scrolls through items
        if (delta > 0) {
            navigateUp();
        } else {
            navigateDown();
        }
    }
    event->accept();
}

bool MenuOverlayWidget::eventFilter(QObject *watched, QEvent *event) {
    // Intercept wheel events from scroll area to move selection instead of scrolling
    if (event->type() == QEvent::Wheel) {
        if (watched == m_scrollArea || watched == m_scrollArea->viewport()) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
            wheelEvent->accept();

            int delta = wheelEvent->angleDelta().y();
            if (delta == 0)
                return true;

            if (m_editMode) {
                // In edit mode, wheel changes value
                if (!m_itemWidgets.isEmpty() && m_selectedIndex < m_itemWidgets.size()) {
                    MenuItem *item = m_itemWidgets[m_selectedIndex]->menuItem();
                    if (delta > 0) {
                        emit menuValueChangeRequested(item->id, "+");
                    } else {
                        emit menuValueChangeRequested(item->id, "-");
                    }
                }
            } else {
                // In browse mode, wheel moves selection (with hard stops)
                if (delta > 0) {
                    navigateUp();
                } else {
                    navigateDown();
                }
            }
            return true; // Event handled, don't let scroll area scroll
        }
    }
    return QWidget::eventFilter(watched, event);
}

void MenuOverlayWidget::navigateUp() {
    if (m_editMode) {
        // In edit mode, up increments value
        if (!m_itemWidgets.isEmpty() && m_selectedIndex < m_itemWidgets.size()) {
            MenuItem *item = m_itemWidgets[m_selectedIndex]->menuItem();
            emit menuValueChangeRequested(item->id, "+");
        }
    } else {
        // In browse mode, up moves selection
        if (m_selectedIndex > 0) {
            m_selectedIndex--;
            updateSelection();
            ensureSelectedVisible();
            updateNormButton();
        }
    }
}

void MenuOverlayWidget::navigateDown() {
    if (m_editMode) {
        // In edit mode, down decrements value
        if (!m_itemWidgets.isEmpty() && m_selectedIndex < m_itemWidgets.size()) {
            MenuItem *item = m_itemWidgets[m_selectedIndex]->menuItem();
            emit menuValueChangeRequested(item->id, "-");
        }
    } else {
        // In browse mode, down moves selection
        if (m_selectedIndex < m_itemWidgets.size() - 1) {
            m_selectedIndex++;
            updateSelection();
            ensureSelectedVisible();
            updateNormButton();
        }
    }
}

void MenuOverlayWidget::selectCurrent() {
    if (m_itemWidgets.isEmpty() || m_selectedIndex >= m_itemWidgets.size()) {
        return;
    }

    MenuItem *item = m_itemWidgets[m_selectedIndex]->menuItem();
    if (item->isReadOnly()) {
        return;
    }

    if (m_editMode) {
        // Exit edit mode (confirm)
        setEditMode(false);
    } else {
        // Enter edit mode
        setEditMode(true);
    }
}

void MenuOverlayWidget::closeOverlay() {
    if (m_editMode) {
        setEditMode(false);
    } else {
        hide();
    }
}

void MenuOverlayWidget::resetToDefault() {
    if (m_itemWidgets.isEmpty() || m_selectedIndex >= m_itemWidgets.size()) {
        return;
    }

    MenuItem *item = m_itemWidgets[m_selectedIndex]->menuItem();
    if (item->isReadOnly()) {
        return;
    }

    // Set value to default
    int defaultVal = item->defaultValue;
    QString cmd = QString("%1").arg(defaultVal, 4, 10, QChar('0'));
    emit menuValueChangeRequested(item->id, cmd);

    // Also update local model immediately
    m_model->updateValue(item->id, defaultVal);
}

void MenuOverlayWidget::updateSelection() {
    for (int i = 0; i < m_itemWidgets.size(); ++i) {
        m_itemWidgets[i]->setSelected(i == m_selectedIndex);
        m_itemWidgets[i]->setEditMode(i == m_selectedIndex && m_editMode);
    }
}

void MenuOverlayWidget::ensureSelectedVisible() {
    if (m_selectedIndex >= 0 && m_selectedIndex < m_itemWidgets.size()) {
        m_scrollArea->ensureWidgetVisible(m_itemWidgets[m_selectedIndex]);
    }
}

void MenuOverlayWidget::setEditMode(bool editing) {
    m_editMode = editing;
    updateSelection();
    updateButtonLabels();
    updateNormButton();
}

void MenuOverlayWidget::updateButtonLabels() {
    if (m_editMode) {
        m_upBtn->setText("+");
        m_downBtn->setText("-");
    } else {
        m_upBtn->setText("\xE2\x96\xB2");   // ▲
        m_downBtn->setText("\xE2\x96\xBC"); // ▼
    }
}

void MenuOverlayWidget::updateNormButton() {
    if (m_itemWidgets.isEmpty() || m_selectedIndex >= m_itemWidgets.size()) {
        m_normBtn->setStyleSheet("QPushButton { background-color: #3A3A45; color: #888; border: none; "
                                 "border-radius: 6px; font-size: 10px; font-weight: bold; }");
        return;
    }

    MenuItem *item = m_itemWidgets[m_selectedIndex]->menuItem();
    bool isDefault = (item->currentValue == item->defaultValue);

    if (isDefault) {
        // Grey - value is at default
        m_normBtn->setStyleSheet("QPushButton { background-color: #3A3A45; color: #888; border: none; "
                                 "border-radius: 6px; font-size: 10px; font-weight: bold; }"
                                 "QPushButton:pressed { background-color: #505060; }");
    } else {
        // White/bright - value differs from default
        m_normBtn->setStyleSheet("QPushButton { background-color: #DDD; color: #333; border: none; "
                                 "border-radius: 6px; font-size: 10px; font-weight: bold; }"
                                 "QPushButton:pressed { background-color: #FFF; }");
    }
}

void MenuOverlayWidget::onMenuValueChanged(int menuId, int newValue) {
    Q_UNUSED(newValue);
    // Find and update the widget for this menu item
    for (auto *widget : m_itemWidgets) {
        if (widget->menuItem()->id == menuId) {
            widget->updateDisplay();
            break;
        }
    }
    updateNormButton();
}
