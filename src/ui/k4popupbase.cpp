#include "k4popupbase.h"
#include "k4styles.h"
#include <QApplication>
#include <QHideEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>

K4PopupBase::K4PopupBase(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);
}

QMargins K4PopupBase::contentMargins() const {
    int sm = K4Styles::Dimensions::ShadowMargin;
    int cm = K4Styles::Dimensions::PopupContentMargin;
    int bsh = K4Styles::Dimensions::PopupBottomStripHeight;
    int th = K4Styles::Dimensions::PopupTriangleHeight;

    return QMargins(sm + cm,           // left
                    sm + cm,           // top
                    sm + cm,           // right
                    sm + cm + bsh + th // bottom (includes strip and triangle space)
    );
}

void K4PopupBase::initPopup() {
    QSize cs = contentSize();
    int totalWidth = cs.width() + 2 * K4Styles::Dimensions::ShadowMargin;
    int totalHeight = cs.height() + 2 * K4Styles::Dimensions::ShadowMargin;
    setFixedSize(totalWidth, totalHeight);
}

QRect K4PopupBase::contentRect() const {
    QSize cs = contentSize();
    // Content height for drawing excludes the triangle (triangle is drawn below content)
    int drawHeight = cs.height() - K4Styles::Dimensions::PopupTriangleHeight;
    return QRect(K4Styles::Dimensions::ShadowMargin, K4Styles::Dimensions::ShadowMargin, cs.width(), drawHeight);
}

void K4PopupBase::showAboveButton(QWidget *triggerButton) {
    showAboveWidget(triggerButton, triggerButton);
}

void K4PopupBase::showAboveWidget(QWidget *referenceWidget, QWidget *arrowTarget) {
    if (!referenceWidget)
        return;

    if (!arrowTarget)
        arrowTarget = referenceWidget;

    // Ensure geometry is realized before positioning
    adjustSize();

    // Get the reference widget's parent (typically button bar) for centering
    QWidget *parentBar = referenceWidget->parentWidget();
    if (!parentBar)
        parentBar = referenceWidget;

    // Get global positions
    QPoint barGlobal = parentBar->mapToGlobal(QPoint(0, 0));
    QPoint refGlobal = referenceWidget->mapToGlobal(QPoint(0, 0));
    QPoint arrowGlobal = arrowTarget->mapToGlobal(QPoint(0, 0));

    int barCenterX = barGlobal.x() + parentBar->width() / 2;
    int arrowCenterX = arrowGlobal.x() + arrowTarget->width() / 2;

    // Content dimensions
    QSize cs = contentSize();

    // Center content area above the parent bar (account for shadow margin offset)
    int popupX = barCenterX - cs.width() / 2 - K4Styles::Dimensions::ShadowMargin;

    // Position popup so its bottom edge (including shadow margin) is at the top of the reference widget
    int popupY = refGlobal.y() - height();

    // Calculate triangle offset to point at the arrow target
    int contentCenterX = popupX + K4Styles::Dimensions::ShadowMargin + cs.width() / 2;
    m_triangleXOffset = arrowCenterX - contentCenterX;

    // Ensure popup stays on screen
    QRect screenGeom = QApplication::primaryScreen()->availableGeometry();

    // Check left edge
    if (popupX < screenGeom.left() - K4Styles::Dimensions::ShadowMargin) {
        popupX = screenGeom.left() - K4Styles::Dimensions::ShadowMargin;
        // Recalculate triangle offset with new popup position
        contentCenterX = popupX + K4Styles::Dimensions::ShadowMargin + cs.width() / 2;
        m_triangleXOffset = arrowCenterX - contentCenterX;
    }
    // Check right edge
    else if (popupX + width() > screenGeom.right() + K4Styles::Dimensions::ShadowMargin) {
        popupX = screenGeom.right() + K4Styles::Dimensions::ShadowMargin - width();
        // Recalculate triangle offset with new popup position
        contentCenterX = popupX + K4Styles::Dimensions::ShadowMargin + cs.width() / 2;
        m_triangleXOffset = arrowCenterX - contentCenterX;
    }

    // Check top edge (if popup would go off top of screen)
    if (popupY < screenGeom.top() - K4Styles::Dimensions::ShadowMargin) {
        popupY = screenGeom.top() - K4Styles::Dimensions::ShadowMargin;
    }

    move(popupX, popupY);
    show();
    raise();
    setFocus();
    update();
}

void K4PopupBase::hidePopup() {
    hide();
    // closed() signal is emitted by hideEvent()
}

void K4PopupBase::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    emit closed();
}

void K4PopupBase::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        hidePopup();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void K4PopupBase::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Get content rect for drawing
    QRect cr = contentRect();

    // Draw drop shadow
    K4Styles::drawDropShadow(painter, cr, 8);

    // Main popup background
    painter.setBrush(QColor(K4Styles::Colors::PopupBackground));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(cr, 8, 8);

    // Gray bottom strip (inside the content rect, at bottom)
    int stripHeight = K4Styles::Dimensions::PopupBottomStripHeight;
    QRect stripRect(cr.left(), cr.bottom() - stripHeight + 1, cr.width(), stripHeight);
    painter.fillRect(stripRect, QColor(K4Styles::Colors::IndicatorStrip));

    // Triangle pointing down from center of bottom strip
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(K4Styles::Colors::IndicatorStrip));

    int triangleWidth = K4Styles::Dimensions::PopupTriangleWidth;
    int triangleHeight = K4Styles::Dimensions::PopupTriangleHeight;
    int triangleX = cr.center().x() + m_triangleXOffset;
    int triangleTop = cr.bottom() + 1;

    QPainterPath triangle;
    triangle.moveTo(triangleX - triangleWidth / 2, triangleTop);
    triangle.lineTo(triangleX + triangleWidth / 2, triangleTop);
    triangle.lineTo(triangleX, triangleTop + triangleHeight);
    triangle.closeSubpath();
    painter.drawPath(triangle);

    // Allow subclass to draw additional content
    paintContent(painter, cr);
}

void K4PopupBase::paintContent(QPainter &painter, const QRect &contentRect) {
    // Default implementation does nothing
    // Subclasses can override for custom painting
    Q_UNUSED(painter)
    Q_UNUSED(contentRect)
}
