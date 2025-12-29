#ifndef SMETERWIDGET_H
#define SMETERWIDGET_H

#include <QWidget>
#include <QColor>

class SMeterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SMeterWidget(QWidget *parent = nullptr);

    void setValue(double sValue);  // S-units (0-9, then 9+dB)
    void setColor(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_value = 0.0;
    QColor m_color;
};

#endif // SMETERWIDGET_H
