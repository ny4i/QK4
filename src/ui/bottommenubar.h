#ifndef BOTTOMMENUBAR_H
#define BOTTOMMENUBAR_H

#include <QWidget>
#include <QPushButton>

/**
 * BottomMenuBar - Horizontal menu bar at bottom of K4Controller
 *
 * Contains 7 menu buttons: MENU, Fn, DISPLAY, BAND, MAIN RX, SUB RX, TX
 * (Icon buttons moved to SideControlPanel)
 *
 * Each menu button triggers a popup menu or panel when pressed.
 * Styled with subtle rounded edges, gradient background, white border.
 */
class BottomMenuBar : public QWidget {
    Q_OBJECT

public:
    explicit BottomMenuBar(QWidget *parent = nullptr);
    ~BottomMenuBar() = default;

    // Getters for button positioning (for popup placement)
    QPushButton *bandButton() const { return m_bandBtn; }
    QPushButton *displayButton() const { return m_displayBtn; }
    QPushButton *fnButton() const { return m_fnBtn; }
    QPushButton *mainRxButton() const { return m_mainRxBtn; }
    QPushButton *subRxButton() const { return m_subRxBtn; }
    QPushButton *txButton() const { return m_txBtn; }

public slots:
    void setMenuActive(bool active);    // Toggle MENU button inverse colors
    void setDisplayActive(bool active); // Toggle DISPLAY button inverse colors
    void setBandActive(bool active);    // Toggle BAND button inverse colors
    void setFnActive(bool active);      // Toggle Fn button inverse colors
    void setMainRxActive(bool active);  // Toggle MAIN RX button inverse colors
    void setSubRxActive(bool active);   // Toggle SUB RX button inverse colors
    void setTxActive(bool active);      // Toggle TX button inverse colors

signals:
    void menuClicked();
    void fnClicked();
    void displayClicked();
    void bandClicked();
    void mainRxClicked();
    void subRxClicked();
    void txClicked();

private:
    void setupUi();
    QPushButton *createMenuButton(const QString &text);
    QString buttonStyleSheet() const;
    QString activeButtonStyleSheet() const;

    // Menu buttons
    QPushButton *m_menuBtn;
    QPushButton *m_fnBtn;
    QPushButton *m_displayBtn;
    QPushButton *m_bandBtn;
    QPushButton *m_mainRxBtn;
    QPushButton *m_subRxBtn;
    QPushButton *m_txBtn;
};

#endif // BOTTOMMENUBAR_H
