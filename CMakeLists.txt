cmake_minimum_required(VERSION 3.16)
project(K4Controller VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia ShaderTools Gui GuiPrivate)

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
    src/ui/smeterwidget.cpp
    src/ui/vfowidget.cpp
    src/ui/bandpopupwidget.cpp
    src/ui/buttonrowpopup.cpp
    src/ui/displaypopupwidget.cpp
    src/ui/optionsdialog.cpp
    src/ui/rightsidepanel.cpp
    src/ui/featuremenubar.cpp
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
    src/ui/smeterwidget.h
    src/ui/vfowidget.h
    src/ui/bandpopupwidget.h
    src/ui/buttonrowpopup.h
    src/ui/displaypopupwidget.h
    src/ui/optionsdialog.h
    src/ui/rightsidepanel.h
    src/ui/featuremenubar.h
    src/hardware/kpoddevice.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

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
    Qt6::GuiPrivate
    ${OPUS_LIBRARIES}
    ${HIDAPI_LIBRARIES}
)

# Compile shaders for QRhi (Metal/DirectX/Vulkan/OpenGL)
qt6_add_shaders(${PROJECT_NAME} "panadapter_shaders"
    BATCHABLE
    PRECOMPILE
    PREFIX "/shaders"
    FILES
        src/dsp/shaders/spectrum.vert
        src/dsp/shaders/spectrum.frag
        src/dsp/shaders/waterfall.vert
        src/dsp/shaders/waterfall.frag
        src/dsp/shaders/overlay.vert
        src/dsp/shaders/overlay.frag
)

# Test executable for hidden QRhiWidget debugging
add_executable(test_hidden_rhi test_hidden_rhi.cpp)
target_link_libraries(test_hidden_rhi PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::GuiPrivate
)
get_target_property(QT_GUI_INCLUDE_DIRS_TEST Qt6::Gui INTERFACE_INCLUDE_DIRECTORIES)
foreach(dir ${QT_GUI_INCLUDE_DIRS_TEST})
    target_include_directories(test_hidden_rhi PRIVATE "${dir}/${Qt6_VERSION}")
    target_include_directories(test_hidden_rhi PRIVATE "${dir}/${Qt6_VERSION}/QtGui")
endforeach()

# Test executable for complex hierarchy
add_executable(test_complex_hierarchy test_complex_hierarchy.cpp)
target_link_libraries(test_complex_hierarchy PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::GuiPrivate
)
foreach(dir ${QT_GUI_INCLUDE_DIRS_TEST})
    target_include_directories(test_complex_hierarchy PRIVATE "${dir}/${Qt6_VERSION}")
    target_include_directories(test_complex_hierarchy PRIVATE "${dir}/${Qt6_VERSION}/QtGui")
endforeach()

# Test executable for popup widgets
add_executable(test_with_popups test_with_popups.cpp)
target_link_libraries(test_with_popups PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::GuiPrivate
)
foreach(dir ${QT_GUI_INCLUDE_DIRS_TEST})
    target_include_directories(test_with_popups PRIVATE "${dir}/${Qt6_VERSION}")
    target_include_directories(test_with_popups PRIVATE "${dir}/${Qt6_VERSION}/QtGui")
endforeach()

# Test executable for real PanadapterRhiWidget
add_executable(test_real_panadapter
    test_real_panadapter.cpp
    src/dsp/panadapter_rhi.cpp
)
target_include_directories(test_real_panadapter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(test_real_panadapter PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::GuiPrivate
)
foreach(dir ${QT_GUI_INCLUDE_DIRS_TEST})
    target_include_directories(test_real_panadapter PRIVATE "${dir}/${Qt6_VERSION}")
    target_include_directories(test_real_panadapter PRIVATE "${dir}/${Qt6_VERSION}/QtGui")
endforeach()

# Add shaders to real panadapter test
qt6_add_shaders(test_real_panadapter "test_panadapter_shaders"
    BATCHABLE
    PRECOMPILE
    PREFIX "/shaders"
    FILES
        src/dsp/shaders/spectrum.vert
        src/dsp/shaders/spectrum.frag
        src/dsp/shaders/waterfall.vert
        src/dsp/shaders/waterfall.frag
        src/dsp/shaders/overlay.vert
        src/dsp/shaders/overlay.frag
)

# Test executable with audio engine
add_executable(test_with_audio
    test_with_audio.cpp
    src/dsp/panadapter_rhi.cpp
    src/audio/audioengine.cpp
    src/audio/opusdecoder.cpp
)
target_include_directories(test_with_audio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPUS_INCLUDE_DIRS}
)
target_link_libraries(test_with_audio PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::GuiPrivate Qt6::Multimedia
    ${OPUS_LIBRARIES}
)
foreach(dir ${QT_GUI_INCLUDE_DIRS_TEST})
    target_include_directories(test_with_audio PRIVATE "${dir}/${Qt6_VERSION}")
    target_include_directories(test_with_audio PRIVATE "${dir}/${Qt6_VERSION}/QtGui")
