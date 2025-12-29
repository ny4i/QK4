#include "rightsidepanel.h"
#include <QGridLayout>

// K4 Color scheme (matching left panel)
namespace {
    const QString Background = "#1a1a1a";
    const QString TextWhite = "#FFFFFF";
    const QString TextGray = "#999999";
    const QString AmberOrange = "#FFB000";
}

RightSidePanel::RightSidePanel(QWidget *parent)
    : QWidget(parent)
    , m_preBtn(nullptr)
    , m_nbBtn(nullptr)
    , m_nrBtn(nullptr)
    , m_ntchBtn(nullptr)
    , m_filBtn(nullptr)
    , m_abBtn(nullptr)
    , m_revBtn(nullptr)
    , m_atobBtn(nullptr)
    , m_spotBtn(nullptr)
    , m_modeBtn(nullptr)
{
    setupUi();
}

void RightSidePanel::setupUi()
{
    // Match left panel dimensions exactly
    setFixedWidth(105);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(6, 8, 6, 8);
    m_layout->setSpacing(4);

    // Create 5×2 button grid
    auto *buttonGrid = new QGridLayout();
    buttonGrid->setContentsMargins(0, 0, 0, 0);
    buttonGrid->setHorizontalSpacing(4);
    buttonGrid->setVerticalSpacing(8);

    // Row 0: PRE/ATTN, NB/LEVEL
    buttonGrid->addWidget(createFunctionButton("PRE", "ATTN", m_preBtn), 0, 0);
    buttonGrid->addWidget(createFunctionButton("NB", "LEVEL", m_nbBtn), 0, 1);

    // Row 1: NR/ADJ, NTCH/MANUAL
    buttonGrid->addWidget(createFunctionButton("NR", "ADJ", m_nrBtn), 1, 0);
    buttonGrid->addWidget(createFunctionButton("NTCH", "MANUAL", m_ntchBtn), 1, 1);

    // Row 2: FIL/APF, A/B/SPLIT
    buttonGrid->addWidget(createFunctionButton("FIL", "APF", m_filBtn), 2, 0);
    buttonGrid->addWidget(createFunctionButton("A/B", "SPLIT", m_abBtn), 2, 1);

    // Row 3: REV, A->B/B->A
    buttonGrid->addWidget(createFunctionButton("REV", "", m_revBtn), 3, 0);
    buttonGrid->addWidget(createFunctionButton("A->B", "B->A", m_atobBtn), 3, 1);

    // Row 4: SPOT/AUTO, MODE/ALT
    buttonGrid->addWidget(createFunctionButton("SPOT", "AUTO", m_spotBtn), 4, 0);
    buttonGrid->addWidget(createFunctionButton("MODE", "ALT", m_modeBtn), 4, 1);

    m_layout->addLayout(buttonGrid);

    // Connect button signals
    connect(m_preBtn, &QPushButton::clicked, this, &RightSidePanel::preClicked);
    connect(m_nbBtn, &QPushButton::clicked, this, &RightSidePanel::nbClicked);
    connect(m_nrBtn, &QPushButton::clicked, this, &RightSidePanel::nrClicked);
    connect(m_ntchBtn, &QPushButton::clicked, this, &RightSidePanel::ntchClicked);
    connect(m_filBtn, &QPushButton::clicked, this, &RightSidePanel::filClicked);
    connect(m_abBtn, &QPushButton::clicked, this, &RightSidePanel::abClicked);
    connect(m_revBtn, &QPushButton::clicked, this, &RightSidePanel::revClicked);
    connect(m_atobBtn, &QPushButton::clicked, this, &RightSidePanel::atobClicked);
    connect(m_spotBtn, &QPushButton::clicked, this, &RightSidePanel::spotClicked);
    connect(m_modeBtn, &QPushButton::clicked, this, &RightSidePanel::modeClicked);

    // Add stretch at bottom to push content up (same as left panel pattern)
    m_layout->addStretch();
}

QWidget* RightSidePanel::createFunctionButton(const QString &mainText, const QString &subText, QPushButton *&btnOut)
{
    // Container widget for button + sub-text label
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setSpacing(5);

    // Button - scaled down from bottom menu bar style (matching left panel TX buttons)
    auto *btn = new QPushButton(mainText, container);
    btn->setFixedHeight(28);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a4a4a, stop:0.4 #3a3a3a,
                stop:0.6 #353535, stop:1 #2a2a2a);
            color: #FFFFFF;
            border: 1px solid #606060;
            border-radius: 4px;
            font-size: 9px;
            font-weight: bold;
            padding: 2px 4px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a5a5a, stop:0.4 #4a4a4a,
                stop:0.6 #454545, stop:1 #3a3a3a);
            border: 1px solid #808080;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2a2a2a, stop:0.4 #353535,
                stop:0.6 #3a3a3a, stop:1 #4a4a4a);
            border: 1px solid #909090;
        }
    )");
    btnOut = btn;
    layout->addWidget(btn);

    // Sub-text label (orange) - add top margin to prevent overlap with button
    auto *subLabel = new QLabel(subText, container);
    subLabel->setStyleSheet("color: #FFB000; font-size: 8px; margin-top: 4px;");
    subLabel->setAlignment(Qt::AlignCenter);
    subLabel->setFixedHeight(12);
    layout->addWidget(subLabel);

    return container;
}
