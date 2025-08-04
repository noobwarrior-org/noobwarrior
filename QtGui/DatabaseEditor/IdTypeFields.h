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
#include <QPushButton>
#include <tuple>
#include <tuple>
#include <tuple>
#include <tuple>

namespace NoobWarrior {
    struct FieldDesc {
        QString                                     Label;
        std::function<std::any(std::any)>           ConvertValueToAny;
        std::function<void(std::any&, QWidget*)>    SetValue;
        std::function<QWidget*(QWidget*,std::any)>  WidgetFactory;
        std::function<QString(QWidget*)>        Validate;
    };

    // mommy i dont think i was destined to be a c++ programmer
template<typename T>
auto GetFields() {
    std::vector<FieldDesc> fields;

    fields.push_back({
        "Id",
        [](std::any obj) -> std::any {
            auto& content = std::any_cast<T&>(obj);
            return std::any { content.Id };
        },
        [](std::any& obj, QWidget *widget) {
            auto *le = qobject_cast<QLineEdit*>(widget);
            auto& content = std::any_cast<T&>(obj);
            content.Id = static_cast<int64_t>(std::any_cast<int>(le->text().toInt()));
        },
        [](QWidget* parent, std::any val) -> QWidget* {
            auto* le = new QLineEdit(parent);
            le->setText(QString::number(std::any_cast<int64_t>(val)));
            return le;
        },
        [](QWidget *widget) {
            bool ok = false;
            auto *le = qobject_cast<QLineEdit*>(widget);
            int id = le->text().toInt(&ok);
            if (id < 0)
                return "Id cannot be less than 0";
            return !ok ? "Id is not a valid number" : "";
        }
    });

    fields.push_back({
        "Name",
        [](std::any obj) -> std::any {
            return std::any{ std::any_cast<T&>(obj).Name };
        },
        [](std::any& obj, QWidget *widget){
            auto *le = qobject_cast<QLineEdit*>(widget);
            auto& content = std::any_cast<T&>(obj);
            content.Name = std::any_cast<std::string>(le->text().toStdString());
        },
        [](QWidget* parent, std::any val) -> QWidget* {
            auto* le = new QLineEdit(parent);
            le->setText(std::any_cast<std::string>(val).c_str());
            return le;
        },
        [](QWidget *widget) {
            auto *le = qobject_cast<QLineEdit*>(widget);
            return le->text().isEmpty() ? "Name cannot be empty" : "";
        }
    });

    fields.push_back({
        "Version",
        [](std::any obj) -> std::any {
            return std::any{ std::any_cast<T&>(obj).Version };
        },
        [](std::any& obj, QWidget *widget){
            auto *sb = qobject_cast<QSpinBox*>(widget);
            auto& content = std::any_cast<T&>(obj);
            content.Version = static_cast<int64_t>(std::any_cast<int>(sb->value()));
        },
        [](QWidget* parent, std::any val) -> QWidget* {
            auto* sb = new QSpinBox(parent);
            sb->setMinimum(1);
            sb->setMaximum(1874919424);
            sb->setValue(std::any_cast<int>(val));
            return sb;
        },
        [](QWidget *widget) {
            return "";
        }
    });

    if constexpr (std::is_same_v<T, Asset>) {
        fields.push_back({
            "Category",
            [](std::any obj) -> std::any {
                return std::any { std::any_cast<T&>(obj).Type };
            },
            [](std::any& obj, QWidget *widget){},
            [](QWidget* parent, std::any val) -> QWidget* {
                auto* combobox = new QComboBox(parent);

                for (int i = 0; i <= AssetCategoryCount; i++) {
                    auto assetTypeCategory = static_cast<AssetCategory>(i);
                    QString assetTypeCategoryStr = AssetCategoryAsTranslatableString(assetTypeCategory);
                    combobox->addItem(assetTypeCategoryStr);
                }

                combobox->setCurrentIndex(static_cast<int>(MapAssetTypeToCategory(std::any_cast<Roblox::AssetType>(val))));

                return combobox;
            },
            [](QWidget *widget) {
                return "";
            }
        });

        fields.push_back({
            "Type",
            [](std::any obj) -> std::any {
                return std::any { std::any_cast<T&>(obj).Type };
            },
            [](std::any& obj, QWidget *widget) {
                auto *combobox = qobject_cast<QComboBox*>(widget);
                auto& content = std::any_cast<T&>(obj);
                content.Type = static_cast<Roblox::AssetType>(std::any_cast<int>(combobox->currentData().toInt()));
            },
            [](QWidget* parent, std::any val) -> QWidget* {
                auto* combobox = new QComboBox(parent);

                for (int i = 1; i <= Roblox::AssetTypeCount; i++) {
                    auto assetType = static_cast<Roblox::AssetType>(i);
                    QString assetTypeStr = Roblox::AssetTypeAsTranslatableString(assetType);
                    if (assetTypeStr.compare("None") != 0)
                        combobox->addItem(assetTypeStr, i);
                }

                combobox->setCurrentText(Roblox::AssetTypeAsTranslatableString(std::any_cast<Roblox::AssetType>(val)));

                return combobox;
            },
            [](QWidget *widget) {
                return "";
            }
        });

        fields.push_back({
            "Currency Type",
            [](std::any obj) -> std::any {
                return std::any { std::any_cast<T&>(obj).CurrencyType };
            },
            [](std::any& obj, QWidget *widget) {
                auto *combobox = qobject_cast<QComboBox*>(widget);
                auto& content = std::any_cast<T&>(obj);
                content.CurrencyType = static_cast<Roblox::CurrencyType>(std::any_cast<int>(combobox->currentData().toInt()));
            },
            [](QWidget* parent, std::any val) -> QWidget* {
                auto* combobox = new QComboBox(parent);

                for (int i = 1; i <= Roblox::CurrencyTypeCount; i++) {
                    auto currencyType = static_cast<Roblox::CurrencyType>(i);
                    QString currencyTypeStr = Roblox::CurrencyTypeAsTranslatableString(currencyType);
                    combobox->addItem(currencyTypeStr, i);
                }

                combobox->setCurrentText(Roblox::CurrencyTypeAsTranslatableString(std::any_cast<Roblox::CurrencyType>(val)));

                return combobox;
            },
            [](QWidget *widget) {
                return "";
            }
        });

        fields.push_back({
            "Price",
            [](std::any obj) -> std::any {
                return std::any { std::any_cast<T&>(obj).Price };
            },
            [](std::any& obj, QWidget *widget) {
                auto *le = qobject_cast<QLineEdit*>(widget);
                auto& content = std::any_cast<T&>(obj);
                content.Price = std::any_cast<int>(le->text().toInt());
            },
            [](QWidget* parent, std::any val) -> QWidget* {
                auto* le = new QLineEdit(parent);
                le->setText(QString::number(std::any_cast<int>(val)));
                return le;
            },
            [](QWidget *widget) {
                bool ok = false;
                auto *le = qobject_cast<QLineEdit*>(widget);
                int id = le->text().toInt(&ok);
                if (id < 0)
                    return "Price cannot be less than 0";
                return !ok ? "Price is not a valid number" : "";
            }
        });
    } else if constexpr (std::is_same_v<T, Badge>) {
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
