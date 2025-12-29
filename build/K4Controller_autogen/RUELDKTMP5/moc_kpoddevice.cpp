/****************************************************************************
** Meta object code from reading C++ file 'kpoddevice.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/hardware/kpoddevice.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'kpoddevice.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10KpodDeviceE_t {};
} // unnamed namespace

template <> constexpr inline auto KpodDevice::qt_create_metaobjectdata<qt_meta_tag_ZN10KpodDeviceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "KpodDevice",
        "deviceConnected",
        "",
        "deviceDisconnected",
        "encoderRotated",
        "ticks",
        "rockerPositionChanged",
        "RockerPosition",
        "position",
        "pollError",
        "error",
        "poll",
        "RockerCenter",
        "RockerRight",
        "RockerLeft"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'deviceConnected'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'deviceDisconnected'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'encoderRotated'
        QtMocHelpers::SignalData<void(int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'rockerPositionChanged'
        QtMocHelpers::SignalData<void(enum RockerPosition)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Signal 'pollError'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Slot 'poll'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'RockerPosition'
        QtMocHelpers::EnumData<enum RockerPosition>(7, 7, QMC::EnumFlags{}).add({
            {   12, RockerPosition::RockerCenter },
            {   13, RockerPosition::RockerRight },
            {   14, RockerPosition::RockerLeft },
        }),
    };
    return QtMocHelpers::metaObjectData<KpodDevice, qt_meta_tag_ZN10KpodDeviceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject KpodDevice::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10KpodDeviceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10KpodDeviceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10KpodDeviceE_t>.metaTypes,
    nullptr
} };

void KpodDevice::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<KpodDevice *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->deviceConnected(); break;
        case 1: _t->deviceDisconnected(); break;
        case 2: _t->encoderRotated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->rockerPositionChanged((*reinterpret_cast<std::add_pointer_t<enum RockerPosition>>(_a[1]))); break;
        case 4: _t->pollError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->poll(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (KpodDevice::*)()>(_a, &KpodDevice::deviceConnected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (KpodDevice::*)()>(_a, &KpodDevice::deviceDisconnected, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (KpodDevice::*)(int )>(_a, &KpodDevice::encoderRotated, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (KpodDevice::*)(RockerPosition )>(_a, &KpodDevice::rockerPositionChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (KpodDevice::*)(const QString & )>(_a, &KpodDevice::pollError, 4))
            return;
    }
}

const QMetaObject *KpodDevice::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KpodDevice::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10KpodDeviceE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int KpodDevice::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void KpodDevice::deviceConnected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void KpodDevice::deviceDisconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void KpodDevice::encoderRotated(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void KpodDevice::rockerPositionChanged(RockerPosition _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void KpodDevice::pollError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}
QT_WARNING_POP
