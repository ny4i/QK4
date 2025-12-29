#ifndef BANDPOPUPWIDGET_H
#define BANDPOPUPWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QList>
#include <QMap>

class BandPopupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BandPopupWidget(QWidget *parent = nullptr);

    // Position and show the popup above the specified button
    void showAboveButton(QWidget *triggerButton);

    // Hide the popup
    void hidePopup();

    // Set the currently selected band by name
    void setSelectedBand(const QString &bandName);
    QString selectedBand() const { return m_selectedBand; }

    // Band number methods for K4 BN command
    void setSelectedBandByNumber(int bandNum);
    int getBandNumber(const QString &bandName) const;
    QString getBandName(int bandNum) const;

signals:
    void bandSelected(const QString &bandName);
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void setupUi();
    QPushButton* createBandButton(const QString &text);
    void updateButtonStyles();
    void onBandButtonClicked();

    // Band buttons organized by row
    QList<QPushButton*> m_row1Buttons;  // 1.8, 3.5, 7, 14, 21, 28, MEM
    QList<QPushButton*> m_row2Buttons;  // GEN, 5, 10, 18, 24, 50, XVTR
    QMap<QString, QPushButton*> m_buttonMap;

    QString m_selectedBand;

    // Position info for drawing the indicator triangle
    int m_triangleXOffset;  // X offset from popup center to triangle point

    // Style sheets
    static QString normalButtonStyle();
    static QString selectedButtonStyle();
};

#endif // BANDPOPUPWIDGET_H
