/****************************************************************************
** Meta object code from reading C++ file 'sidecontrolpanel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/sidecontrolpanel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sidecontrolpanel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16SideControlPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto SideControlPanel::qt_create_metaobjectdata<qt_meta_tag_ZN16SideControlPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SideControlPanel",
        "helpClicked",
        "",
        "connectClicked",
        "wpmChanged",
        "delta",
        "pitchChanged",
        "powerChanged",
        "delayChanged",
        "bandwidthChanged",
        "highCutChanged",
        "shiftChanged",
        "lowCutChanged",
        "mainRfGainChanged",
        "mainSquelchChanged",
        "subSquelchChanged",
        "subRfGainChanged",
        "volumeChanged",
        "value",
        "onWpmBecameActive",
        "onPwrBecameActive",
        "onWpmScrolled",
        "onPwrScrolled",
        "onBwBecameActive",
        "onShiftBecameActive",
        "onBwScrolled",
        "onShiftScrolled",
        "onBwClicked",
        "onShiftClicked",
        "onMainRfBecameActive",
        "onSubSqlBecameActive",
        "onMainRfScrolled",
        "onSubSqlScrolled"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'helpClicked'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'connectClicked'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'wpmChanged'
        QtMocHelpers::SignalData<void(int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'pitchChanged'
        QtMocHelpers::SignalData<void(int)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'powerChanged'
        QtMocHelpers::SignalData<void(int)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'delayChanged'
        QtMocHelpers::SignalData<void(int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'bandwidthChanged'
        QtMocHelpers::SignalData<void(int)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'highCutChanged'
        QtMocHelpers::SignalData<void(int)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'shiftChanged'
        QtMocHelpers::SignalData<void(int)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'lowCutChanged'
        QtMocHelpers::SignalData<void(int)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'mainRfGainChanged'
        QtMocHelpers::SignalData<void(int)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'mainSquelchChanged'
        QtMocHelpers::SignalData<void(int)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'subSquelchChanged'
        QtMocHelpers::SignalData<void(int)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'subRfGainChanged'
        QtMocHelpers::SignalData<void(int)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'volumeChanged'
        QtMocHelpers::SignalData<void(int)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 18 },
        }}),
        // Slot 'onWpmBecameActive'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPwrBecameActive'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onWpmScrolled'
        QtMocHelpers::SlotData<void(int)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'onPwrScrolled'
        QtMocHelpers::SlotData<void(int)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'onBwBecameActive'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onShiftBecameActive'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onBwScrolled'
        QtMocHelpers::SlotData<void(int)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'onShiftScrolled'
        QtMocHelpers::SlotData<void(int)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'onBwClicked'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onShiftClicked'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onMainRfBecameActive'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSubSqlBecameActive'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onMainRfScrolled'
        QtMocHelpers::SlotData<void(int)>(31, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'onSubSqlScrolled'
        QtMocHelpers::SlotData<void(int)>(32, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SideControlPanel, qt_meta_tag_ZN16SideControlPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SideControlPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16SideControlPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16SideControlPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16SideControlPanelE_t>.metaTypes,
    nullptr
} };

void SideControlPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SideControlPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->helpClicked(); break;
        case 1: _t->connectClicked(); break;
        case 2: _t->wpmChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->pitchChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->powerChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->delayChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->bandwidthChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->highCutChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->shiftChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->lowCutChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 10: _t->mainRfGainChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->mainSquelchChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 12: _t->subSquelchChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 13: _t->subRfGainChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 14: _t->volumeChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 15: _t->onWpmBecameActive(); break;
        case 16: _t->onPwrBecameActive(); break;
        case 17: _t->onWpmScrolled((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 18: _t->onPwrScrolled((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 19: _t->onBwBecameActive(); break;
        case 20: _t->onShiftBecameActive(); break;
        case 21: _t->onBwScrolled((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 22: _t->onShiftScrolled((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 23: _t->onBwClicked(); break;
        case 24: _t->onShiftClicked(); break;
        case 25: _t->onMainRfBecameActive(); break;
        case 26: _t->onSubSqlBecameActive(); break;
        case 27: _t->onMainRfScrolled((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 28: _t->onSubSqlScrolled((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)()>(_a, &SideControlPanel::helpClicked, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)()>(_a, &SideControlPanel::connectClicked, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::wpmChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::pitchChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::powerChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::delayChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::bandwidthChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::highCutChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::shiftChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::lowCutChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::mainRfGainChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::mainSquelchChanged, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::subSquelchChanged, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::subRfGainChanged, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (SideControlPanel::*)(int )>(_a, &SideControlPanel::volumeChanged, 14))
            return;
    }
}

const QMetaObject *SideControlPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SideControlPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16SideControlPanelE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SideControlPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 29)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 29;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 29)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 29;
    }
    return _id;
}

// SIGNAL 0
void SideControlPanel::helpClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SideControlPanel::connectClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SideControlPanel::wpmChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void SideControlPanel::pitchChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void SideControlPanel::powerChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void SideControlPanel::delayChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void SideControlPanel::bandwidthChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void SideControlPanel::highCutChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void SideControlPanel::shiftChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void SideControlPanel::lowCutChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void SideControlPanel::mainRfGainChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}

// SIGNAL 11
void SideControlPanel::mainSquelchChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1);
}

// SIGNAL 12
void SideControlPanel::subSquelchChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1);
}

// SIGNAL 13
void SideControlPanel::subRfGainChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 13, nullptr, _t1);
}

// SIGNAL 14
void SideControlPanel::volumeChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 14, nullptr, _t1);
}
QT_WARNING_POP
