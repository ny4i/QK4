#ifndef WATERFALL_H
#define WATERFALL_H

#include <QWidget>
#include <QImage>
#include <QVector>

class WaterfallWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WaterfallWidget(QWidget *parent = nullptr);

    void addLine(const QVector<float> &spectrum);
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QRgb valueToColor(float value);
    QImage m_image;
    int m_currentLine;
};

#endif // WATERFALL_H
