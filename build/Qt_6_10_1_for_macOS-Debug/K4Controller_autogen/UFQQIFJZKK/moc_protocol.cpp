/****************************************************************************
** Meta object code from reading C++ file 'protocol.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/network/protocol.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'protocol.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8ProtocolE_t {};
} // unnamed namespace

template <> constexpr inline auto Protocol::qt_create_metaobjectdata<qt_meta_tag_ZN8ProtocolE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Protocol",
        "audioDataReady",
        "",
        "opusData",
        "spectrumDataReady",
        "receiver",
        "spectrumData",
        "centerFreq",
        "sampleRate",
        "noiseFloor",
        "miniSpectrumDataReady",
        "catResponseReceived",
        "response",
        "packetReceived",
        "type",
        "payload"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'audioDataReady'
        QtMocHelpers::SignalData<void(const QByteArray &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QByteArray, 3 },
        }}),
        // Signal 'spectrumDataReady'
        QtMocHelpers::SignalData<void(int, const QByteArray &, qint64, qint32, float)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { QMetaType::QByteArray, 6 }, { QMetaType::LongLong, 7 }, { QMetaType::Int, 8 },
            { QMetaType::Float, 9 },
        }}),
        // Signal 'miniSpectrumDataReady'
        QtMocHelpers::SignalData<void(int, const QByteArray &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { QMetaType::QByteArray, 6 },
        }}),
        // Signal 'catResponseReceived'
        QtMocHelpers::SignalData<void(const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 },
        }}),
        // Signal 'packetReceived'
        QtMocHelpers::SignalData<void(quint8, const QByteArray &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UChar, 14 }, { QMetaType::QByteArray, 15 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Protocol, qt_meta_tag_ZN8ProtocolE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Protocol::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8ProtocolE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8ProtocolE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8ProtocolE_t>.metaTypes,
    nullptr
} };

void Protocol::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Protocol *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->audioDataReady((*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[1]))); break;
        case 1: _t->spectrumDataReady((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<qint64>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<qint32>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<float>>(_a[5]))); break;
        case 2: _t->miniSpectrumDataReady((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[2]))); break;
        case 3: _t->catResponseReceived((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->packetReceived((*reinterpret_cast<std::add_pointer_t<quint8>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Protocol::*)(const QByteArray & )>(_a, &Protocol::audioDataReady, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Protocol::*)(int , const QByteArray & , qint64 , qint32 , float )>(_a, &Protocol::spectrumDataReady, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (Protocol::*)(int , const QByteArray & )>(_a, &Protocol::miniSpectrumDataReady, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (Protocol::*)(const QString & )>(_a, &Protocol::catResponseReceived, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (Protocol::*)(quint8 , const QByteArray & )>(_a, &Protocol::packetReceived, 4))
            return;
    }
}

const QMetaObject *Protocol::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Protocol::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8ProtocolE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Protocol::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void Protocol::audioDataReady(const QByteArray & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void Protocol::spectrumDataReady(int _t1, const QByteArray & _t2, qint64 _t3, qint32 _t4, float _t5)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2, _t3, _t4, _t5);
}

// SIGNAL 2
void Protocol::miniSpectrumDataReady(int _t1, const QByteArray & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void Protocol::catResponseReceived(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void Protocol::packetReceived(quint8 _t1, const QByteArray & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}
QT_WARNING_POP
