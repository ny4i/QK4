/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "onConnectClicked",
        "",
        "onDisconnectClicked",
        "onStateChanged",
        "TcpClient::ConnectionState",
        "state",
        "onError",
        "error",
        "onAuthenticated",
        "onAuthenticationFailed",
        "onCatResponse",
        "response",
        "onFrequencyChanged",
        "freq",
        "onFrequencyBChanged",
        "onModeChanged",
        "RadioState::Mode",
        "mode",
        "onModeBChanged",
        "onSMeterChanged",
        "value",
        "onSMeterBChanged",
        "onBandwidthChanged",
        "bw",
        "onBandwidthBChanged",
        "onRfPowerChanged",
        "watts",
        "isQrp",
        "onSupplyVoltageChanged",
        "volts",
        "onSupplyCurrentChanged",
        "amps",
        "onSwrChanged",
        "swr",
        "onSplitChanged",
        "enabled",
        "onAntennaChanged",
        "txAnt",
        "rxAntMain",
        "rxAntSub",
        "onAntennaNameChanged",
        "index",
        "name",
        "onVoxChanged",
        "onRitXitChanged",
        "ritEnabled",
        "xitEnabled",
        "offset",
        "onMessageBankChanged",
        "bank",
        "onProcessingChanged",
        "onProcessingChangedB",
        "onSpectrumData",
        "receiver",
        "data",
        "centerFreq",
        "sampleRate",
        "noiseFloor",
        "onMiniSpectrumData",
        "onAudioData",
        "opusData",
        "showRadioManager",
        "connectToRadio",
        "RadioEntry",
        "radio",
        "updateDateTime",
        "showMenuOverlay",
        "onMenuValueChangeRequested",
        "menuId",
        "action",
        "showBandPopup",
        "onBandSelected",
        "bandName",
        "updateBandSelection",
        "bandNum",
        "onKpodEncoderRotated",
        "ticks",
        "onKpodRockerChanged",
        "position",
        "onKpodPollError",
        "onKpodEnabledChanged",
        "onKpa1500Connected",
        "onKpa1500Disconnected",
        "onKpa1500Error",
        "onKpa1500EnabledChanged",
        "onKpa1500SettingsChanged",
        "updateKpa1500Status"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onConnectClicked'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDisconnectClicked'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStateChanged'
        QtMocHelpers::SlotData<void(TcpClient::ConnectionState)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 5, 6 },
        }}),
        // Slot 'onError'
        QtMocHelpers::SlotData<void(const QString &)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Slot 'onAuthenticated'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAuthenticationFailed'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCatResponse'
        QtMocHelpers::SlotData<void(const QString &)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 12 },
        }}),
        // Slot 'onFrequencyChanged'
        QtMocHelpers::SlotData<void(quint64)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::ULongLong, 14 },
        }}),
        // Slot 'onFrequencyBChanged'
        QtMocHelpers::SlotData<void(quint64)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::ULongLong, 14 },
        }}),
        // Slot 'onModeChanged'
        QtMocHelpers::SlotData<void(RadioState::Mode)>(16, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 17, 18 },
        }}),
        // Slot 'onModeBChanged'
        QtMocHelpers::SlotData<void(RadioState::Mode)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 17, 18 },
        }}),
        // Slot 'onSMeterChanged'
        QtMocHelpers::SlotData<void(double)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 21 },
        }}),
        // Slot 'onSMeterBChanged'
        QtMocHelpers::SlotData<void(double)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 21 },
        }}),
        // Slot 'onBandwidthChanged'
        QtMocHelpers::SlotData<void(int)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 24 },
        }}),
        // Slot 'onBandwidthBChanged'
        QtMocHelpers::SlotData<void(int)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 24 },
        }}),
        // Slot 'onRfPowerChanged'
        QtMocHelpers::SlotData<void(double, bool)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 27 }, { QMetaType::Bool, 28 },
        }}),
        // Slot 'onSupplyVoltageChanged'
        QtMocHelpers::SlotData<void(double)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 30 },
        }}),
        // Slot 'onSupplyCurrentChanged'
        QtMocHelpers::SlotData<void(double)>(31, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 32 },
        }}),
        // Slot 'onSwrChanged'
        QtMocHelpers::SlotData<void(double)>(33, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 34 },
        }}),
        // Slot 'onSplitChanged'
        QtMocHelpers::SlotData<void(bool)>(35, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 36 },
        }}),
        // Slot 'onAntennaChanged'
        QtMocHelpers::SlotData<void(int, int, int)>(37, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 38 }, { QMetaType::Int, 39 }, { QMetaType::Int, 40 },
        }}),
        // Slot 'onAntennaNameChanged'
        QtMocHelpers::SlotData<void(int, const QString &)>(41, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 42 }, { QMetaType::QString, 43 },
        }}),
        // Slot 'onVoxChanged'
        QtMocHelpers::SlotData<void(bool)>(44, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 36 },
        }}),
        // Slot 'onRitXitChanged'
        QtMocHelpers::SlotData<void(bool, bool, int)>(45, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 46 }, { QMetaType::Bool, 47 }, { QMetaType::Int, 48 },
        }}),
        // Slot 'onMessageBankChanged'
        QtMocHelpers::SlotData<void(int)>(49, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 50 },
        }}),
        // Slot 'onProcessingChanged'
        QtMocHelpers::SlotData<void()>(51, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onProcessingChangedB'
        QtMocHelpers::SlotData<void()>(52, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSpectrumData'
        QtMocHelpers::SlotData<void(int, const QByteArray &, qint64, qint32, float)>(53, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 54 }, { QMetaType::QByteArray, 55 }, { QMetaType::LongLong, 56 }, { QMetaType::Int, 57 },
            { QMetaType::Float, 58 },
        }}),
        // Slot 'onMiniSpectrumData'
        QtMocHelpers::SlotData<void(int, const QByteArray &)>(59, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 54 }, { QMetaType::QByteArray, 55 },
        }}),
        // Slot 'onAudioData'
        QtMocHelpers::SlotData<void(const QByteArray &)>(60, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QByteArray, 61 },
        }}),
        // Slot 'showRadioManager'
        QtMocHelpers::SlotData<void()>(62, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'connectToRadio'
        QtMocHelpers::SlotData<void(const RadioEntry &)>(63, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 64, 65 },
        }}),
        // Slot 'updateDateTime'
        QtMocHelpers::SlotData<void()>(66, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'showMenuOverlay'
        QtMocHelpers::SlotData<void()>(67, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onMenuValueChangeRequested'
        QtMocHelpers::SlotData<void(int, const QString &)>(68, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 69 }, { QMetaType::QString, 70 },
        }}),
        // Slot 'showBandPopup'
        QtMocHelpers::SlotData<void()>(71, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onBandSelected'
        QtMocHelpers::SlotData<void(const QString &)>(72, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 73 },
        }}),
        // Slot 'updateBandSelection'
        QtMocHelpers::SlotData<void(int)>(74, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 75 },
        }}),
        // Slot 'onKpodEncoderRotated'
        QtMocHelpers::SlotData<void(int)>(76, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 77 },
        }}),
        // Slot 'onKpodRockerChanged'
        QtMocHelpers::SlotData<void(int)>(78, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 79 },
        }}),
        // Slot 'onKpodPollError'
        QtMocHelpers::SlotData<void(const QString &)>(80, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Slot 'onKpodEnabledChanged'
        QtMocHelpers::SlotData<void(bool)>(81, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 36 },
        }}),
        // Slot 'onKpa1500Connected'
        QtMocHelpers::SlotData<void()>(82, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onKpa1500Disconnected'
        QtMocHelpers::SlotData<void()>(83, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onKpa1500Error'
        QtMocHelpers::SlotData<void(const QString &)>(84, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Slot 'onKpa1500EnabledChanged'
        QtMocHelpers::SlotData<void(bool)>(85, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 36 },
        }}),
        // Slot 'onKpa1500SettingsChanged'
        QtMocHelpers::SlotData<void()>(86, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateKpa1500Status'
        QtMocHelpers::SlotData<void()>(87, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onConnectClicked(); break;
        case 1: _t->onDisconnectClicked(); break;
        case 2: _t->onStateChanged((*reinterpret_cast<std::add_pointer_t<TcpClient::ConnectionState>>(_a[1]))); break;
        case 3: _t->onError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->onAuthenticated(); break;
        case 5: _t->onAuthenticationFailed(); break;
        case 6: _t->onCatResponse((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->onFrequencyChanged((*reinterpret_cast<std::add_pointer_t<quint64>>(_a[1]))); break;
        case 8: _t->onFrequencyBChanged((*reinterpret_cast<std::add_pointer_t<quint64>>(_a[1]))); break;
        case 9: _t->onModeChanged((*reinterpret_cast<std::add_pointer_t<RadioState::Mode>>(_a[1]))); break;
        case 10: _t->onModeBChanged((*reinterpret_cast<std::add_pointer_t<RadioState::Mode>>(_a[1]))); break;
        case 11: _t->onSMeterChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 12: _t->onSMeterBChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 13: _t->onBandwidthChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 14: _t->onBandwidthBChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 15: _t->onRfPowerChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 16: _t->onSupplyVoltageChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 17: _t->onSupplyCurrentChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 18: _t->onSwrChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 19: _t->onSplitChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 20: _t->onAntennaChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 21: _t->onAntennaNameChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 22: _t->onVoxChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 23: _t->onRitXitChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 24: _t->onMessageBankChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 25: _t->onProcessingChanged(); break;
        case 26: _t->onProcessingChangedB(); break;
        case 27: _t->onSpectrumData((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<qint64>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<qint32>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<float>>(_a[5]))); break;
        case 28: _t->onMiniSpectrumData((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[2]))); break;
        case 29: _t->onAudioData((*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[1]))); break;
        case 30: _t->showRadioManager(); break;
        case 31: _t->connectToRadio((*reinterpret_cast<std::add_pointer_t<RadioEntry>>(_a[1]))); break;
        case 32: _t->updateDateTime(); break;
        case 33: _t->showMenuOverlay(); break;
        case 34: _t->onMenuValueChangeRequested((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 35: _t->showBandPopup(); break;
        case 36: _t->onBandSelected((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 37: _t->updateBandSelection((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 38: _t->onKpodEncoderRotated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 39: _t->onKpodRockerChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 40: _t->onKpodPollError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 41: _t->onKpodEnabledChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 42: _t->onKpa1500Connected(); break;
        case 43: _t->onKpa1500Disconnected(); break;
        case 44: _t->onKpa1500Error((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 45: _t->onKpa1500EnabledChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 46: _t->onKpa1500SettingsChanged(); break;
        case 47: _t->updateKpa1500Status(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 48)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 48;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 48)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 48;
    }
    return _id;
}
QT_WARNING_POP
