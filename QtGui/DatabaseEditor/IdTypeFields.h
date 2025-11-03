// === noobWarrior ===
// File: IdTypeFields.h
// Started by: Hattozo
// Started on: 8/3/2025
// Description: Fields for id types, cause you need to show what things the user can edit for this certain id.
// And we're using a fucking language that still doesn't have reflection so everything is retarded
#pragma once
#include <NoobWarrior/Reflection.h>
#include <NoobWarrior/Database/Record/IdRecord.h>
#include <NoobWarrior/Database/Record/IdType/Asset.h>
#include <NoobWarrior/Database/Record/IdType/Badge.h>
#include <NoobWarrior/Database/Record/IdType/User.h>
#include <NoobWarrior/Database/AssetCategory.h>
#include <NoobWarrior/Roblox/Api/Asset.h>

#include "ContentEditorDialogBase.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QMessageBox>

#include <map>
#include <QComboBox>
#include <QMessageBox>
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
    std::function<QWidget*(ContentEditorDialogBase*, std::any)>  WidgetFactory;
    std::function<QString(ContentEditorDialogBase*, QWidget*)>        Validate;
};

// mommy i dont think i was destined to be a c++ programmer
template<typename T>
auto GetFields() {
    static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");
    std::vector<FieldDesc> fields;

    fields.push_back({
        .Label = "Id",
        .ConvertValueToAny = [](std::any obj) -> std::any {
            auto& content = std::any_cast<T&>(obj);
            return std::any { content.Id };
        },
        .SetValue = [](std::any& obj, QWidget *widget) {
            auto *le = qobject_cast<QLineEdit*>(widget);
            auto& content = std::any_cast<T&>(obj);
            content.Id = static_cast<int64_t>(std::any_cast<int>(le->text().toInt()));
        },
        .WidgetFactory = [](ContentEditorDialogBase* parent, std::any val) -> QWidget* {
            auto* le = new QLineEdit(parent);
            le->setText(QString::number(std::any_cast<int64_t>(val)));
            return le;
        },
        .Validate = [](ContentEditorDialogBase* parent, QWidget *widget) {
            bool ok = false;
            auto *le = qobject_cast<QLineEdit*>(widget);
            int id = le->text().toInt(&ok);

            if (id < 0)
                return "Id cannot be less than 0";
            if (!ok)
                return "Id is not a valid number";

            // You are creating a new asset (because parent->GetId() is null so that means we aren't already editing an existing id)
            // But this new asset you are trying to make has an id that already exists in our database? You must warn them so that they dont fuck up!
            if (!parent->GetId().has_value() && parent->GetDatabase()->DoesContentExist<T>(id)) {
                QMessageBox::StandardButton res = QMessageBox::question(parent, "Warning",
            QString("The %1 ID you are inputting (%2) already exists in the database.\nIf you click \"Yes\", your %3 will override the previous one.").arg(QString::fromStdString(Reflection::GetIdTypeName<T>()), le->text(), QString::fromStdString(Reflection::GetIdTypeName<T>())),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
                if (res != QMessageBox::Yes) {
                    return "SILENTFAIL";
                }
            }
            return "";
        }
    });

    fields.push_back({
        .Label = "Name",
        .ConvertValueToAny = [](std::any obj) -> std::any {
            return std::any{ std::any_cast<T&>(obj).Name };
        },
        .SetValue = [](std::any& obj, QWidget *widget){
            auto *le = qobject_cast<QLineEdit*>(widget);
            auto& content = std::any_cast<T&>(obj);
            content.Name = std::any_cast<std::string>(le->text().toStdString());
        },
        .WidgetFactory = [](ContentEditorDialogBase* parent, std::any val) -> QWidget* {
            auto* le = new QLineEdit(parent);
            le->setText(std::any_cast<std::string>(val).c_str());
            return le;
        },
        .Validate = [](ContentEditorDialogBase* parent, QWidget *widget) {
            auto *le = qobject_cast<QLineEdit*>(widget);
            return le->text().isEmpty() ? "Name cannot be empty" : "";
        }
    });

    fields.push_back({
        .Label = "Version",
        .ConvertValueToAny = [](std::any obj) -> std::any {
            return std::any{ std::any_cast<T&>(obj).Version };
        },
        .SetValue = [](std::any& obj, QWidget *widget){
            auto *sb = qobject_cast<QSpinBox*>(widget);
            auto& content = std::any_cast<T&>(obj);
            content.Version = static_cast<int64_t>(std::any_cast<int>(sb->value()));
        },
        .WidgetFactory = [](ContentEditorDialogBase* parent, std::any val) -> QWidget* {
            auto* sb = new QSpinBox(parent);
            sb->setMinimum(1);
            sb->setMaximum(1874919424);
            sb->setValue(std::any_cast<int>(val));
            return sb;
        },
        .Validate = [](ContentEditorDialogBase* parent, QWidget *widget) {
            return "";
        }
    });

    if constexpr (std::is_same_v<T, Asset>) {
        fields.push_back({
            .Label = "Category",
            .ConvertValueToAny = [](std::any obj) -> std::any {
                return std::any { std::any_cast<T&>(obj).Type };
            },
            .SetValue = [](std::any& obj, QWidget *widget){},
            .WidgetFactory = [](ContentEditorDialogBase* parent, std::any val) -> QWidget* {
                auto* combobox = new QComboBox(parent);

                for (int i = 0; i <= AssetCategoryCount; i++) {
                    auto assetTypeCategory = static_cast<AssetCategory>(i);
                    QString assetTypeCategoryStr = AssetCategoryAsTranslatableString(assetTypeCategory);
                    combobox->addItem(assetTypeCategoryStr);
                }

                combobox->setCurrentIndex(static_cast<int>(MapAssetTypeToCategory(std::any_cast<Roblox::AssetType>(val))));

                return combobox;
            },
            .Validate = [](ContentEditorDialogBase* parent, QWidget *widget) {
                return "";
            }
        });

        fields.push_back({
            .Label = "Type",
            .ConvertValueToAny = [](std::any obj) -> std::any {
                return std::any { std::any_cast<T&>(obj).Type };
            },
            .SetValue = [](std::any& obj, QWidget *widget) {
                auto *combobox = qobject_cast<QComboBox*>(widget);
                auto& content = std::any_cast<T&>(obj);
                content.Type = static_cast<Roblox::AssetType>(std::any_cast<int>(combobox->currentData().toInt()));
            },
            .WidgetFactory = [](ContentEditorDialogBase* parent, std::any val) -> QWidget* {
                auto* combobox = new QComboBox(parent);

                for (int i = 1; i <= Roblox::AssetTypeCount; i++) {
                    auto assetType = static_cast<Roblox::AssetType>(i);
                    QString assetTypeStr = Roblox::AssetTypeAsTranslatableString(assetType);
                    if (assetTypeStr.compare("None") != 0)
                        combobox->addItem(assetTypeStr, i);
                }

                combobox->setCurrentText(Roblox::AssetTypeAsTranslatableString(std::any_cast<Roblox::AssetType>(val)));

                std::function setImageToAssetType = [parent, combobox]() {
                    auto assetType = static_cast<Roblox::AssetType>(combobox->currentData().toInt());
                    std::optional<int> id = parent->GetId();

                    QImage image;
                    id.has_value() ? image.loadFromData(parent->GetDatabase()->RetrieveContentImageData<T>(id.value())) :
                        image.loadFromData(GetImageForAssetType(assetType));

                    QPixmap pixmap = QPixmap::fromImage(image);
                    parent->mIcon->setPixmap(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                };
                setImageToAssetType();
                parent->connect(combobox, &QComboBox::currentIndexChanged, [setImageToAssetType](int index) {
                    setImageToAssetType();
                });

                return combobox;
            },
            .Validate = [](ContentEditorDialogBase* parent, QWidget *widget) {
                return "";
            }
        });

        fields.push_back({
            .Label = "Currency Type",
            .ConvertValueToAny = [](std::any obj) -> std::any {
                return std::any { std::any_cast<T&>(obj).CurrencyType };
            },
            .SetValue = [](std::any& obj, QWidget *widget) {
                auto *combobox = qobject_cast<QComboBox*>(widget);
                auto& content = std::any_cast<T&>(obj);
                content.CurrencyType = static_cast<Roblox::CurrencyType>(std::any_cast<int>(combobox->currentData().toInt()));
            },
            .WidgetFactory = [](ContentEditorDialogBase* parent, std::any val) -> QWidget* {
                auto* combobox = new QComboBox(parent);

                for (int i = 1; i <= Roblox::CurrencyTypeCount; i++) {
                    auto currencyType = static_cast<Roblox::CurrencyType>(i);
                    QString currencyTypeStr = Roblox::CurrencyTypeAsTranslatableString(currencyType);
                    combobox->addItem(currencyTypeStr, i);
                }

                combobox->setCurrentText(Roblox::CurrencyTypeAsTranslatableString(std::any_cast<Roblox::CurrencyType>(val)));

                return combobox;
            },
            .Validate = [](ContentEditorDialogBase* parent, QWidget *widget) {
                return "";
            }
        });

        fields.push_back({
            .Label = "Price",
            .ConvertValueToAny = [](std::any obj) -> std::any {
                return std::any { std::any_cast<T&>(obj).Price };
            },
            .SetValue = [](std::any& obj, QWidget *widget) {
                auto *le = qobject_cast<QLineEdit*>(widget);
                auto& content = std::any_cast<T&>(obj);
                content.Price = std::any_cast<int>(le->text().toInt());
            },
            .WidgetFactory = [](ContentEditorDialogBase* parent, std::any val) -> QWidget* {
                auto* le = new QLineEdit(parent);
                le->setText(QString::number(std::any_cast<int>(val)));
                return le;
            },
            .Validate = [](ContentEditorDialogBase* parent, QWidget *widget) {
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
