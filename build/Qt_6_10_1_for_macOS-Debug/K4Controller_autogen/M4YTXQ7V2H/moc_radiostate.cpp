/****************************************************************************
** Meta object code from reading C++ file 'radiostate.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/models/radiostate.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'radiostate.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10RadioStateE_t {};
} // unnamed namespace

template <> constexpr inline auto RadioState::qt_create_metaobjectdata<qt_meta_tag_ZN10RadioStateE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "RadioState",
        "frequencyChanged",
        "",
        "freq",
        "frequencyBChanged",
        "modeChanged",
        "Mode",
        "mode",
        "modeBChanged",
        "filterBandwidthChanged",
        "bw",
        "filterBandwidthBChanged",
        "ifShiftChanged",
        "shiftHz",
        "cwPitchChanged",
        "pitchHz",
        "sMeterChanged",
        "value",
        "sMeterBChanged",
        "powerMeterChanged",
        "watts",
        "transmitStateChanged",
        "transmitting",
        "rfPowerChanged",
        "isQrp",
        "supplyVoltageChanged",
        "volts",
        "supplyCurrentChanged",
        "amps",
        "swrChanged",
        "swr",
        "txMeterChanged",
        "alc",
        "compression",
        "fwdPower",
        "splitChanged",
        "enabled",
        "antennaChanged",
        "txAnt",
        "rxAntMain",
        "rxAntSub",
        "antennaNameChanged",
        "index",
        "name",
        "ritXitChanged",
        "ritEnabled",
        "xitEnabled",
        "offset",
        "messageBankChanged",
        "bank",
        "processingChanged",
        "processingChangedB",
        "refLevelChanged",
        "level",
        "spanChanged",
        "spanHz",
        "keyerSpeedChanged",
        "wpm",
        "qskDelayChanged",
        "delay",
        "rfGainChanged",
        "gain",
        "squelchChanged",
        "voxChanged",
        "stateUpdated",
        "LSB",
        "USB",
        "CW",
        "FM",
        "AM",
        "DATA",
        "CW_R",
        "DATA_R",
        "AGCSpeed",
        "AGC_Off",
        "AGC_Slow",
        "AGC_Fast"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'frequencyChanged'
        QtMocHelpers::SignalData<void(quint64)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::ULongLong, 3 },
        }}),
        // Signal 'frequencyBChanged'
        QtMocHelpers::SignalData<void(quint64)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::ULongLong, 3 },
        }}),
        // Signal 'modeChanged'
        QtMocHelpers::SignalData<void(enum Mode)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Signal 'modeBChanged'
        QtMocHelpers::SignalData<void(enum Mode)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Signal 'filterBandwidthChanged'
        QtMocHelpers::SignalData<void(int)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Signal 'filterBandwidthBChanged'
        QtMocHelpers::SignalData<void(int)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Signal 'ifShiftChanged'
        QtMocHelpers::SignalData<void(int)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 },
        }}),
        // Signal 'cwPitchChanged'
        QtMocHelpers::SignalData<void(int)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 15 },
        }}),
        // Signal 'sMeterChanged'
        QtMocHelpers::SignalData<void(double)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 17 },
        }}),
        // Signal 'sMeterBChanged'
        QtMocHelpers::SignalData<void(double)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 17 },
        }}),
        // Signal 'powerMeterChanged'
        QtMocHelpers::SignalData<void(int)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 20 },
        }}),
        // Signal 'transmitStateChanged'
        QtMocHelpers::SignalData<void(bool)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 22 },
        }}),
        // Signal 'rfPowerChanged'
        QtMocHelpers::SignalData<void(double, bool)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 20 }, { QMetaType::Bool, 24 },
        }}),
        // Signal 'supplyVoltageChanged'
        QtMocHelpers::SignalData<void(double)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 26 },
        }}),
        // Signal 'supplyCurrentChanged'
        QtMocHelpers::SignalData<void(double)>(27, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 28 },
        }}),
        // Signal 'swrChanged'
        QtMocHelpers::SignalData<void(double)>(29, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 30 },
        }}),
        // Signal 'txMeterChanged'
        QtMocHelpers::SignalData<void(int, int, double, double)>(31, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 32 }, { QMetaType::Int, 33 }, { QMetaType::Double, 34 }, { QMetaType::Double, 30 },
        }}),
        // Signal 'splitChanged'
        QtMocHelpers::SignalData<void(bool)>(35, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 36 },
        }}),
        // Signal 'antennaChanged'
        QtMocHelpers::SignalData<void(int, int, int)>(37, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 38 }, { QMetaType::Int, 39 }, { QMetaType::Int, 40 },
        }}),
        // Signal 'antennaNameChanged'
        QtMocHelpers::SignalData<void(int, const QString &)>(41, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 42 }, { QMetaType::QString, 43 },
        }}),
        // Signal 'ritXitChanged'
        QtMocHelpers::SignalData<void(bool, bool, int)>(44, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 45 }, { QMetaType::Bool, 46 }, { QMetaType::Int, 47 },
        }}),
        // Signal 'messageBankChanged'
        QtMocHelpers::SignalData<void(int)>(48, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 49 },
        }}),
        // Signal 'processingChanged'
        QtMocHelpers::SignalData<void()>(50, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'processingChangedB'
        QtMocHelpers::SignalData<void()>(51, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'refLevelChanged'
        QtMocHelpers::SignalData<void(int)>(52, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 53 },
        }}),
        // Signal 'spanChanged'
        QtMocHelpers::SignalData<void(int)>(54, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 55 },
        }}),
        // Signal 'keyerSpeedChanged'
        QtMocHelpers::SignalData<void(int)>(56, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 57 },
        }}),
        // Signal 'qskDelayChanged'
        QtMocHelpers::SignalData<void(int)>(58, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 59 },
        }}),
        // Signal 'rfGainChanged'
        QtMocHelpers::SignalData<void(int)>(60, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 61 },
        }}),
        // Signal 'squelchChanged'
        QtMocHelpers::SignalData<void(int)>(62, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 53 },
        }}),
        // Signal 'voxChanged'
        QtMocHelpers::SignalData<void(bool)>(63, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 36 },
        }}),
        // Signal 'stateUpdated'
        QtMocHelpers::SignalData<void()>(64, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'Mode'
        QtMocHelpers::EnumData<enum Mode>(6, 6, QMC::EnumFlags{}).add({
            {   65, Mode::LSB },
            {   66, Mode::USB },
            {   67, Mode::CW },
            {   68, Mode::FM },
            {   69, Mode::AM },
            {   70, Mode::DATA },
            {   71, Mode::CW_R },
            {   72, Mode::DATA_R },
        }),
        // enum 'AGCSpeed'
        QtMocHelpers::EnumData<enum AGCSpeed>(73, 73, QMC::EnumFlags{}).add({
            {   74, AGCSpeed::AGC_Off },
            {   75, AGCSpeed::AGC_Slow },
            {   76, AGCSpeed::AGC_Fast },
        }),
    };
    return QtMocHelpers::metaObjectData<RadioState, qt_meta_tag_ZN10RadioStateE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject RadioState::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10RadioStateE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10RadioStateE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10RadioStateE_t>.metaTypes,
    nullptr
} };

void RadioState::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<RadioState *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->frequencyChanged((*reinterpret_cast<std::add_pointer_t<quint64>>(_a[1]))); break;
        case 1: _t->frequencyBChanged((*reinterpret_cast<std::add_pointer_t<quint64>>(_a[1]))); break;
        case 2: _t->modeChanged((*reinterpret_cast<std::add_pointer_t<enum Mode>>(_a[1]))); break;
        case 3: _t->modeBChanged((*reinterpret_cast<std::add_pointer_t<enum Mode>>(_a[1]))); break;
        case 4: _t->filterBandwidthChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->filterBandwidthBChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->ifShiftChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->cwPitchChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->sMeterChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 9: _t->sMeterBChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 10: _t->powerMeterChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->transmitStateChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 12: _t->rfPowerChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 13: _t->supplyVoltageChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 14: _t->supplyCurrentChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 15: _t->swrChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 16: _t->txMeterChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[4]))); break;
        case 17: _t->splitChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 18: _t->antennaChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 19: _t->antennaNameChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 20: _t->ritXitChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 21: _t->messageBankChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 22: _t->processingChanged(); break;
        case 23: _t->processingChangedB(); break;
        case 24: _t->refLevelChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 25: _t->spanChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 26: _t->keyerSpeedChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 27: _t->qskDelayChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 28: _t->rfGainChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 29: _t->squelchChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 30: _t->voxChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 31: _t->stateUpdated(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(quint64 )>(_a, &RadioState::frequencyChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(quint64 )>(_a, &RadioState::frequencyBChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(Mode )>(_a, &RadioState::modeChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(Mode )>(_a, &RadioState::modeBChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::filterBandwidthChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::filterBandwidthBChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::ifShiftChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::cwPitchChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(double )>(_a, &RadioState::sMeterChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(double )>(_a, &RadioState::sMeterBChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::powerMeterChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(bool )>(_a, &RadioState::transmitStateChanged, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(double , bool )>(_a, &RadioState::rfPowerChanged, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(double )>(_a, &RadioState::supplyVoltageChanged, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(double )>(_a, &RadioState::supplyCurrentChanged, 14))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(double )>(_a, &RadioState::swrChanged, 15))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int , int , double , double )>(_a, &RadioState::txMeterChanged, 16))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(bool )>(_a, &RadioState::splitChanged, 17))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int , int , int )>(_a, &RadioState::antennaChanged, 18))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int , const QString & )>(_a, &RadioState::antennaNameChanged, 19))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(bool , bool , int )>(_a, &RadioState::ritXitChanged, 20))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::messageBankChanged, 21))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)()>(_a, &RadioState::processingChanged, 22))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)()>(_a, &RadioState::processingChangedB, 23))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::refLevelChanged, 24))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::spanChanged, 25))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::keyerSpeedChanged, 26))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::qskDelayChanged, 27))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::rfGainChanged, 28))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(int )>(_a, &RadioState::squelchChanged, 29))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)(bool )>(_a, &RadioState::voxChanged, 30))
            return;
        if (QtMocHelpers::indexOfMethod<void (RadioState::*)()>(_a, &RadioState::stateUpdated, 31))
            return;
    }
}

const QMetaObject *RadioState::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RadioState::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10RadioStateE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int RadioState::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 32)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 32;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 32)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 32;
    }
    return _id;
}

// SIGNAL 0
void RadioState::frequencyChanged(quint64 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void RadioState::frequencyBChanged(quint64 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void RadioState::modeChanged(Mode _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void RadioState::modeBChanged(Mode _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void RadioState::filterBandwidthChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void RadioState::filterBandwidthBChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void RadioState::ifShiftChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void RadioState::cwPitchChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void RadioState::sMeterChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void RadioState::sMeterBChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void RadioState::powerMeterChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}

// SIGNAL 11
void RadioState::transmitStateChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1);
}

// SIGNAL 12
void RadioState::rfPowerChanged(double _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1, _t2);
}

// SIGNAL 13
void RadioState::supplyVoltageChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 13, nullptr, _t1);
}

// SIGNAL 14
void RadioState::supplyCurrentChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 14, nullptr, _t1);
}

// SIGNAL 15
void RadioState::swrChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 15, nullptr, _t1);
}

// SIGNAL 16
void RadioState::txMeterChanged(int _t1, int _t2, double _t3, double _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 16, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 17
void RadioState::splitChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 17, nullptr, _t1);
}

// SIGNAL 18
void RadioState::antennaChanged(int _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 18, nullptr, _t1, _t2, _t3);
}

// SIGNAL 19
void RadioState::antennaNameChanged(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 19, nullptr, _t1, _t2);
}

// SIGNAL 20
void RadioState::ritXitChanged(bool _t1, bool _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 20, nullptr, _t1, _t2, _t3);
}

// SIGNAL 21
void RadioState::messageBankChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 21, nullptr, _t1);
}

// SIGNAL 22
void RadioState::processingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 22, nullptr);
}

// SIGNAL 23
void RadioState::processingChangedB()
{
    QMetaObject::activate(this, &staticMetaObject, 23, nullptr);
}

// SIGNAL 24
void RadioState::refLevelChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 24, nullptr, _t1);
}

// SIGNAL 25
void RadioState::spanChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 25, nullptr, _t1);
}

// SIGNAL 26
void RadioState::keyerSpeedChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 26, nullptr, _t1);
}

// SIGNAL 27
void RadioState::qskDelayChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 27, nullptr, _t1);
}

// SIGNAL 28
void RadioState::rfGainChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 28, nullptr, _t1);
}

// SIGNAL 29
void RadioState::squelchChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 29, nullptr, _t1);
}

// SIGNAL 30
void RadioState::voxChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 30, nullptr, _t1);
}

// SIGNAL 31
void RadioState::stateUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 31, nullptr);
}
QT_WARNING_POP
