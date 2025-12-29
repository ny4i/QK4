#include "bottommenubar.h"
#include <QHBoxLayout>

BottomMenuBar::BottomMenuBar(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void BottomMenuBar::setupUi() {
    setFixedHeight(52);

    auto *layout = new QHBoxLayout(this);
    // Left margin matches side panel width to align buttons with waterfall above
    layout->setContentsMargins(105, 6, 10, 6);
    layout->setSpacing(10); // Equal spacing between all buttons

    // Add stretch before buttons to center them
    layout->addStretch();

    // ===== Menu Buttons =====
    m_menuBtn = createMenuButton("MENU");
    m_fnBtn = createMenuButton("Fn");
    m_displayBtn = createMenuButton("DISPLAY");
    m_bandBtn = createMenuButton("BAND");
    m_mainRxBtn = createMenuButton("MAIN RX");
    m_subRxBtn = createMenuButton("SUB RX");
    m_txBtn = createMenuButton("TX");

    layout->addWidget(m_menuBtn);
    layout->addWidget(m_fnBtn);
    layout->addWidget(m_displayBtn);
    layout->addWidget(m_bandBtn);
    layout->addWidget(m_mainRxBtn);
    layout->addWidget(m_subRxBtn);
    layout->addWidget(m_txBtn);

    // Add stretch after buttons to center them
    layout->addStretch();

    // ===== Connect Signals =====
    connect(m_menuBtn, &QPushButton::clicked, this, &BottomMenuBar::menuClicked);
    connect(m_fnBtn, &QPushButton::clicked, this, &BottomMenuBar::fnClicked);
    connect(m_displayBtn, &QPushButton::clicked, this, &BottomMenuBar::displayClicked);
    connect(m_bandBtn, &QPushButton::clicked, this, &BottomMenuBar::bandClicked);
    connect(m_mainRxBtn, &QPushButton::clicked, this, &BottomMenuBar::mainRxClicked);
    connect(m_subRxBtn, &QPushButton::clicked, this, &BottomMenuBar::subRxClicked);
    connect(m_txBtn, &QPushButton::clicked, this, &BottomMenuBar::txClicked);
}

QPushButton *BottomMenuBar::createMenuButton(const QString &text) {
    auto *btn = new QPushButton(text, this);
    btn->setMinimumWidth(70);
    btn->setFixedHeight(36);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(buttonStyleSheet());
    return btn;
}

QString BottomMenuBar::buttonStyleSheet() const {
    // Subtle rounded edges, gradient grey to white, white border with thin line
    return R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a4a4a,
                stop:0.4 #3a3a3a,
                stop:0.6 #353535,
                stop:1 #2a2a2a);
            color: #FFFFFF;
            border: 1px solid #606060;
            border-radius: 5px;
            padding: 6px 12px;
            font-size: 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a5a5a,
                stop:0.4 #4a4a4a,
                stop:0.6 #454545,
                stop:1 #3a3a3a);
            border: 1px solid #808080;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2a2a2a,
                stop:0.4 #353535,
                stop:0.6 #3a3a3a,
                stop:1 #4a4a4a);
            border: 1px solid #909090;
        }
    )";
}

QString BottomMenuBar::activeButtonStyleSheet() const {
    // Inverse colors when button is active (e.g., MENU button when menu is open)
    return R"(
        QPushButton {
            background: #FFFFFF;
            color: #666666;
            border: 1px solid #AAAAAA;
            border-radius: 5px;
            padding: 6px 12px;
            font-size: 12px;
            font-weight: bold;
        }
    )";
}

void BottomMenuBar::setMenuActive(bool active) {
    if (active) {
        m_menuBtn->setStyleSheet(activeButtonStyleSheet());
    } else {
        m_menuBtn->setStyleSheet(buttonStyleSheet());
    }
}
