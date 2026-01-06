cmake_minimum_required(VERSION 3.16)
project(K4Controller VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Core Qt components (GuiPrivate handled separately for cross-platform compatibility)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia ShaderTools Gui)

# GuiPrivate is needed for QRhi access but may not have CMake config on all platforms
if(APPLE)
    # macOS Homebrew Qt includes GuiPrivate CMake config
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
    set(QT_GUI_PRIVATE_FOUND TRUE)
elseif(WIN32)
    # Windows aqtinstall binary packages may not include GuiPrivate CMake config
    # Try to find it, but fall back to manual include paths if not found
    find_package(Qt6 COMPONENTS GuiPrivate QUIET)
    if(Qt6GuiPrivate_FOUND)
        set(QT_GUI_PRIVATE_FOUND TRUE)
    else()
        message(STATUS "Qt6GuiPrivate CMake config not found, using manual include paths")
        set(QT_GUI_PRIVATE_FOUND FALSE)
    endif()
else()
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
    set(QT_GUI_PRIVATE_FOUND TRUE)
endif()

# Platform-specific dependency handling
if(APPLE)
    # macOS with Homebrew
    set(OPUS_PREFIX "/opt/homebrew/opt/opus")
    set(OPUS_INCLUDE_DIRS "${OPUS_PREFIX}/include")
    set(OPUS_LIBRARIES "${OPUS_PREFIX}/lib/libopus.dylib")

    set(HIDAPI_PREFIX "/opt/homebrew/opt/hidapi")
    set(HIDAPI_INCLUDE_DIRS "${HIDAPI_PREFIX}/include")
    set(HIDAPI_LIBRARIES "${HIDAPI_PREFIX}/lib/libhidapi.dylib")
elseif(WIN32)
    # Windows with vcpkg
    find_package(Opus CONFIG REQUIRED)
    find_package(hidapi CONFIG REQUIRED)
    set(OPUS_INCLUDE_DIRS "")
    set(HIDAPI_INCLUDE_DIRS "")
    set(OPUS_LIBRARIES Opus::opus)
    set(HIDAPI_LIBRARIES hidapi::hidapi)
else()
    # Linux with pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(OPUS REQUIRED opus)
    pkg_check_modules(HIDAPI REQUIRED hidapi-libusb)
endif()
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/network/tcpclient.cpp
    src/network/protocol.cpp
    src/network/kpa1500client.cpp
    src/audio/audioengine.cpp
    src/audio/opusdecoder.cpp
    src/audio/opusencoder.cpp
    src/dsp/panadapter_rhi.cpp
    src/dsp/minipan_rhi.cpp
    src/settings/radiosettings.cpp
    src/models/radiostate.cpp
    src/models/menumodel.cpp
    src/ui/radiomanagerdialog.cpp
    src/ui/dualcontrolbutton.cpp
    src/ui/sidecontrolpanel.cpp
    src/ui/bottommenubar.cpp
    src/ui/menuoverlay.cpp
    src/ui/micmeterwidget.cpp
    src/ui/vfowidget.cpp
    src/ui/bandpopupwidget.cpp
    src/ui/buttonrowpopup.cpp
    src/ui/displaypopupwidget.cpp
    src/ui/optionsdialog.cpp
    src/ui/rightsidepanel.cpp
    src/ui/featuremenubar.cpp
    src/ui/modepopupwidget.cpp
    src/ui/txmeterwidget.cpp
    src/hardware/kpoddevice.cpp
)

set(HEADERS
    src/mainwindow.h
    src/network/tcpclient.h
    src/network/protocol.h
    src/network/kpa1500client.h
    src/audio/audioengine.h
    src/audio/opusdecoder.h
    src/audio/opusencoder.h
    src/dsp/panadapter_rhi.h
    src/dsp/minipan_rhi.h
    src/settings/radiosettings.h
    src/models/radiostate.h
    src/models/menumodel.h
    src/ui/radiomanagerdialog.h
    src/ui/dualcontrolbutton.h
    src/ui/sidecontrolpanel.h
    src/ui/bottommenubar.h
    src/ui/menuoverlay.h
    src/ui/micmeterwidget.h
    src/ui/vfowidget.h
    src/ui/bandpopupwidget.h
    src/ui/buttonrowpopup.h
    src/ui/displaypopupwidget.h
    src/ui/optionsdialog.h
    src/ui/rightsidepanel.h
    src/ui/featuremenubar.h
    src/ui/modepopupwidget.h
    src/ui/txmeterwidget.h
    src/hardware/kpoddevice.h
)

# On macOS, create an app bundle
if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.elecraft.k4controller")
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${HEADERS})

    # Set the app bundle output directory
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist.in"
    )
elseif(WIN32)
    # WIN32 flag creates a GUI app without console window
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPUS_INCLUDE_DIRS}
    ${HIDAPI_INCLUDE_DIRS}
)

# Add Qt private headers for QRhi access
get_target_property(QT_GUI_INCLUDE_DIRS Qt6::Gui INTERFACE_INCLUDE_DIRECTORIES)
foreach(dir ${QT_GUI_INCLUDE_DIRS})
    target_include_directories(${PROJECT_NAME} PRIVATE "${dir}/${Qt6_VERSION}")
    target_include_directories(${PROJECT_NAME} PRIVATE "${dir}/${Qt6_VERSION}/QtGui")
