#include <QApplication>
#include <QDebug>
#include <QSysInfo>
#include <QGuiApplication>
#include <rhi/qrhi.h>
#ifdef Q_OS_MACOS
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#endif
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("K4Controller");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("K4Radio");

    // Debug: System info
    qDebug() << "=== K4Controller Startup ===";
    qDebug() << "Platform:" << QSysInfo::prettyProductName();
    qDebug() << "Kernel:" << QSysInfo::kernelType() << QSysInfo::kernelVersion();
    qDebug() << "CPU:" << QSysInfo::currentCpuArchitecture();
    qDebug() << "Qt version:" << QT_VERSION_STR;

    // Debug: Test QRhi Metal support
#ifdef Q_OS_MACOS
    qDebug() << "=== Testing QRhi Metal Backend ===";

    // Try to create a Metal QRhi to see if it works
    QRhiMetalInitParams metalParams;
    QRhi::Flags flags;
    std::unique_ptr<QRhi> testRhi(QRhi::create(QRhi::Metal, &metalParams, flags));
    if (testRhi) {
        qDebug() << "✓ QRhi Metal backend created successfully!";
        qDebug() << "  Backend:" << testRhi->backendName();
        qDebug() << "  Device:" << testRhi->driverInfo().deviceName;
    } else {
        qWarning() << "✗ QRhi Metal backend FAILED to create!";
        qWarning() << "  This may indicate a Metal framework or driver issue.";
    }
    testRhi.reset(); // Clean up test instance

    // Check platform RHI capability
    auto *pi = QGuiApplicationPrivate::platformIntegration();
    if (pi) {
        bool rhiCapable = pi->hasCapability(QPlatformIntegration::RhiBasedRendering);
        qDebug() << "RhiBasedRendering capability:" << rhiCapable;
        if (!rhiCapable) {
            qWarning() << "!!! Platform does NOT support RhiBasedRendering !!!";
            qWarning() << "    QRhiWidget will fail to get QRhi from window backing store.";
        }
    } else {
        qWarning() << "Could not get platform integration!";
    }
#endif

    MainWindow window;
    window.show();

    return app.exec();
}
