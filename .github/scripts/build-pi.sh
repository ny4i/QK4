#!/bin/bash
set -ex

# =============================================================================
# K4Controller Raspberry Pi Build Script
# =============================================================================
# This script cross-compiles K4Controller for Raspberry Pi (ARM64) using
# Debian multiarch and creates a portable tarball with all required
# libraries bundled.
#
# Target: Raspberry Pi 4/5 running Debian Trixie or compatible
# Qt Version: 6.8+ (from Debian Trixie repos)
# Build method: x86_64 cross-compilation with aarch64-linux-gnu toolchain
# =============================================================================

# Enable arm64 multiarch
dpkg --add-architecture arm64
apt-get update

# Native host tools (x86, full speed)
apt-get install -y cmake g++-aarch64-linux-gnu file patchelf pkg-config

# Qt6 native host tools (moc, rcc, uic, qsb)
apt-get install -y qt6-base-dev-tools qt6-shadertools-dev

# Qt6 arm64 development libraries
apt-get install -y \
  qt6-base-dev:arm64 qt6-base-private-dev:arm64 \
  qt6-multimedia-dev:arm64 qt6-shadertools-dev:arm64 \
  qt6-serialport-dev:arm64

# System arm64 libraries
apt-get install -y \
  libopus-dev:arm64 libhidapi-dev:arm64 libssl-dev:arm64 \
  libasound2-dev:arm64 libpulse-dev:arm64

# Build with cross-compilation toolchain
export PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig
export PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig

cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64-linux.cmake
cmake --build build -j$(nproc)

# Verify output is actually ARM64
file build/K4Controller | grep -q "aarch64" || { echo "ERROR: Binary is not ARM64!"; exit 1; }

# =============================================================================
# Create portable bundle
# =============================================================================
DIST=dist/K4Controller
mkdir -p $DIST/lib
mkdir -p $DIST/plugins

cp build/K4Controller $DIST/

# -----------------------------------------------------------------------------
# Qt Libraries
# -----------------------------------------------------------------------------
for lib in \
  libQt6Core.so.6 \
  libQt6Gui.so.6 \
  libQt6Widgets.so.6 \
  libQt6Network.so.6 \
  libQt6Multimedia.so.6 \
  libQt6SerialPort.so.6 \
  libQt6ShaderTools.so.6 \
  libQt6OpenGL.so.6 \
  libQt6OpenGLWidgets.so.6 \
  libQt6DBus.so.6 \
  libQt6XcbQpa.so.6 \
  libQt6Svg.so.6; do
  cp -L /usr/lib/aarch64-linux-gnu/$lib $DIST/lib/ 2>/dev/null || true
done

# -----------------------------------------------------------------------------
# Qt Plugins
# -----------------------------------------------------------------------------
# Platform plugins (X11/XCB support)
cp -r /usr/lib/aarch64-linux-gnu/qt6/plugins/platforms $DIST/plugins/
cp -r /usr/lib/aarch64-linux-gnu/qt6/plugins/xcbglintegrations $DIST/plugins/ 2>/dev/null || true

# Multimedia plugins (audio/video backends)
cp -r /usr/lib/aarch64-linux-gnu/qt6/plugins/multimedia $DIST/plugins/ 2>/dev/null || true

# TLS/SSL plugins (for HTTPS connections)
cp -r /usr/lib/aarch64-linux-gnu/qt6/plugins/tls $DIST/plugins/ 2>/dev/null || true

# Image format plugins
cp -r /usr/lib/aarch64-linux-gnu/qt6/plugins/imageformats $DIST/plugins/ 2>/dev/null || true

# Icon engine plugins
cp -r /usr/lib/aarch64-linux-gnu/qt6/plugins/iconengines $DIST/plugins/ 2>/dev/null || true

# -----------------------------------------------------------------------------
# System Libraries
# -----------------------------------------------------------------------------
for lib in \
  libopus.so.0 \
  libhidapi-hidraw.so.0 \
  libhidapi-libusb.so.0 \
  libssl.so.3 \
  libcrypto.so.3; do
  cp -L /usr/lib/aarch64-linux-gnu/$lib $DIST/lib/ 2>/dev/null || true
done

# PulseAudio/ALSA libraries for audio
for lib in \
  libpulse.so.0 \
  libpulse-simple.so.0 \
  libasound.so.2; do
  cp -L /usr/lib/aarch64-linux-gnu/$lib $DIST/lib/ 2>/dev/null || true
done

# -----------------------------------------------------------------------------
# Set rpath so binary finds bundled libs
# -----------------------------------------------------------------------------
patchelf --set-rpath '$ORIGIN/lib' $DIST/K4Controller

# Also patch any bundled .so files that might need it
find $DIST/lib -name "*.so*" -type f -exec patchelf --set-rpath '$ORIGIN' {} \; 2>/dev/null || true
find $DIST/plugins -name "*.so*" -type f -exec patchelf --set-rpath '$ORIGIN/../../lib' {} \; 2>/dev/null || true

# -----------------------------------------------------------------------------
# Create launcher script
# -----------------------------------------------------------------------------
cat > $DIST/run.sh << 'LAUNCHER'
#!/bin/bash
# K4Controller Launcher for Raspberry Pi
# This script sets up the environment to use bundled libraries

DIR="$(cd "$(dirname "$0")" && pwd)"

# Use bundled libraries
export LD_LIBRARY_PATH="$DIR/lib:$LD_LIBRARY_PATH"

# Use bundled Qt plugins
export QT_PLUGIN_PATH="$DIR/plugins"

# Force XCB platform (X11) if Wayland gives issues
# Uncomment the next line if you have display problems:
# export QT_QPA_PLATFORM=xcb

# Launch the application
exec "$DIR/K4Controller" "$@"
LAUNCHER
chmod +x $DIST/run.sh

# -----------------------------------------------------------------------------
# Package
# -----------------------------------------------------------------------------
cd dist
tar czf ../K4Controller-raspberry-pi-arm64.tar.gz K4Controller/

echo "============================================="
echo "Build complete!"
echo "Output: K4Controller-raspberry-pi-arm64.tar.gz"
echo "============================================="