endforeach()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::Gui
    ${OPUS_LIBRARIES}
    ${HIDAPI_LIBRARIES}
)

# Link GuiPrivate if found via CMake, otherwise private headers are included manually
if(QT_GUI_PRIVATE_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::GuiPrivate)
endif()

# macOS frameworks for KPOD hotplug detection (IOKit)
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework IOKit"
        "-framework CoreFoundation"
    )
endif()

# Compile shaders for QRhi (Metal/DirectX/Vulkan/OpenGL)
qt6_add_shaders(${PROJECT_NAME} "panadapter_shaders"
    BATCHABLE
    PRECOMPILE
    PREFIX "/shaders"
    FILES
        src/dsp/shaders/spectrum.vert
        src/dsp/shaders/spectrum.frag
        src/dsp/shaders/spectrum_blue.vert
        src/dsp/shaders/spectrum_blue.frag
        src/dsp/shaders/spectrum_blue_amp.frag
        src/dsp/shaders/waterfall.vert
        src/dsp/shaders/waterfall.frag
        src/dsp/shaders/overlay.vert
        src/dsp/shaders/overlay.frag
)

# =============================================================================
# Deployment / Bundling - Create self-contained distributable app
# =============================================================================

if(APPLE)
    # Find macdeployqt
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${Qt6_DIR}/../../../bin")

    # OpenSSL locations (check both Apple Silicon and Intel paths)
    set(OPENSSL_SEARCH_PATHS
        "/opt/homebrew/opt/openssl@3/lib"   # Apple Silicon
        "/usr/local/opt/openssl@3/lib"      # Intel Mac
        "/opt/homebrew/opt/openssl/lib"
        "/usr/local/opt/openssl/lib"
    )

    foreach(OPENSSL_PATH ${OPENSSL_SEARCH_PATHS})
        if(EXISTS "${OPENSSL_PATH}/libssl.3.dylib")
            set(OPENSSL_LIB_DIR ${OPENSSL_PATH})
            break()
        endif()
    endforeach()

    if(OPENSSL_LIB_DIR)
        message(STATUS "Found OpenSSL at: ${OPENSSL_LIB_DIR}")
    else()
        message(WARNING "OpenSSL not found - TLS/PSK will not be available in bundled app")
    endif()

    # Get the real (resolved) path for OpenSSL - Homebrew uses symlinks
    execute_process(
        COMMAND realpath "${OPENSSL_LIB_DIR}"
        OUTPUT_VARIABLE OPENSSL_REAL_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "OpenSSL real path: ${OPENSSL_REAL_DIR}")

    # Custom target to create distributable app bundle
    add_custom_target(deploy
        COMMAND ${MACDEPLOYQT_EXECUTABLE} "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app" -always-overwrite
        # Copy OpenSSL libraries
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_LIB_DIR}/libssl.3.dylib"
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_LIB_DIR}/libcrypto.3.dylib"
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks/"
        # Copy Opus library
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPUS_LIBRARIES}"
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks/"
        # Copy HIDAPI library
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${HIDAPI_LIBRARIES}"
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks/"
        # Fix OpenSSL library paths - use real Cellar path for -change
        COMMAND install_name_tool -id "@executable_path/../Frameworks/libssl.3.dylib"
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks/libssl.3.dylib"
        COMMAND install_name_tool -id "@executable_path/../Frameworks/libcrypto.3.dylib"
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks/libcrypto.3.dylib"
        COMMAND install_name_tool -change "${OPENSSL_REAL_DIR}/libcrypto.3.dylib"
            "@executable_path/../Frameworks/libcrypto.3.dylib"
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks/libssl.3.dylib"
        # Fix Opus library path
        COMMAND install_name_tool -id "@executable_path/../Frameworks/libopus.dylib"
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks/libopus.dylib" || true
        COMMAND install_name_tool -id "@executable_path/../Frameworks/libopus.0.dylib"
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks/libopus.0.dylib" || true
        # Fix HIDAPI library path
        COMMAND install_name_tool -id "@executable_path/../Frameworks/libhidapi.dylib"
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks/libhidapi.dylib" || true
        # Re-sign the app bundle (required after modifying libraries)
        # Uses ad-hoc signing by default; set CODESIGN_IDENTITY for Developer ID
        COMMAND codesign --force --deep --sign "$ENV{CODESIGN_IDENTITY}"
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app" ||
            codesign --force --deep --sign - "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app"
        COMMENT "Creating deployable app bundle with all dependencies..."
        DEPENDS ${PROJECT_NAME}
    )

elseif(WIN32)
    # Find windeployqt
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin")

    # Custom target to create distributable package
    add_custom_target(deploy
        COMMAND ${WINDEPLOYQT_EXECUTABLE} --release --no-translations
            "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Deploying Qt dependencies for Windows..."
        DEPENDS ${PROJECT_NAME}
    )
endif()

