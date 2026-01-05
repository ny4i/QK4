#ifndef TXMETERWIDGET_H
#define TXMETERWIDGET_H

#include <QWidget>

/**
 * TxMeterWidget - Multi-function TX meter display (IC-7760 style)
 *
 * Displays 5 horizontal bar meters stacked vertically:
 * - Po (Forward Power): 0-100W (QRO) or 0-10W (QRP)
 * - ALC: 0-10 bars
 * - COMP: 0-30 dB compression
 * - SWR: 1.0 to 3.0+ ratio
 * - Id: 0-15A current draw
 *
 * Appears below S-meter on the TX VFO side (follows split state).
 */
class TxMeterWidget : public QWidget {
    Q_OBJECT

public:
    explicit TxMeterWidget(QWidget *parent = nullptr);

    // Set meter values (from TM command and supply current)
    void setPower(double watts, bool isQrp = false);
    void setAlc(int bars);        // 0-10
    void setCompression(int dB);  // 0-30
    void setSwr(double ratio);    // 1.0-3.0+
    void setCurrent(double amps); // 0-15

    // Set all TX meters at once (from txMeterChanged signal)
    void setTxMeters(int alc, int compDb, double fwdPower, double swr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // Meter values
    double m_power = 0.0;
    bool m_isQrp = false;
    int m_alc = 0;
    int m_compression = 0;
    double m_swr = 1.0;
    double m_current = 0.0;

    // Drawing helpers
    void drawMeterRow(QPainter &painter, int y, int rowHeight, const QString &label, double fillRatio,
                      const QStringList &scaleLabels, const QFont &scaleFont, int barStartX, int barWidth,
                      int barHeight);
};

#endif // TXMETERWIDGET_H
