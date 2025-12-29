/****************************************************************************
** Meta object code from reading C++ file 'kpa1500client.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/network/kpa1500client.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'kpa1500client.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN13KPA1500ClientE_t {};
} // unnamed namespace

template <> constexpr inline auto KPA1500Client::qt_create_metaobjectdata<qt_meta_tag_ZN13KPA1500ClientE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "KPA1500Client",
        "stateChanged",
        "",
        "ConnectionState",
        "state",
        "connected",
        "disconnected",
        "errorOccurred",
        "error",
        "bandChanged",
        "band",
        "powerChanged",
        "forward",
        "reflected",
        "drive",
        "swrChanged",
        "swr",
        "paVoltageChanged",
        "voltage",
        "paCurrentChanged",
        "current",
        "paTemperatureChanged",
        "tempC",
        "operatingStateChanged",
        "OperatingState",
        "faultStatusChanged",
        "FaultStatus",
        "status",
        "faultCode",
        "atuStatusChanged",
        "present",
        "active",
        "onSocketConnected",
        "onSocketDisconnected",
        "onReadyRead",
        "onSocketError",
        "QAbstractSocket::SocketError",
        "onPollTimer",
        "Disconnected",
        "Connecting",
        "Connected",
        "StateUnknown",
        "StateStandby",
        "StateOperate",
        "FaultNone",
        "FaultActive",
        "FaultHistory"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'stateChanged'
        QtMocHelpers::SignalData<void(enum ConnectionState)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'connected'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'disconnected'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Signal 'bandChanged'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Signal 'powerChanged'
        QtMocHelpers::SignalData<void(double, double, double)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 12 }, { QMetaType::Double, 13 }, { QMetaType::Double, 14 },
        }}),
        // Signal 'swrChanged'
        QtMocHelpers::SignalData<void(double)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 16 },
        }}),
        // Signal 'paVoltageChanged'
        QtMocHelpers::SignalData<void(double)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 18 },
        }}),
        // Signal 'paCurrentChanged'
        QtMocHelpers::SignalData<void(double)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 20 },
        }}),
        // Signal 'paTemperatureChanged'
        QtMocHelpers::SignalData<void(double)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 22 },
        }}),
        // Signal 'operatingStateChanged'
        QtMocHelpers::SignalData<void(enum OperatingState)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 24, 4 },
        }}),
        // Signal 'faultStatusChanged'
        QtMocHelpers::SignalData<void(enum FaultStatus, const QString &)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 26, 27 }, { QMetaType::QString, 28 },
        }}),
        // Signal 'atuStatusChanged'
        QtMocHelpers::SignalData<void(bool, bool)>(29, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 30 }, { QMetaType::Bool, 31 },
        }}),
        // Slot 'onSocketConnected'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSocketDisconnected'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onReadyRead'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSocketError'
        QtMocHelpers::SlotData<void(QAbstractSocket::SocketError)>(35, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 36, 8 },
        }}),
        // Slot 'onPollTimer'
        QtMocHelpers::SlotData<void()>(37, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'ConnectionState'
        QtMocHelpers::EnumData<enum ConnectionState>(3, 3, QMC::EnumFlags{}).add({
            {   38, ConnectionState::Disconnected },
            {   39, ConnectionState::Connecting },
            {   40, ConnectionState::Connected },
        }),
        // enum 'OperatingState'
        QtMocHelpers::EnumData<enum OperatingState>(24, 24, QMC::EnumFlags{}).add({
            {   41, OperatingState::StateUnknown },
            {   42, OperatingState::StateStandby },
            {   43, OperatingState::StateOperate },
        }),
        // enum 'FaultStatus'
        QtMocHelpers::EnumData<enum FaultStatus>(26, 26, QMC::EnumFlags{}).add({
            {   44, FaultStatus::FaultNone },
            {   45, FaultStatus::FaultActive },
            {   46, FaultStatus::FaultHistory },
        }),
    };
    return QtMocHelpers::metaObjectData<KPA1500Client, qt_meta_tag_ZN13KPA1500ClientE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject KPA1500Client::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13KPA1500ClientE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13KPA1500ClientE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13KPA1500ClientE_t>.metaTypes,
    nullptr
} };

void KPA1500Client::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<KPA1500Client *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->stateChanged((*reinterpret_cast<std::add_pointer_t<enum ConnectionState>>(_a[1]))); break;
        case 1: _t->connected(); break;
        case 2: _t->disconnected(); break;
        case 3: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->bandChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->powerChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[3]))); break;
        case 6: _t->swrChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 7: _t->paVoltageChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 8: _t->paCurrentChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 9: _t->paTemperatureChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 10: _t->operatingStateChanged((*reinterpret_cast<std::add_pointer_t<enum OperatingState>>(_a[1]))); break;
        case 11: _t->faultStatusChanged((*reinterpret_cast<std::add_pointer_t<enum FaultStatus>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 12: _t->atuStatusChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 13: _t->onSocketConnected(); break;
        case 14: _t->onSocketDisconnected(); break;
        case 15: _t->onReadyRead(); break;
        case 16: _t->onSocketError((*reinterpret_cast<std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        case 17: _t->onPollTimer(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 16:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)(ConnectionState )>(_a, &KPA1500Client::stateChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)()>(_a, &KPA1500Client::connected, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)()>(_a, &KPA1500Client::disconnected, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)(const QString & )>(_a, &KPA1500Client::errorOccurred, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)(const QString & )>(_a, &KPA1500Client::bandChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)(double , double , double )>(_a, &KPA1500Client::powerChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)(double )>(_a, &KPA1500Client::swrChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)(double )>(_a, &KPA1500Client::paVoltageChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)(double )>(_a, &KPA1500Client::paCurrentChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)(double )>(_a, &KPA1500Client::paTemperatureChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)(OperatingState )>(_a, &KPA1500Client::operatingStateChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)(FaultStatus , const QString & )>(_a, &KPA1500Client::faultStatusChanged, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (KPA1500Client::*)(bool , bool )>(_a, &KPA1500Client::atuStatusChanged, 12))
            return;
    }
}

const QMetaObject *KPA1500Client::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KPA1500Client::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13KPA1500ClientE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int KPA1500Client::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    return _id;
}

// SIGNAL 0
void KPA1500Client::stateChanged(ConnectionState _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void KPA1500Client::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void KPA1500Client::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void KPA1500Client::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void KPA1500Client::bandChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void KPA1500Client::powerChanged(double _t1, double _t2, double _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2, _t3);
}

// SIGNAL 6
void KPA1500Client::swrChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void KPA1500Client::paVoltageChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void KPA1500Client::paCurrentChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void KPA1500Client::paTemperatureChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void KPA1500Client::operatingStateChanged(OperatingState _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}

// SIGNAL 11
void KPA1500Client::faultStatusChanged(FaultStatus _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1, _t2);
}

// SIGNAL 12
void KPA1500Client::atuStatusChanged(bool _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1, _t2);
}
QT_WARNING_POP
