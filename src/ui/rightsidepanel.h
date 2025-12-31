#ifndef RIGHTSIDEPANEL_H
#define RIGHTSIDEPANEL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

/**
 * RightSidePanel - Right-side vertical panel for K4Controller
 *
 * Contains button grids:
 *
 * 5×2 grid (main functions):
 * - Row 0: PRE/ATTN, NB/LEVEL
 * - Row 1: NR/ADJ, NTCH/MANUAL
 * - Row 2: FIL/APF, A/B/SPLIT
 * - Row 3: REV, A->B/B->A
 * - Row 4: SPOT/AUTO, MODE/ALT
 *
 * 2×2 grid (PF buttons):
 * - Row 0: B SET/PF 1, CLR/PF 2
 * - Row 1: RIT/PF 3, XIT/PF 4
 *
 * 2×2 grid (bottom functions):
 * - Row 0: FREQ ENT/SCAN, RATE/KHZ
 * - Row 1: LOCK A/LOCK B, SUB/DIVERSITY
 *
 * Dimensions:
 * - Fixed width: 105px (matches left panel)
 * - Margins: 6, 8, 6, 8
 * - Spacing: 4
 */
class RightSidePanel : public QWidget {
    Q_OBJECT

public:
    explicit RightSidePanel(QWidget *parent = nullptr);
    ~RightSidePanel() = default;

    // Access main layout for adding content
    QVBoxLayout *contentLayout() { return m_layout; }

signals:
    // Button click signals (main function)
    void preClicked();
    void nbClicked();
    void nrClicked();
    void ntchClicked();
    void filClicked();
    void abClicked();
    void revClicked();
    void atobClicked();
    void spotClicked();
    void modeClicked();

    // New button signals (PF row)
    void bsetClicked();
    void clrClicked();
    void ritClicked();
    void xitClicked();

    // New button signals (bottom row)
    void freqEntClicked();
    void rateClicked();
    void lockAClicked();
    void subClicked();

private:
    void setupUi();
    QWidget *createFunctionButton(const QString &mainText, const QString &subText, QPushButton *&btnOut);

    QVBoxLayout *m_layout;

    // Button pointers (existing 5x2 grid)
    QPushButton *m_preBtn;
    QPushButton *m_nbBtn;
    QPushButton *m_nrBtn;
    QPushButton *m_ntchBtn;
    QPushButton *m_filBtn;
    QPushButton *m_abBtn;
    QPushButton *m_revBtn;
    QPushButton *m_atobBtn;
    QPushButton *m_spotBtn;
    QPushButton *m_modeBtn;

    // New button pointers (PF row)
    QPushButton *m_bsetBtn;
    QPushButton *m_clrBtn;
    QPushButton *m_ritBtn;
    QPushButton *m_xitBtn;

    // New button pointers (bottom row)
    QPushButton *m_freqEntBtn;
    QPushButton *m_rateBtn;
    QPushButton *m_lockABtn;
    QPushButton *m_subBtn;
};

#endif // RIGHTSIDEPANEL_H
