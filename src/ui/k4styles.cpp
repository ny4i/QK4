#include "k4styles.h"

namespace K4Styles {

QString popupButtonNormal() {
    return QString(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a4a4a,
                stop:0.4 #3a3a3a,
                stop:0.6 #353535,
                stop:1 #2a2a2a);
            color: #FFFFFF;
            border: 2px solid #606060;
            border-radius: 6px;
            font-size: %1px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a5a5a,
                stop:0.4 #4a4a4a,
                stop:0.6 #454545,
                stop:1 #3a3a3a);
            border: 2px solid #808080;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2a2a2a,
                stop:0.4 #353535,
                stop:0.6 #3a3a3a,
                stop:1 #4a4a4a);
        }
    )")
        .arg(Dimensions::PopupButtonSize);
}

QString popupButtonSelected() {
    return QString(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #E0E0E0,
                stop:0.4 #D0D0D0,
                stop:0.6 #C8C8C8,
                stop:1 #B8B8B8);
            color: #333333;
            border: 2px solid #AAAAAA;
            border-radius: 6px;
            font-size: %1px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #F0F0F0,
                stop:0.4 #E0E0E0,
                stop:0.6 #D8D8D8,
                stop:1 #C8C8C8);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #B8B8B8,
                stop:0.4 #C8C8C8,
                stop:0.6 #D0D0D0,
                stop:1 #E0E0E0);
        }
    )")
        .arg(Dimensions::PopupButtonSize);
}

QString menuBarButton() {
    return QString(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a4a4a,
                stop:0.4 #3a3a3a,
                stop:0.6 #353535,
                stop:1 #2a2a2a);
            color: #FFFFFF;
            border: 2px solid #606060;
            border-radius: 6px;
            padding: 6px 12px;
            font-size: %1px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a5a5a,
                stop:0.4 #4a4a4a,
                stop:0.6 #454545,
                stop:1 #3a3a3a);
            border: 2px solid #808080;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2a2a2a,
                stop:0.4 #353535,
                stop:0.6 #3a3a3a,
                stop:1 #4a4a4a);
            border: 2px solid #909090;
        }
    )")
        .arg(Dimensions::PopupButtonSize);
}

QString menuBarButtonActive() {
    return QString(R"(
        QPushButton {
            background: #FFFFFF;
            color: #666666;
            border: 2px solid #AAAAAA;
            border-radius: 6px;
            padding: 6px 12px;
            font-size: %1px;
            font-weight: 600;
        }
    )")
        .arg(Dimensions::PopupButtonSize);
}

QString menuBarButtonSmall() {
    return QString(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a4a4a,
                stop:0.4 #3a3a3a,
                stop:0.6 #353535,
                stop:1 #2a2a2a);
            color: #FFFFFF;
            border: 2px solid #606060;
            border-radius: 6px;
            font-size: %1px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a5a5a,
                stop:0.4 #4a4a4a,
                stop:0.6 #454545,
                stop:1 #3a3a3a);
            border: 2px solid #808080;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2a2a2a,
                stop:0.4 #353535,
                stop:0.6 #3a3a3a,
                stop:1 #4a4a4a);
            border: 2px solid #909090;
        }
    )")
        .arg(Dimensions::PopupValueSize);
}

QString menuBarButtonPttPressed() {
    return QString(R"(
        QPushButton {
            background: %1;
            color: #FFFFFF;
            border: 2px solid %1;
            border-radius: 6px;
            padding: 6px 12px;
            font-size: %2px;
            font-weight: 600;
        }
    )")
        .arg(Colors::TxRed)
        .arg(Dimensions::PopupButtonSize);
}

void drawDropShadow(QPainter &painter, const QRect &contentRect, int cornerRadius) {
    painter.setPen(Qt::NoPen);
    for (int i = Dimensions::ShadowLayers; i > 0; --i) {
        int blur = i * 2;
        int alpha = 12 + (Dimensions::ShadowLayers - i) * 3;
        QRect shadowRect = contentRect.adjusted(-blur, -blur, blur, blur);
        shadowRect.translate(Dimensions::ShadowOffsetX, Dimensions::ShadowOffsetY);
        painter.setBrush(QColor(0, 0, 0, alpha));
        painter.drawRoundedRect(shadowRect, cornerRadius + blur / 2, cornerRadius + blur / 2);
    }
}

QLinearGradient buttonGradient(int top, int bottom, bool hovered) {
    QLinearGradient grad(0, top, 0, bottom);
    if (hovered) {
        grad.setColorAt(0, QColor(Colors::HoverTop));
        grad.setColorAt(0.4, QColor(Colors::HoverMid1));
        grad.setColorAt(0.6, QColor(Colors::HoverMid2));
        grad.setColorAt(1, QColor(Colors::HoverBottom));
    } else {
        grad.setColorAt(0, QColor(Colors::GradientTop));
        grad.setColorAt(0.4, QColor(Colors::GradientMid1));
        grad.setColorAt(0.6, QColor(Colors::GradientMid2));
        grad.setColorAt(1, QColor(Colors::GradientBottom));
    }
    return grad;
}

