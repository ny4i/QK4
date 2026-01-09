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
    m_styleBtn = createMenuButton("STYLE"); // TEMP: For testing spectrum styles

    layout->addWidget(m_menuBtn);
    layout->addWidget(m_fnBtn);
    layout->addWidget(m_displayBtn);
    layout->addWidget(m_bandBtn);
    layout->addWidget(m_mainRxBtn);
    layout->addWidget(m_subRxBtn);
    layout->addWidget(m_txBtn);
    layout->addWidget(m_styleBtn);

    // Add stretch after buttons to center them
    layout->addStretch();

    // PTT button at far right (separated from main buttons)
    m_pttBtn = createMenuButton("PTT");
    layout->addWidget(m_pttBtn);

    // ===== Connect Signals =====
    connect(m_menuBtn, &QPushButton::clicked, this, &BottomMenuBar::menuClicked);
    connect(m_fnBtn, &QPushButton::clicked, this, &BottomMenuBar::fnClicked);
    connect(m_displayBtn, &QPushButton::clicked, this, &BottomMenuBar::displayClicked);
    connect(m_bandBtn, &QPushButton::clicked, this, &BottomMenuBar::bandClicked);
    connect(m_mainRxBtn, &QPushButton::clicked, this, &BottomMenuBar::mainRxClicked);
    connect(m_subRxBtn, &QPushButton::clicked, this, &BottomMenuBar::subRxClicked);
    connect(m_txBtn, &QPushButton::clicked, this, &BottomMenuBar::txClicked);
    connect(m_styleBtn, &QPushButton::clicked, this, &BottomMenuBar::styleClicked);

    // PTT uses press/release for momentary activation
    connect(m_pttBtn, &QPushButton::pressed, this, &BottomMenuBar::pttPressed);
    connect(m_pttBtn, &QPushButton::released, this, &BottomMenuBar::pttReleased);
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
            border: 2px solid #606060;
            border-radius: 6px;
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
            border: 2px solid #808080;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2a2a2a,
                stop:0.4 #353535,
                stop:0.6 #3a3a3a,
                stop:1 #4a4a4a);
            border: 2px solid #909090;
        }
    )";
}

QString BottomMenuBar::activeButtonStyleSheet() const {
    // Inverse colors when button is active (e.g., MENU button when menu is open)
    return R"(
        QPushButton {
            background: #FFFFFF;
            color: #666666;
            border: 2px solid #AAAAAA;
            border-radius: 6px;
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

void BottomMenuBar::setDisplayActive(bool active) {
    if (active) {
        m_displayBtn->setStyleSheet(activeButtonStyleSheet());
    } else {
        m_displayBtn->setStyleSheet(buttonStyleSheet());
    }
}

void BottomMenuBar::setBandActive(bool active) {
    if (active) {
        m_bandBtn->setStyleSheet(activeButtonStyleSheet());
    } else {
        m_bandBtn->setStyleSheet(buttonStyleSheet());
    }
}

void BottomMenuBar::setFnActive(bool active) {
    if (active) {
        m_fnBtn->setStyleSheet(activeButtonStyleSheet());
    } else {
        m_fnBtn->setStyleSheet(buttonStyleSheet());
    }
}

void BottomMenuBar::setMainRxActive(bool active) {
    if (active) {
        m_mainRxBtn->setStyleSheet(activeButtonStyleSheet());
    } else {
        m_mainRxBtn->setStyleSheet(buttonStyleSheet());
    }
}

void BottomMenuBar::setSubRxActive(bool active) {
    if (active) {
        m_subRxBtn->setStyleSheet(activeButtonStyleSheet());
    } else {
        m_subRxBtn->setStyleSheet(buttonStyleSheet());
    }
}

void BottomMenuBar::setTxActive(bool active) {
    if (active) {
        m_txBtn->setStyleSheet(activeButtonStyleSheet());
    } else {
        m_txBtn->setStyleSheet(buttonStyleSheet());
    }
}

void BottomMenuBar::setPttActive(bool active) {
    if (active) {
        m_pttBtn->setStyleSheet(activeButtonStyleSheet());
    } else {
        m_pttBtn->setStyleSheet(buttonStyleSheet());
    }
}
