// === noobWarrior ===
// File: IdTypeFields.h
// Started by: Hattozo
// Started on: 8/3/2025
// Description: Fields for id types, cause you need to show what things the user can edit for this certain id.
// And we're using a fucking language that still doesn't have reflection so everything is retarded
#pragma once
#include <NoobWarrior/Database/Record/IdRecord.h>
#include <NoobWarrior/Database/Record/IdType/Asset.h>
#include <NoobWarrior/Database/Record/IdType/Badge.h>
#include <NoobWarrior/Database/Record/IdType/User.h>

#include <QLineEdit>
#include <QSpinBox>

#include <map>
#include <QComboBox>
#include <tuple>
#include <tuple>
#include <tuple>
#include <tuple>

namespace NoobWarrior {
    struct FieldDesc {
        QString Label;
        std::function<void*(void*)> MemberGetter;
        std::function<QWidget*(QWidget*)> WidgetFactory;
    };

    // mommy i dont think i was destined to be a c++ programmer
template<typename T>
auto GetIdTypeFields() {
    std::vector<FieldDesc> fields = {
        {
            "Id",
            [](void *obj) -> void* { return &static_cast<T*>(obj)->Id; },
            [](auto *p) { return new QLineEdit(p); }
        },
        {
            "Name",
            [](void *obj) -> void* { return &static_cast<T*>(obj)->Name; },
            [](auto *p) { return new QLineEdit(p); }
        },
        {
            "Version",
            [](void *obj) -> void* { return &static_cast<T*>(obj)->Version; },
            [](auto *p) { return new QSpinBox(p); }
        }
    };
    if constexpr (std::is_same_v<T, Asset>) {
        fields.push_back({
            "Version",
            [](void *obj) -> void* { return &static_cast<T*>(obj)->Type; },
            [](auto *p) { return new QComboBox(p); }
        });
        fields.push_back({
            "Currency Type",
            [](void *obj) -> void* { return &static_cast<T*>(obj)->CurrencyType; },
            [](auto *p) { return new QComboBox(p); }
        });
        fields.push_back({
            "Price",
            [](void *obj) -> void* { return &static_cast<T*>(obj)->Price; },
            [](auto *p) { return new QSpinBox(p); }
        });
    } else if constexpr (std::is_same_v<T, Badge>) {
        fields.push_back({
            "Awarded",
            [](void *obj) -> void* { return &static_cast<T*>(obj)->Awarded; },
            [](auto *p) { return new QSpinBox(p); }
        });
        fields.push_back({
            "Awarded Yesterday",
            [](void *obj) -> void* { return &static_cast<T*>(obj)->Id; },
            [](auto *p) { return new QSpinBox(p); }
        });
    } else if constexpr (std::is_same_v<T, User>) {

    }
    return fields;
}

// https://www.cppstories.com/2022/tuple-iteration-apply/
template <typename TupleT, typename Fn>
void for_each_tuple(TupleT&& tp, Fn&& fn) {
    std::apply
    (
        [&fn]<typename ...T>(T&& ...args)
        {
            (fn(std::forward<T>(args)), ...);
        }, std::forward<TupleT>(tp)
    );
}
}
