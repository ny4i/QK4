cmake_minimum_required(VERSION 3.16)
project(K4Controller VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia)

# Homebrew paths for macOS
if(APPLE)
    set(OPUS_PREFIX "/opt/homebrew/opt/opus")
    set(OPUS_INCLUDE_DIRS "${OPUS_PREFIX}/include")
    set(OPUS_LIBRARIES "${OPUS_PREFIX}/lib/libopus.dylib")

    set(HIDAPI_PREFIX "/opt/homebrew/opt/hidapi")
    set(HIDAPI_INCLUDE_DIRS "${HIDAPI_PREFIX}/include")
    set(HIDAPI_LIBRARIES "${HIDAPI_PREFIX}/lib/libhidapi.dylib")
else()
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
    src/dsp/spectrum.cpp
    src/dsp/waterfall.cpp
    src/dsp/panadapter.cpp
    src/dsp/minipanwidget.cpp
    src/settings/radiosettings.cpp
    src/models/radiostate.cpp
    src/models/menumodel.cpp
    src/ui/radiomanagerdialog.cpp
    src/ui/dualcontrolbutton.cpp
    src/ui/sidecontrolpanel.cpp
    src/ui/bottommenubar.cpp
    src/ui/menuoverlay.cpp
    src/ui/smeterwidget.cpp
    src/ui/vfowidget.cpp
    src/ui/bandpopupwidget.cpp
    src/ui/optionsdialog.cpp
    src/ui/rightsidepanel.cpp
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
    src/dsp/spectrum.h
    src/dsp/waterfall.h
    src/dsp/panadapter.h
    src/dsp/minipanwidget.h
    src/settings/radiosettings.h
    src/models/radiostate.h
    src/models/menumodel.h
    src/ui/radiomanagerdialog.h
    src/ui/dualcontrolbutton.h
    src/ui/sidecontrolpanel.h
    src/ui/bottommenubar.h
    src/ui/menuoverlay.h
    src/ui/smeterwidget.h
    src/ui/vfowidget.h
    src/ui/bandpopupwidget.h
    src/ui/optionsdialog.h
    src/ui/rightsidepanel.h
    src/hardware/kpoddevice.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPUS_INCLUDE_DIRS}
    ${HIDAPI_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    ${OPUS_LIBRARIES}
    ${HIDAPI_LIBRARIES}
)
