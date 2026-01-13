#ifndef BALOVERLAY_H
#define BALOVERLAY_H

#include "sidecontroloverlay.h"
#include <QLabel>

/**
 * @brief Sub AF balance overlay for SideControlPanel.
 *
 * Shows SUB AF balance mode with MAIN and SUB values.
 * CAT commands TBD - placeholder implementation.
 */
class BalOverlay : public SideControlOverlay {
    Q_OBJECT

public:
    explicit BalOverlay(QWidget *parent = nullptr);

    /**
     * @brief Set the balance mode display text.
     * @param mode Mode string (e.g., "NOR", "BAL", etc.)
     */
    void setBalanceMode(const QString &mode);

    /**
     * @brief Set the MAIN value.
     * @param value Main value (typically 0-100)
     */
    void setMainValue(int value);

    /**
     * @brief Set the SUB value.
     * @param value Sub value (typically 0-100)
     */
    void setSubValue(int value);

    QString balanceMode() const { return m_balanceMode; }
    int mainValue() const { return m_mainValue; }
    int subValue() const { return m_subValue; }

signals:
    /**
     * @brief Emitted when the user scrolls to change balance.
     * @param delta Scroll delta (positive = increase, negative = decrease)
     */
    void balanceChangeRequested(int delta);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setupUi();
    void updateDisplay();

    QLabel *m_titleLabel;
    QLabel *m_modeLabel;
    QLabel *m_mainLabel;
    QLabel *m_subLabel;

    QString m_balanceMode = "NOR";
    int m_mainValue = 50;
    int m_subValue = 50;
};

#endif // BALOVERLAY_H