endforeach()
qt6_add_shaders(test_with_audio "test_audio_shaders"
    BATCHABLE
    PRECOMPILE
    PREFIX "/shaders"
    FILES
        src/dsp/shaders/spectrum.vert
        src/dsp/shaders/spectrum.frag
        src/dsp/shaders/waterfall.vert
        src/dsp/shaders/waterfall.frag
        src/dsp/shaders/overlay.vert
        src/dsp/shaders/overlay.frag
)

# Test executable with menu bar created AFTER panadapter
add_executable(test_menubar_after
    test_menubar_after.cpp
    src/dsp/panadapter_rhi.cpp
)
target_include_directories(test_menubar_after PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(test_menubar_after PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::GuiPrivate
)
foreach(dir ${QT_GUI_INCLUDE_DIRS_TEST})
    target_include_directories(test_menubar_after PRIVATE "${dir}/${Qt6_VERSION}")
    target_include_directories(test_menubar_after PRIVATE "${dir}/${Qt6_VERSION}/QtGui")
endforeach()
qt6_add_shaders(test_menubar_after "test_menubar_after_shaders"
    BATCHABLE
    PRECOMPILE
    PREFIX "/shaders"
    FILES
        src/dsp/shaders/spectrum.vert
        src/dsp/shaders/spectrum.frag
        src/dsp/shaders/waterfall.vert
        src/dsp/shaders/waterfall.frag
        src/dsp/shaders/overlay.vert
        src/dsp/shaders/overlay.frag
)

# Test executable with menu bar (created before panadapter - FAILS)
add_executable(test_with_menubar
    test_with_menubar.cpp
    src/dsp/panadapter_rhi.cpp
)
target_include_directories(test_with_menubar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(test_with_menubar PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::GuiPrivate
)
foreach(dir ${QT_GUI_INCLUDE_DIRS_TEST})
    target_include_directories(test_with_menubar PRIVATE "${dir}/${Qt6_VERSION}")
    target_include_directories(test_with_menubar PRIVATE "${dir}/${Qt6_VERSION}/QtGui")
endforeach()
qt6_add_shaders(test_with_menubar "test_menubar_shaders"
    BATCHABLE
    PRECOMPILE
    PREFIX "/shaders"
    FILES
        src/dsp/shaders/spectrum.vert
        src/dsp/shaders/spectrum.frag
        src/dsp/shaders/waterfall.vert
        src/dsp/shaders/waterfall.frag
        src/dsp/shaders/overlay.vert
        src/dsp/shaders/overlay.frag
)

# Test executable with minimal MainWindow infrastructure
add_executable(test_minimal_mainwindow
    test_minimal_mainwindow.cpp
    src/dsp/panadapter_rhi.cpp
    src/network/tcpclient.cpp
    src/network/protocol.cpp
    src/models/radiostate.cpp
    src/models/menumodel.cpp
    src/audio/audioengine.cpp
    src/audio/opusdecoder.cpp
)
target_include_directories(test_minimal_mainwindow PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPUS_INCLUDE_DIRS}
)
target_link_libraries(test_minimal_mainwindow PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::GuiPrivate Qt6::Multimedia Qt6::Network
    ${OPUS_LIBRARIES}
)
foreach(dir ${QT_GUI_INCLUDE_DIRS_TEST})
    target_include_directories(test_minimal_mainwindow PRIVATE "${dir}/${Qt6_VERSION}")
    target_include_directories(test_minimal_mainwindow PRIVATE "${dir}/${Qt6_VERSION}/QtGui")
endforeach()
qt6_add_shaders(test_minimal_mainwindow "test_minimal_mainwindow_shaders"
    BATCHABLE
    PRECOMPILE
    PREFIX "/shaders"
    FILES
        src/dsp/shaders/spectrum.vert
        src/dsp/shaders/spectrum.frag
        src/dsp/shaders/waterfall.vert
        src/dsp/shaders/waterfall.frag
        src/dsp/shaders/overlay.vert
        src/dsp/shaders/overlay.frag
)

# Test executable with VFOWidget
add_executable(test_with_vfo
    test_with_vfo.cpp
    src/dsp/panadapter_rhi.cpp
    src/dsp/minipan_rhi.cpp
    src/ui/vfowidget.cpp
    src/ui/smeterwidget.cpp
)
target_include_directories(test_with_vfo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(test_with_vfo PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::GuiPrivate
)
foreach(dir ${QT_GUI_INCLUDE_DIRS_TEST})
    target_include_directories(test_with_vfo PRIVATE "${dir}/${Qt6_VERSION}")
    target_include_directories(test_with_vfo PRIVATE "${dir}/${Qt6_VERSION}/QtGui")
endforeach()
qt6_add_shaders(test_with_vfo "test_vfo_shaders"
    BATCHABLE
    PRECOMPILE
    PREFIX "/shaders"
    FILES
        src/dsp/shaders/spectrum.vert
        src/dsp/shaders/spectrum.frag
        src/dsp/shaders/waterfall.vert
        src/dsp/shaders/waterfall.frag
        src/dsp/shaders/overlay.vert
        src/dsp/shaders/overlay.frag
)
