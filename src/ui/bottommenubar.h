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
class BottomMenuBar : public QWidget
{
    Q_OBJECT

public:
    explicit BottomMenuBar(QWidget *parent = nullptr);
    ~BottomMenuBar() = default;

    // Getters for button positioning (for popup placement)
    QPushButton* bandButton() const { return m_bandBtn; }

public slots:
    void setMenuActive(bool active);  // Toggle MENU button inverse colors

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
    QPushButton* createMenuButton(const QString &text);
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
