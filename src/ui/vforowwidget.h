#ifndef VFOROWWIDGET_H
#define VFOROWWIDGET_H

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

/**
 * VfoRowWidget - First row of center section with absolute positioning
 *
 * Contains: A square, TX indicator, B square, SUB/DIV indicators
 * Uses absolute positioning to perfectly center TX regardless of
 * asymmetric SUB/DIV on B side.
 */
class VfoRowWidget : public QWidget {
    Q_OBJECT

public:
    explicit VfoRowWidget(QWidget *parent = nullptr);

    // Accessors for MainWindow to connect signals and install event filters
    QLabel *vfoASquare() const { return m_vfoASquare; }
    QLabel *vfoBSquare() const { return m_vfoBSquare; }
    QLabel *modeALabel() const { return m_modeALabel; }
    QLabel *modeBLabel() const { return m_modeBLabel; }
    QLabel *txIndicator() const { return m_txIndicator; }
    QLabel *txTriangle() const { return m_txTriangle; }
    QLabel *txTriangleB() const { return m_txTriangleB; }
    QLabel *testLabel() const { return m_testLabel; }
    QLabel *subLabel() const { return m_subLabel; }
    QLabel *divLabel() const { return m_divLabel; }

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupWidgets();
    void positionWidgets();

    // Containers (absolute positioned within this widget)
    QWidget *m_vfoAContainer;
    QWidget *m_txContainer;
    QWidget *m_vfoBContainer;
    QWidget *m_subDivContainer;

    // Labels within containers
    QLabel *m_vfoASquare;
    QLabel *m_vfoBSquare;
    QLabel *m_modeALabel;
    QLabel *m_modeBLabel;
    QLabel *m_txIndicator;
    QLabel *m_txTriangle;
    QLabel *m_txTriangleB;
    QLabel *m_testLabel;
    QLabel *m_subLabel;
    QLabel *m_divLabel;
};

#endif // VFOROWWIDGET_H
