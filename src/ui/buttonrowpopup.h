#ifndef BUTTONROWPOPUP_H
#define BUTTONROWPOPUP_H

#include <QWidget>
#include <QPushButton>
#include <QList>

/**
 * ButtonRowPopup - A single-row popup menu with 7 buttons
 *
 * Based on BandPopupWidget styling but simplified to one row.
 * Used for Fn, MAIN RX, SUB RX, and TX bottom menu popups.
 * Features:
 * - 7 buttons in a horizontal row
 * - Gray bottom strip with triangle indicator pointing to trigger button
 * - Customizable button labels
 */
class ButtonRowPopup : public QWidget {
    Q_OBJECT

public:
    explicit ButtonRowPopup(QWidget *parent = nullptr);

    // Position and show the popup above the specified button
    void showAboveButton(QWidget *triggerButton);

    // Hide the popup
    void hidePopup();

    // Configure button labels
    void setButtonLabels(const QStringList &labels);
    void setButtonLabel(int index, const QString &label);
    QString buttonLabel(int index) const;

signals:
    void buttonClicked(int index); // Emits 0-6 for button clicks
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void setupUi();
    void onButtonClicked();

    QList<QPushButton *> m_buttons; // 7 buttons

    // Position info for drawing the indicator triangle
    int m_triangleXOffset = 0; // X offset from popup center to triangle point

    // Style sheets
    static QString normalButtonStyle();
};

#endif // BUTTONROWPOPUP_H