QLinearGradient selectedGradient(int top, int bottom) {
    QLinearGradient grad(0, top, 0, bottom);
    grad.setColorAt(0, QColor(Colors::SelectedTop));
    grad.setColorAt(0.4, QColor(Colors::SelectedMid1));
    grad.setColorAt(0.6, QColor(Colors::SelectedMid2));
    grad.setColorAt(1, QColor(Colors::SelectedBottom));
    return grad;
}

QColor borderColor() {
    return QColor(Colors::BorderNormal);
}

QColor borderColorSelected() {
    return QColor(Colors::BorderSelected);
}

QLinearGradient meterGradient(qreal x1, qreal y1, qreal x2, qreal y2) {
    QLinearGradient gradient(x1, y1, x2, y2);
    gradient.setColorAt(0.00, QColor(Colors::MeterGreenDark));
    gradient.setColorAt(0.15, QColor(Colors::MeterGreen));
    gradient.setColorAt(0.30, QColor(Colors::MeterYellowGreen));
    gradient.setColorAt(0.45, QColor(Colors::MeterYellow));
    gradient.setColorAt(0.60, QColor(Colors::MeterOrange));
    gradient.setColorAt(0.80, QColor(Colors::MeterOrangeRed));
    gradient.setColorAt(1.00, QColor(Colors::MeterRed));
    return gradient;
}

QString sliderHorizontal(const QString &grooveColor, const QString &handleColor) {
    return QString("QSlider::groove:horizontal { background: %1; height: %2px; border-radius: %3px; }"
                   "QSlider::handle:horizontal { background: %4; width: %5px; margin: %6px 0; border-radius: %7px; }"
                   "QSlider::sub-page:horizontal { background: %4; border-radius: %3px; }")
        .arg(grooveColor)
        .arg(Dimensions::SliderGrooveHeight)
        .arg(Dimensions::SliderBorderRadius)
        .arg(handleColor)
        .arg(Dimensions::SliderHandleWidth)
        .arg(Dimensions::SliderHandleMargin)
        .arg(Dimensions::SliderHandleRadius);
}

QString checkboxButton(int size) {
    // Checkbox shows ✓ when checked, empty when unchecked
    // Uses dark gradient unchecked, same dark gradient + white checkmark when checked
    return QString(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a4a4a,
                stop:0.4 #3a3a3a,
                stop:0.6 #353535,
                stop:1 #2a2a2a);
            color: transparent;
            border: 1px solid #606060;
            border-radius: 3px;
            min-width: %1px;
            max-width: %1px;
            min-height: %1px;
            max-height: %1px;
            font-size: %2px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a5a5a,
                stop:0.4 #4a4a4a,
                stop:0.6 #454545,
                stop:1 #3a3a3a);
            border: 1px solid #808080;
        }
        QPushButton:checked {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a4a4a,
                stop:0.4 #3a3a3a,
                stop:0.6 #353535,
                stop:1 #2a2a2a);
            color: #FFFFFF;
            border: 1px solid #808080;
        }
        QPushButton:checked:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a5a5a,
                stop:0.4 #4a4a4a,
                stop:0.6 #454545,
                stop:1 #3a3a3a);
        }
        QPushButton:disabled {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #3a3a3a,
                stop:0.4 #2a2a2a,
                stop:0.6 #252525,
                stop:1 #1a1a1a);
            border: 1px solid #404040;
            color: transparent;
        }
        QPushButton:disabled:checked {
            color: #666666;
        }
    )")
        .arg(size)
        .arg(size - 4); // Font size slightly smaller than box
}

QString radioButton() {
    return R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a4a4a,
                stop:0.4 #3a3a3a,
                stop:0.6 #353535,
                stop:1 #2a2a2a);
            color: #FFFFFF;
            border: 2px solid #606060;
            border-radius: 6px;
            padding: 6px 12px;
            font-size: 11px;
            font-weight: bold;
            text-align: left;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a5a5a,
                stop:0.4 #4a4a4a,
                stop:0.6 #454545,
                stop:1 #3a3a3a);
            border: 2px solid #808080;
        }
        QPushButton:checked {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #E0E0E0,
                stop:0.4 #D0D0D0,
                stop:0.6 #C8C8C8,
                stop:1 #B8B8B8);
            color: #333333;
            border: 2px solid #AAAAAA;
        }
    )";
}

} // namespace K4Styles
