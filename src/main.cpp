#include <QApplication>
#include <QDebug>
#include <QSysInfo>
#include <QGuiApplication>
#include <QFontDatabase>
#include <rhi/qrhi.h>
#ifdef Q_OS_MACOS
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <cstdlib>
#include <QDir>
#include <QFileInfo>
#endif
#include "mainwindow.h"

// Load embedded fonts and set application defaults
void setupFonts() {
    // Load Inter font family (screen-optimized sans-serif)
    int interRegular = QFontDatabase::addApplicationFont(":/fonts/Inter-Regular.ttf");
    int interMedium = QFontDatabase::addApplicationFont(":/fonts/Inter-Medium.ttf");
    int interSemiBold = QFontDatabase::addApplicationFont(":/fonts/Inter-SemiBold.ttf");
    int interBold = QFontDatabase::addApplicationFont(":/fonts/Inter-Bold.ttf");

    // Load JetBrains Mono (crisp monospace for frequency/data display)
    int monoRegular = QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Regular.ttf");
    int monoMedium = QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Medium.ttf");
    int monoBold = QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Bold.ttf");

    // Verify fonts loaded
    if (interRegular < 0 || interMedium < 0) {
        qWarning() << "Failed to load Inter font - using system default";
    } else {
        qDebug() << "Loaded Inter font family";
    }

    if (monoRegular < 0) {
        qWarning() << "Failed to load JetBrains Mono - using system monospace";
    } else {
        qDebug() << "Loaded JetBrains Mono font family";
    }

    // Set Inter Medium as the default application font (crisper than Regular)
    QFont defaultFont("Inter", 11);
    defaultFont.setWeight(QFont::Medium);
    defaultFont.setHintingPreference(QFont::PreferFullHinting);
    defaultFont.setStyleStrategy(QFont::PreferAntialias);
    QApplication::setFont(defaultFont);

    qDebug() << "Default font set to:" << defaultFont.family() << defaultFont.pointSize() << "pt";
}

int main(int argc, char *argv[]) {
#ifdef Q_OS_MACOS
    // Enable OpenSSL for TLS/PSK support
    // Qt's OpenSSL backend dynamically loads libssl/libcrypto at runtime
    // Check bundled location first (inside .app bundle), then Homebrew locations

    // Get the path to the executable to find the Frameworks folder
    QString execPath = QString::fromLocal8Bit(argv[0]);
    QString bundledFrameworks;
    if (execPath.contains(".app/Contents/MacOS/")) {
        bundledFrameworks = QFileInfo(execPath).absolutePath() + "/../Frameworks";
    }

    QStringList opensslPaths;
    if (!bundledFrameworks.isEmpty()) {
        opensslPaths << bundledFrameworks; // Check bundled first
    }
    opensslPaths << "/opt/homebrew/opt/openssl@3/lib" // Homebrew on Apple Silicon
                 << "/usr/local/opt/openssl@3/lib"    // Homebrew on Intel Mac
                 << "/opt/homebrew/opt/openssl/lib"   // Homebrew openssl (latest)
                 << "/usr/local/opt/openssl/lib";     // Homebrew openssl on Intel

    QString currentPath = QString::fromLocal8Bit(qgetenv("DYLD_LIBRARY_PATH"));
    bool foundOpenSSL = false;

    for (const QString &opensslPath : opensslPaths) {
        // Check if libssl exists in this location
        if (QFileInfo::exists(opensslPath + "/libssl.3.dylib") || QFileInfo::exists(opensslPath + "/libssl.dylib")) {
            if (!currentPath.contains(opensslPath)) {
                QString newPath = currentPath.isEmpty() ? opensslPath : QString("%1:%2").arg(opensslPath, currentPath);
                qputenv("DYLD_LIBRARY_PATH", newPath.toLocal8Bit());
                qDebug() << "Added OpenSSL to library path for TLS/PSK support:" << opensslPath;
            }
            foundOpenSSL = true;
            break;
        }
    }

    if (!foundOpenSSL) {
        qDebug() << "OpenSSL not found - TLS/PSK will not be available";
    }
#endif

    // Enable HiDPI scaling for crisp rendering on Retina/4K displays
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("K4Controller");
    app.setApplicationVersion(K4CONTROLLER_VERSION);
    app.setOrganizationName("AI5QK");
    app.setOrganizationDomain("ai5qk.com");

    // Load embedded fonts (Inter + JetBrains Mono)
    setupFonts();

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
