#ifndef RIGHTSIDEPANEL_H
#define RIGHTSIDEPANEL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

/**
 * RightSidePanel - Right-side vertical panel for K4Controller
 *
 * Contains a 5×2 grid of dual-function buttons:
 * - Row 0: PRE/ATTN, NB/LEVEL
 * - Row 1: NR/ADJ, NTCH/MANUAL
 * - Row 2: FIL/APF, A/B/SPLIT
 * - Row 3: REV, A->B/B->A
 * - Row 4: SPOT/AUTO, MODE/ALT
 *
 * Dimensions:
 * - Fixed width: 105px (matches left panel)
 * - Margins: 6, 8, 6, 8
 * - Spacing: 4
 */
class RightSidePanel : public QWidget
{
    Q_OBJECT

public:
    explicit RightSidePanel(QWidget *parent = nullptr);
    ~RightSidePanel() = default;

    // Access main layout for adding content
    QVBoxLayout* contentLayout() { return m_layout; }

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

private:
    void setupUi();
    QWidget* createFunctionButton(const QString &mainText, const QString &subText, QPushButton *&btnOut);

    QVBoxLayout *m_layout;

    // Button pointers
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
};

#endif // RIGHTSIDEPANEL_H
