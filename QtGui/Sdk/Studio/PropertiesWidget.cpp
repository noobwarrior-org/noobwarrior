/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: PropertiesWidget.cpp
// Started by: Hattozo
// Started on: 8/29/2026
// Description: A poor replication of Roblox Studio's Properties widget
#include "PropertiesWidget.h"

#include "Sdk/Item/AssetDataFileType.h"

#include <QStyledItemDelegate>
#include <QPainter>
#include <QComboBox>
#include <QHeaderView>
#include <QShortcut>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QToolButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDialogButtonBox>
#include <QMediaPlayer>
#include <QTemporaryFile>
#include <QDir>
#include <QAudioOutput>

#include <numbers>

#include <NoobWarrior/Roblox/FileFormat/Utility/BrickColors.h>

#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
#include <NoobWarrior/Roblox/FileFormat/Generated/EnumItems.h> // enum token -> member names
#include <NoobWarrior/Roblox/FileFormat/Generated/Registry.h> // superclass walk for class icons
#endif

using namespace NoobWarrior;

// Source is a ProtectedString in XML but a plain String column in binary files.
static std::optional<std::string> ScriptSourceFromProperty(const Roblox::Property& prop) {
    if (const auto* ps = prop.CastValue<Roblox::DataTypes::ProtectedString>())
        return ps->ToString();
    if (const auto* s = prop.CastValue<std::string>())
        return *s;
    return std::nullopt;
}

static bool IsScriptSourceProperty(const std::string& propName, const Roblox::Property& prop) {
    return propName == "Source" && ScriptSourceFromProperty(prop).has_value();
}

// Item data roles carried by property rows.
static constexpr int kRoleProp      = Qt::UserRole;     // serialized property name; empty = not a property row
static constexpr int kRoleComponent = Qt::UserRole + 1; // which part of a compound value this row edits
static constexpr int kRoleEditable  = Qt::UserRole + 2; // whether ApplyEdit knows how to write it
static constexpr int kRoleEnumType  = Qt::UserRole + 3; // enum type name; the row edits through a dropdown

static constexpr int kCompWhole     = -1;
static constexpr int kCompX         = 0;
static constexpr int kCompY         = 1;
static constexpr int kCompZ         = 2;
static constexpr int kCompCFramePos = 10;
static constexpr int kCompCFrameRot = 11;
static constexpr int kCompUDim2X    = 20;
static constexpr int kCompUDim2Y    = 21;
// Synthetic (Fake) BrickColor row backed by the Color3uint8 property, edited through the palette.
static constexpr int kCompBrickColorSynthetic = 30;

// The generated Category field is the VALUE type, not Studio's panel grouping.
static QString CategoryForPropertyName(const QString& name) {
    static const std::unordered_map<QString, const char*> kMap = {
        // Appearance
        {"BrickColor", "Appearance"}, {"Color", "Appearance"}, {"Color3uint8", "Appearance"},
        {"Material", "Appearance"}, {"MaterialVariant", "Appearance"},
        {"MaterialVariantSerialized", "Appearance"}, {"Reflectance", "Appearance"},
        {"Transparency", "Appearance"}, {"LocalTransparencyModifier", "Appearance"},
        {"Texture", "Appearance"}, {"TextureID", "Appearance"}, {"TextureId", "Appearance"},
        {"MeshId", "Appearance"}, {"MeshType", "Appearance"}, {"VertexColor", "Appearance"},
        {"Offset", "Appearance"}, {"Scale", "Appearance"}, {"Face", "Appearance"},
        {"BackgroundColor3", "Appearance"}, {"BackgroundTransparency", "Appearance"},
        {"BorderColor3", "Appearance"}, {"BorderSizePixel", "Appearance"},
        {"Image", "Appearance"}, {"ImageColor3", "Appearance"}, {"ImageTransparency", "Appearance"},
        {"Ambient", "Appearance"}, {"Brightness", "Appearance"}, {"OutdoorAmbient", "Appearance"},
        {"FogColor", "Appearance"}, {"FogStart", "Appearance"}, {"FogEnd", "Appearance"},
        // Data
        {"ClassName", "Data"}, {"Name", "Data"}, {"Parent", "Data"},
        {"Position", "Data"}, {"Rotation", "Data"}, {"Orientation", "Data"},
        {"CFrame", "Data"}, {"CoordinateFrame", "Data"}, {"RotVelocity", "Data"},
        {"Velocity", "Data"}, {"Size", "Data"}, {"size", "Data"},
        {"AssemblyLinearVelocity", "Data"}, {"AssemblyAngularVelocity", "Data"},
        {"Value", "Data"}, {"SoundId", "Data"}, {"AnimationId", "Data"},
        {"Health", "Data"}, {"Health_XML", "Data"}, {"MaxHealth", "Data"},
        {"WalkSpeed", "Data"}, {"JumpPower", "Data"}, {"JumpHeight", "Data"},
        {"DisplayName", "Data"}, {"Time", "Data"}, {"TimeOfDay", "Data"}, {"ClockTime", "Data"},
        {"GeographicLatitude", "Data"},
        // Transform / Pivot
        {"PivotOffset", "Pivot"}, {"WorldPivotData", "Pivot"},
        // Behavior
        {"Anchored", "Behavior"}, {"Archivable", "Behavior"}, {"CanCollide", "Behavior"},
        {"CanQuery", "Behavior"}, {"CanTouch", "Behavior"}, {"CastShadow", "Behavior"},
        {"Locked", "Behavior"}, {"Massless", "Behavior"}, {"Enabled", "Behavior"},
        {"Disabled", "Behavior"}, {"Visible", "Behavior"}, {"Active", "Behavior"},
        {"Selectable", "Behavior"}, {"Draggable", "Behavior"}, {"DraggingV1", "Behavior"},
        {"AutoLocalize", "Behavior"}, {"ResetOnSpawn", "Behavior"}, {"Looped", "Behavior"},
        {"Playing", "Behavior"}, {"PlayOnRemove", "Behavior"}, {"CanBeDropped", "Behavior"},
        {"RequiresHandle", "Behavior"}, {"ManualActivationOnly", "Behavior"},
        {"GlobalShadows", "Behavior"}, {"StreamingEnabled", "Behavior"},
        // Collision
        {"CollisionGroup", "Collision"}, {"CollisionGroupId", "Collision"},
        // Part
        {"Shape", "Part"}, {"shape", "Part"}, {"formFactorRaw", "Part"}, {"FormFactor", "Part"},
        {"Elasticity", "Part"}, {"Friction", "Part"}, {"Density", "Part"},
        {"CustomPhysicalProperties", "Part"}, {"RootPriority", "Part"},
        // Camera
        {"FieldOfView", "Camera"}, {"CameraType", "Camera"}, {"CameraSubject", "Camera"},
        {"Focus", "Camera"},
        // Text
        {"Font", "Text"}, {"FontFace", "Text"}, {"Text", "Text"}, {"TextColor3", "Text"},
        {"TextSize", "Text"}, {"TextScaled", "Text"}, {"TextWrapped", "Text"},
        {"TextTransparency", "Text"}, {"TextStrokeColor3", "Text"},
        {"TextStrokeTransparency", "Text"}, {"TextXAlignment", "Text"},
        {"TextYAlignment", "Text"}, {"RichText", "Text"},
    };
    if (auto it = kMap.find(name); it != kMap.end())
        return it->second;
    if (name.endsWith("Surface") || name.endsWith("SurfaceInput") ||
        name.endsWith("ParamA") || name.endsWith("ParamB"))
        return "Surface";
    return "Data";
}

// Studio-style number: no scientific notation, trailing zeros trimmed ("0", "-10", "0.5").
static QString FormatNumber(double v) {
    QString s = QString::number(v, 'f', 3);
    while (s.contains('.') && (s.endsWith('0') || s.endsWith('.')))
        s.chop(1);
    if (s == "-0")
        s = "0";
    return s;
}

static QString FormatVector3(const Roblox::DataTypes::Vector3& v) {
    return FormatNumber(v.X) + ", " + FormatNumber(v.Y) + ", " + FormatNumber(v.Z);
}

// Roblox's fromOrientation is Ry * Rx * Rz (-sin(rx) at m12) - Studio's "Orientation".
static void CFrameToOrientationDegrees(const Roblox::DataTypes::CFrame& cf, double& rx, double& ry, double& rz) {
    const auto& c = cf.Components;
    const double m02 = c[5], m10 = c[6], m11 = c[7], m12 = c[8], m22 = c[11];
    constexpr double kRadToDeg = 180.0 / std::numbers::pi;
    rx = std::asin(std::clamp(-static_cast<double>(m12), -1.0, 1.0)) * kRadToDeg;
    ry = std::atan2(m02, m22) * kRadToDeg;
    rz = std::atan2(m10, m11) * kRadToDeg;
}

// The inverse: writes the Ry*Rx*Rz rotation for the given degrees into a CFrame's matrix part.
static void OrientationDegreesToCFrame(Roblox::DataTypes::CFrame& cf, double rxDeg, double ryDeg, double rzDeg) {
    constexpr double kDegToRad = std::numbers::pi / 180.0;
    const double sx = std::sin(rxDeg * kDegToRad), cx = std::cos(rxDeg * kDegToRad);
    const double sy = std::sin(ryDeg * kDegToRad), cy = std::cos(ryDeg * kDegToRad);
    const double sz = std::sin(rzDeg * kDegToRad), cz = std::cos(rzDeg * kDegToRad);
    auto& c = cf.Components;
    c[3]  = static_cast<float>(cy * cz + sy * sx * sz);
    c[4]  = static_cast<float>(-cy * sz + sy * sx * cz);
    c[5]  = static_cast<float>(sy * cx);
    c[6]  = static_cast<float>(cx * sz);
    c[7]  = static_cast<float>(cx * cz);
    c[8]  = static_cast<float>(-sx);
    c[9]  = static_cast<float>(-sy * cz + cy * sx * sz);
    c[10] = static_cast<float>(sy * sz + cy * sx * cz);
    c[11] = static_cast<float>(cy * cx);
}

static bool LooksBinary(const std::string& s) {
    for (unsigned char c : s)
        if (c < 0x20 && c != '\t' && c != '\r' && c != '\n')
            return true;
    return false;
}

// "1, 2, 3" / "[1, 2, 3]" / "{1, 2}" -> exactly `count` numbers, or nullopt.
static std::optional<std::vector<double>> ParseNumbers(QString text, int count) {
    text.remove('[').remove(']').remove('{').remove('}');
    QStringList parts = text.split(',', Qt::SkipEmptyParts);
    if (parts.size() != count)
        return std::nullopt;
    std::vector<double> out;
    for (const QString& part : parts) {
        bool ok = false;
        double v = part.trimmed().toDouble(&ok);
        if (!ok)
            return std::nullopt;
        out.push_back(v);
    }
    return out;
}

static QString FallbackTypeName(const Roblox::Property& prop) {
    if (!prop.XmlToken.empty())
        return "<" + QString::fromStdString(prop.XmlToken) + ">";
    return QString("<type %1>").arg(static_cast<int>(prop.Type));
}

struct RenderedValue {
    QString Text;
    bool HasSwatch { false };
    QColor Swatch;
    bool IsBool { false };
    bool BoolValue { false };
    bool Editable { false };
    QString EnumType; // non-empty = an enum property; the row edits through a member dropdown
    struct Child {
        QString Label;
        QString Text;
        int Component { kCompWhole };
        bool Editable { false };
    };
    QList<Child> Children;
};

static const Roblox::BrickColors::BrickColorInfo* NearestBrickColor(const Roblox::DataTypes::Color3uint8& color) {
    const Roblox::BrickColors::BrickColorInfo* best = nullptr;
    int bestDistance = 255 * 255 * 3 + 1;
    for (const auto& info : Roblox::BrickColors::kInfoMap) {
        const int dr = static_cast<int>(info.Color8.R) - color.R;
        const int dg = static_cast<int>(info.Color8.G) - color.G;
        const int db = static_cast<int>(info.Color8.B) - color.B;
        const int distance = dr * dr + dg * dg + db * db;
        if (distance < bestDistance) {
            bestDistance = distance;
            best = &info;
        }
    }
    return best;
}

static RenderedValue RenderBrickColorNumber(int32_t number) {
    RenderedValue out;
    out.Editable = true;
    const Roblox::BrickColors::BrickColorInfo* info = Roblox::BrickColors::FindByNumber(number);
    if (info != nullptr) {
        out.Text = QString::fromStdString(std::string(info->Name));
        Roblox::DataTypes::Color3 c = info->Color();
        out.HasSwatch = true;
        out.Swatch = QColor(std::clamp(static_cast<int>(std::lround(c.R * 255.0f)), 0, 255),
                            std::clamp(static_cast<int>(std::lround(c.G * 255.0f)), 0, 255),
                            std::clamp(static_cast<int>(std::lround(c.B * 255.0f)), 0, 255));
    } else {
        out.Text = QString::number(number);
    }
    return out;
}

static RenderedValue RenderPropertyValue(const std::string& className, const std::string& propName,
                                  const Roblox::Property& prop) {
    RenderedValue out;

#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
    // Enum properties are bare uint32 tokens; EnumType turns the row into a member dropdown.
    if (const auto* u = prop.CastValue<uint32_t>()) {
        const Roblox::PropertyDescriptor* descriptor = Roblox::FindProperty(className, propName);
        if (descriptor != nullptr && descriptor->Category == "Enum") {
            out.EnumType = QString::fromStdString(std::string(descriptor->Type));
            out.Editable = true;
            out.Text = QString::number(*u); // fallback: a token the dump does not know
            if (const Roblox::EnumInfo* info = Roblox::FindEnumInfo(descriptor->Type)) {
                for (const Roblox::EnumItemInfo& item : info->Items) {
                    if (item.Value == static_cast<int32_t>(*u)) {
                        out.Text = QString::fromStdString(std::string(item.Name));
                        break;
                    }
                }
            }
            return out;
        }
    }
#endif

    // Before the string branch: binary files store Source as a plain String column.
    if (IsScriptSourceProperty(propName, prop)) {
        QString firstLine = QString::fromStdString(*ScriptSourceFromProperty(prop))
                                .section('\n', 0, 0).trimmed();
        if (firstLine.length() > 40)
            firstLine = firstLine.left(40).trimmed() + QStringLiteral("…");
        else if (!firstLine.isEmpty())
            firstLine += QStringLiteral("…");
        out.Text = firstLine;
        return out; // Editable stays false: the code editor is the only write path
    }

    if (const auto* b = prop.CastValue<bool>()) {
        out.IsBool = true;
        out.BoolValue = *b;
        out.Editable = true;
        return out;
    }
    if (const auto* i = prop.CastValue<int32_t>()) {
        // Legacy XML serializes BrickColor as a bare <int>; give it the palette treatment.
        if (propName == "BrickColor")
            return RenderBrickColorNumber(*i);
        out.Text = QString::number(*i);
        out.Editable = true;
        return out;
    }
    if (const auto* u = prop.CastValue<uint32_t>()) { out.Text = QString::number(*u); out.Editable = true; return out; }
    if (const auto* l = prop.CastValue<int64_t>())  { out.Text = QString::number(*l); out.Editable = true; return out; }
    if (const auto* f = prop.CastValue<float>())    { out.Text = FormatNumber(*f); out.Editable = true; return out; }
    if (const auto* d = prop.CastValue<double>())   { out.Text = FormatNumber(*d); out.Editable = true; return out; }

    if (const auto* s = prop.CastValue<std::string>()) {
        if (LooksBinary(*s)) {
            out.Text = QString("<%1 bytes>").arg(s->size());
        } else {
            out.Text = QString::fromStdString(*s);
            out.Editable = true;
        }
        return out;
    }

    if (const auto* v3 = prop.CastValue<Roblox::DataTypes::Vector3>()) {
        out.Text = FormatVector3(*v3);
        out.Editable = true;
        out.Children = {
            {"X", FormatNumber(v3->X), kCompX, true},
            {"Y", FormatNumber(v3->Y), kCompY, true},
            {"Z", FormatNumber(v3->Z), kCompZ, true},
        };
        return out;
    }
    if (const auto* v2 = prop.CastValue<Roblox::DataTypes::Vector2>()) {
        out.Text = FormatNumber(v2->X) + ", " + FormatNumber(v2->Y);
        out.Editable = true;
        out.Children = {
            {"X", FormatNumber(v2->X), kCompX, true},
            {"Y", FormatNumber(v2->Y), kCompY, true},
        };
        return out;
    }

    if (const auto* c3 = prop.CastValue<Roblox::DataTypes::Color3>()) {
        int r = std::clamp(static_cast<int>(std::lround(c3->R * 255.0f)), 0, 255);
        int g = std::clamp(static_cast<int>(std::lround(c3->G * 255.0f)), 0, 255);
        int b = std::clamp(static_cast<int>(std::lround(c3->B * 255.0f)), 0, 255);
        out.Text = QString("[%1, %2, %3]").arg(r).arg(g).arg(b);
        out.HasSwatch = true;
        out.Swatch = QColor(r, g, b);
        out.Editable = true;
        return out;
    }
    if (const auto* c8 = prop.CastValue<Roblox::DataTypes::Color3uint8>()) {
        out.Text = QString("[%1, %2, %3]").arg(c8->R).arg(c8->G).arg(c8->B);
        out.HasSwatch = true;
        out.Swatch = QColor(c8->R, c8->G, c8->B);
        out.Editable = true;
        return out;
    }

    if (const auto* bc = prop.CastValue<Roblox::DataTypes::BrickColor>())
        return RenderBrickColorNumber(bc->Number);

    if (const auto* cf = prop.CastValue<Roblox::DataTypes::CFrame>()) {
        Roblox::DataTypes::Vector3 pos(cf->Components[0], cf->Components[1], cf->Components[2]);
        double rx = 0, ry = 0, rz = 0;
        CFrameToOrientationDegrees(*cf, rx, ry, rz);
        out.Text = FormatVector3(pos);
        out.Editable = true; // editing the top row moves the position
        out.Children = {
            {"Position", FormatVector3(pos), kCompCFramePos, true},
            {"Orientation", FormatNumber(rx) + ", " + FormatNumber(ry) + ", " + FormatNumber(rz),
             kCompCFrameRot, true},
        };
        return out;
    }

    if (const auto* ud = prop.CastValue<Roblox::DataTypes::UDim>()) {
        out.Text = QString("%1, %2").arg(FormatNumber(ud->Scale)).arg(ud->Offset);
        out.Editable = true;
        return out;
    }
    if (const auto* ud2 = prop.CastValue<Roblox::DataTypes::UDim2>()) {
        out.Text = QString("{%1, %2}, {%3, %4}")
                       .arg(FormatNumber(ud2->X.Scale)).arg(ud2->X.Offset)
                       .arg(FormatNumber(ud2->Y.Scale)).arg(ud2->Y.Offset);
        out.Editable = true;
        out.Children = {
            {"X", QString("%1, %2").arg(FormatNumber(ud2->X.Scale)).arg(ud2->X.Offset), kCompUDim2X, true},
            {"Y", QString("%1, %2").arg(FormatNumber(ud2->Y.Scale)).arg(ud2->Y.Offset), kCompUDim2Y, true},
        };
        return out;
    }

    if (const auto* ps = prop.CastValue<Roblox::DataTypes::ProtectedString>()) {
        out.Text = QString("<ProtectedString (%1 bytes)>").arg(ps->RawBuffer.size());
        return out;
    }
    if (const auto* uid = prop.CastValue<Roblox::DataTypes::UniqueId>()) {
        out.Text = QString::fromStdString(uid->ToString());
        return out;
    }
    if (const auto* ss = prop.CastValue<Roblox::DataTypes::SharedString>()) {
        out.Text = QString("<SharedString (%1 bytes)>").arg(ss->Value.size());
        return out;
    }
    if (const auto* cid = prop.CastValue<Roblox::DataTypes::ContentId>()) {
        out.Text = QString::fromStdString(cid->Uri);
        out.Editable = true;
        return out;
    }
    if (const auto* content = prop.CastValue<Roblox::DataTypes::Content>()) {
        if (content->SourceType == Roblox::DataTypes::ContentSourceType::Uri) {
            out.Text = QString::fromStdString(content->Uri);
            out.Editable = true;
        } else if (content->SourceType == Roblox::DataTypes::ContentSourceType::None) {
            out.Text = "Content.none";
        } else {
            out.Text = "<Content (object)>";
        }
        return out;
    }

    // Ref properties resolve to a live instance after the document's reference pass.
    if (const auto* ref = prop.CastValue<Roblox::Instance*>()) {
        out.Text = *ref != nullptr ? QString::fromStdString((*ref)->Name) : QString();
        return out;
    }

    out.Text = FallbackTypeName(prop);
    return out;
}

// Grid hairlines (stylesheet borders render unreliably) + dropdown editors for enum rows.
class PropertyGridDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        QStyledItemDelegate::paint(painter, option, index);
        painter->save();
        painter->setPen(option.palette.color(QPalette::Mid));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
        if (index.column() == 0)
            painter->drawLine(option.rect.topRight(), option.rect.bottomRight());
        painter->restore();
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override {
#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
        const QString enumType = index.siblingAtColumn(0).data(kRoleEnumType).toString();
        if (index.column() == 1 && !enumType.isEmpty()) {
            if (const Roblox::EnumInfo* info = Roblox::FindEnumInfo(enumType.toStdString())) {
                auto* combo = new QComboBox(parent);
                for (const Roblox::EnumItemInfo& item : info->Items)
                    combo->addItem(QString::fromStdString(std::string(item.Name)));
                // Commit immediately on pick.
                auto* self = const_cast<PropertyGridDelegate*>(this);
                connect(combo, &QComboBox::activated, combo, [self, combo]() {
                    emit self->commitData(combo);
                    emit self->closeEditor(combo);
                });
                return combo;
            }
        }
#endif
        return QStyledItemDelegate::createEditor(parent, option, index);
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        if (auto* combo = qobject_cast<QComboBox*>(editor)) {
            const int current = combo->findText(index.data(Qt::DisplayRole).toString());
            if (current >= 0)
                combo->setCurrentIndex(current);
            return;
        }
        QStyledItemDelegate::setEditorData(editor, index);
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override {
        if (auto* combo = qobject_cast<QComboBox*>(editor)) {
            model->setData(index, combo->currentText(), Qt::EditRole);
            return;
        }
        QStyledItemDelegate::setModelData(editor, model, index);
    }
};

static QIcon SwatchIcon(const QColor& color) {
    QPixmap pm(12, 12);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.fillRect(1, 1, 10, 10, color);
    p.setPen(QColor(90, 90, 90));
    p.drawRect(1, 1, 9, 9);
    return QIcon(pm);
}

// Display-only alias mapping; renaming the serialized names themselves corrupts files.
static QString DisplayNameForProperty(const std::string& serializedName) {
    static const std::unordered_map<std::string, const char*> kAliases = {
        {"Color3uint8", "Color"}, {"size", "Size"}, {"shape", "Shape"},
        {"Health_XML", "Health"}, {"MaterialVariantSerialized", "MaterialVariant"},
        {"formFactorRaw", "FormFactor"}, {"CoordinateFrame", "CFrame"},
    };
    if (auto it = kAliases.find(serializedName); it != kAliases.end())
        return it->second;
    return QString::fromStdString(serializedName);
}

// BrickColor edits accept a palette name or a number.
static std::optional<int32_t> ParseBrickColorText(const QString& text) {
    bool ok = false;
    int number = text.trimmed().toInt(&ok);
    if (ok)
        return number;
    const auto* info = Roblox::BrickColors::FindByName(text.trimmed().toStdString());
    if (info != nullptr)
        return info->Number();
    return std::nullopt;
}

// A modal palette picker listing every BrickColor with its swatch; nullopt on cancel.
static std::optional<int32_t> PickBrickColor(QWidget* parent, int32_t current) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Select BrickColor");
    dialog.resize(280, 420);
    auto* layout = new QVBoxLayout(&dialog);

    auto* list = new QListWidget(&dialog);
    for (const auto& info : Roblox::BrickColors::kInfoMap) {
        auto* entry = new QListWidgetItem(QString::fromStdString(std::string(info.Name)));
        Roblox::DataTypes::Color3 c = info.Color();
        entry->setIcon(SwatchIcon(QColor(
            std::clamp(static_cast<int>(std::lround(c.R * 255.0f)), 0, 255),
            std::clamp(static_cast<int>(std::lround(c.G * 255.0f)), 0, 255),
            std::clamp(static_cast<int>(std::lround(c.B * 255.0f)), 0, 255))));
        entry->setData(Qt::UserRole, info.Number());
        list->addItem(entry);
        if (info.Number() == current)
            list->setCurrentItem(entry);
    }
    layout->addWidget(list, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(list, &QListWidget::itemDoubleClicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted || list->currentItem() == nullptr)
        return std::nullopt;
    return list->currentItem()->data(Qt::UserRole).toInt();
}

// The asset id inside a Content-ish URI ("rbxassetid://123", ".../asset/?id=123"), or 0.
static int64_t AssetIdFromUriText(const QString& uri) {
    static const QRegularExpression kPatterns[] = {
        QRegularExpression("rbxassetid://(\\d+)"),
        QRegularExpression("[?&]id=(\\d+)"),
    };
    for (const QRegularExpression& re : kPatterns) {
        QRegularExpressionMatch match = re.match(uri);
        if (match.hasMatch())
            return match.captured(1).toLongLong();
    }
    return 0;
}

static QString UriTextFromProperty(const Roblox::Property& prop) {
    if (const auto* cid = prop.CastValue<Roblox::DataTypes::ContentId>())
        return QString::fromStdString(cid->Uri);
    if (const auto* content = prop.CastValue<Roblox::DataTypes::Content>())
        return content->SourceType == Roblox::DataTypes::ContentSourceType::Uri ? QString::fromStdString(content->Uri)
                                                                 : QString();
    if (const auto* s = prop.CastValue<std::string>())
        return QString::fromStdString(*s);
    return {};
}

// Play/pause + scrubber, loading through the resolver on first play. Temp file rather than a
// stream: the real extension is what lets the media backend pick the right demuxer.
class SoundPreviewWidget : public QWidget {
public:
    SoundPreviewWidget(int64_t assetId, AssetDataResolver resolver, QWidget* parent)
        : QWidget(parent), mAssetId(assetId), mResolver(std::move(resolver)) {
        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(4);

        mPlayButton = new QToolButton(this);
        mPlayButton->setIcon(QIcon(":/images/silk/control_play.png"));
        mPlayButton->setAutoRaise(true);
        mPlayButton->setToolTip("Play");
        row->addWidget(mPlayButton);

        mSeek = new QSlider(Qt::Horizontal, this);
        mSeek->setRange(0, 0);
        row->addWidget(mSeek, 1);

        connect(mPlayButton, &QToolButton::clicked, this, [this]() { TogglePlay(); });
        connect(mSeek, &QSlider::sliderMoved, this, [this](int value) {
            if (mPlayer != nullptr)
                mPlayer->setPosition(value);
        });
    }

    ~SoundPreviewWidget() override {
        // Release the file before ~QTemporaryFile runs; Windows won't delete an open file.
        if (mPlayer != nullptr)
            mPlayer->setSource(QUrl());
    }
private:
    void TogglePlay() {
        if (mPlayer == nullptr && !Load())
            return;
        if (mPlayer->playbackState() == QMediaPlayer::PlayingState)
            mPlayer->pause();
        else
            mPlayer->play();
    }

    bool Load() {
        std::vector<unsigned char> data;
        if (!mResolver || !mResolver(mAssetId, &data) || data.empty()) {
            mPlayButton->setEnabled(false);
            mPlayButton->setToolTip("This sound's data is not in any open database.");
            return false;
        }

        mTempFile = new QTemporaryFile(this);
        mTempFile->setFileTemplate(QDir::tempPath() + "/nwpreview_XXXXXX." + DetectAssetExtension(data));
        if (!mTempFile->open())
            return false;
        mTempFile->write(reinterpret_cast<const char*>(data.data()), static_cast<qint64>(data.size()));
        mTempFile->flush();
        mTempFile->close();

        mPlayer = new QMediaPlayer(this);
        mAudioOutput = new QAudioOutput(this);
        mPlayer->setAudioOutput(mAudioOutput);
        connect(mPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
            bool playing = state == QMediaPlayer::PlayingState;
            mPlayButton->setIcon(QIcon(playing ? ":/images/silk/control_pause.png"
                                               : ":/images/silk/control_play.png"));
            mPlayButton->setToolTip(playing ? "Pause" : "Play");
        });
        connect(mPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
            mSeek->setRange(0, static_cast<int>(duration));
        });
        connect(mPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
            if (!mSeek->isSliderDown())
                mSeek->setValue(static_cast<int>(position));
        });

        mPlayer->setSource(QUrl::fromLocalFile(mTempFile->fileName()));
        return true;
    }

    int64_t mAssetId;
    AssetDataResolver mResolver;
    QToolButton* mPlayButton;
    QSlider* mSeek;
    QMediaPlayer* mPlayer { nullptr };
    QAudioOutput* mAudioOutput { nullptr };
    QTemporaryFile* mTempFile { nullptr };
};

PropertiesWidget::PropertiesWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    mFilter = new QLineEdit(this);
    mFilter->setPlaceholderText("Filter Properties (Ctrl+Shift+P)");
    mFilter->setClearButtonEnabled(true);
    layout->addWidget(mFilter);

    mTree = new QTreeWidget(this);
    mTree->setColumnCount(2);
    mTree->header()->hide();
    mTree->setSelectionMode(QAbstractItemView::SingleSelection);
    mTree->setEditTriggers(QAbstractItemView::NoEditTriggers); // edits go through editItem only
    mTree->setUniformRowHeights(true);
    mTree->setIndentation(14);
    mTree->setRootIsDecorated(true);
    // Studio look: flat rows, no alternating stripes; the grid lines come from the delegate.
    mTree->setAlternatingRowColors(false);
    mTree->setStyleSheet(
        "QTreeWidget { border: none; }"
        "QTreeWidget::item { padding: 2px 3px; }");
    mTree->setItemDelegate(new PropertyGridDelegate(mTree));
    layout->addWidget(mTree, 1);

    connect(mFilter, &QLineEdit::textChanged, this, &PropertiesWidget::ApplyFilter);
    auto* focusFilter = new QShortcut(QKeySequence("Ctrl+Shift+P"), this);
    focusFilter->setContext(Qt::WidgetWithChildrenShortcut);
    connect(focusFilter, &QShortcut::activated, [this]() { mFilter->setFocus(); });

    connect(mTree, &QTreeWidget::itemChanged, this, &PropertiesWidget::OnItemChanged);
    connect(mTree, &QTreeWidget::itemDoubleClicked, this, &PropertiesWidget::OnItemDoubleClicked);
}

void PropertiesWidget::SetInstance(Roblox::Instance* instance) {
    if (instance == nullptr)
        SetInstances({});
    else
        SetInstances({instance});
}

void PropertiesWidget::SetInstances(std::vector<Roblox::Instance*> instances) {
    mInstances = std::move(instances);
    mInstance = mInstances.empty() ? nullptr : mInstances.front();
    Rebuild();
    ApplyFilter(mFilter->text());
}

void PropertiesWidget::Rebuild() {
    mRebuilding = true;
    mTree->clear();
    if (mInstance == nullptr) {
        mRebuilding = false;
        return;
    }

    static const QStringList kPinnedOrder = {"Appearance", "Data", "Transform", "Pivot", "Behavior",
                                             "Collision", "Part", "Surface", "Text", "Camera"};
    std::map<QString, QTreeWidgetItem*> categories;
    auto categoryItem = [&](const QString& name) -> QTreeWidgetItem* {
        if (auto it = categories.find(name); it != categories.end())
            return it->second;
        auto* cat = new QTreeWidgetItem();
        cat->setText(0, name);
        QFont bold = cat->font(0);
        bold.setBold(true);
        cat->setFont(0, bold);
        cat->setFirstColumnSpanned(true);
        cat->setBackground(0, palette().brush(QPalette::Window));
        cat->setBackground(1, palette().brush(QPalette::Window));
        cat->setFlags(Qt::ItemIsEnabled);
        categories[name] = cat;
        return cat;
    };

    const QBrush disabledText = palette().brush(QPalette::Disabled, QPalette::Text);
    auto addRow = [&](const QString& category, const QString& displayName,
                      const std::string& serializedName, const RenderedValue& value,
                      bool readOnly) -> QTreeWidgetItem* {
        QTreeWidgetItem* row = new QTreeWidgetItem(categoryItem(category));
        row->setText(0, displayName);
        row->setData(0, kRoleProp, QString::fromStdString(serializedName));
        row->setData(0, kRoleComponent, kCompWhole);
        row->setData(0, kRoleEditable, !readOnly && value.Editable);
        if (!value.EnumType.isEmpty())
            row->setData(0, kRoleEnumType, value.EnumType);
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (value.IsBool) {
            if (!readOnly && value.Editable)
                flags |= Qt::ItemIsUserCheckable;
            row->setFlags(flags);
            row->setCheckState(1, value.BoolValue ? Qt::Checked : Qt::Unchecked);
        } else {
            if (!readOnly && value.Editable)
                flags |= Qt::ItemIsEditable;
            row->setFlags(flags);
            row->setText(1, value.Text);
            if (value.HasSwatch)
                row->setIcon(1, SwatchIcon(value.Swatch));
        }
        if (readOnly) {
            row->setForeground(0, disabledText);
            row->setForeground(1, disabledText);
        }
        for (const RenderedValue::Child& childValue : value.Children) {
            auto* child = new QTreeWidgetItem(row);
            child->setText(0, childValue.Label);
            child->setText(1, childValue.Text);
            child->setData(0, kRoleProp, QString::fromStdString(serializedName));
            child->setData(0, kRoleComponent, childValue.Component);
            child->setData(0, kRoleEditable, !readOnly && childValue.Editable);
            Qt::ItemFlags childFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            if (!readOnly && childValue.Editable)
                childFlags |= Qt::ItemIsEditable;
            child->setFlags(childFlags);
        }
        return row;
    };

    // Synthetic ClassName/Parent rows; a RobloxFile parent shows blank, as do disagreements.
    RenderedValue classNameValue;
    classNameValue.Text = QString::fromStdString(mInstance->ClassName);
    for (size_t i = 1; i < mInstances.size(); i++)
        if (mInstances[i]->ClassName != mInstance->ClassName)
            classNameValue.Text.clear();
    addRow("Data", "ClassName", "", classNameValue, true);
    RenderedValue parentValue;
    if (Roblox::Instance* instParent = mInstance->GetParent()) {
        if (dynamic_cast<Roblox::RobloxFile*>(instParent) == nullptr)
            parentValue.Text = QString::fromStdString(instParent->Name.empty() ? instParent->ClassName
                                                                               : instParent->Name);
    }
    for (size_t i = 1; i < mInstances.size(); i++)
        if (mInstances[i]->GetParent() != mInstance->GetParent())
            parentValue.Text.clear();
    addRow("Data", "Parent", "", parentValue, true);

    // Modern files serialize only Color3uint8; the BrickColor row is computed from it.
    if (mInstance->GetProperty("BrickColor") == nullptr) {
        Roblox::Property* colorProp = mInstance->GetProperty("Color3uint8");
        const auto* c8 = colorProp != nullptr ? colorProp->CastValue<Roblox::DataTypes::Color3uint8>() : nullptr;
        bool allHave = c8 != nullptr;
        bool colorMixed = false;
        for (size_t i = 1; allHave && i < mInstances.size(); i++) {
            Roblox::Property* otherProp = mInstances[i]->GetProperty("Color3uint8");
            const auto* other = otherProp != nullptr ? otherProp->CastValue<Roblox::DataTypes::Color3uint8>() : nullptr;
            if (other == nullptr)
                allHave = false;
            else if (other->R != c8->R || other->G != c8->G || other->B != c8->B)
                colorMixed = true;
        }
        if (allHave) {
            if (const auto* nearest = NearestBrickColor(*c8)) {
                RenderedValue bcValue = RenderBrickColorNumber(nearest->Number());
                bcValue.Editable = false; // no inline text edit; double-click opens the palette
                if (colorMixed) {
                    bcValue.Text.clear();
                    bcValue.HasSwatch = false;
                }
                QTreeWidgetItem* bcRow =
                    addRow("Appearance", "BrickColor", "Color3uint8", bcValue, false);
                bcRow->setData(0, kRoleComponent, kCompBrickColorSynthetic);
            }
        }
    }

    QList<QTreeWidgetItem*> expandAfter; // rows re-expanded after the depth pass (sound previews)

    for (const auto& [serializedName, prop] : mInstance->GetProperties()) {
        if (serializedName == Roblox::Instance::kAttributesProperty)
            continue; // the raw attribute blob; Studio shows attributes in their own panel

        // Multi-select, only the intersection survives. Differing values render blank.
        RenderedValue value = RenderPropertyValue(mInstance->ClassName, serializedName, prop);
        bool presentOnAll = true;
        bool boolMixed = false;
        for (size_t i = 1; i < mInstances.size(); i++) {
            Roblox::Instance* other = mInstances[i];
            Roblox::Property* otherProp = other->GetProperty(serializedName);
            if (otherProp == nullptr || otherProp->Type != prop.Type) {
                presentOnAll = false;
                break;
            }
            RenderedValue otherValue = RenderPropertyValue(other->ClassName, serializedName, *otherProp);
            // Frame.Style and TextButton.Style are different enums despite the same name+type.
            if (otherValue.EnumType != value.EnumType) {
                presentOnAll = false;
                break;
            }
            if (value.IsBool) {
                if (otherValue.BoolValue != value.BoolValue)
                    boolMixed = true;
            } else if (otherValue.Text != value.Text) {
                value.Text.clear();
                value.HasSwatch = false;
            }
            for (int c = 0; c < value.Children.size() && c < otherValue.Children.size(); c++)
                if (otherValue.Children[c].Text != value.Children[c].Text)
                    value.Children[c].Text.clear();
        }
        if (!presentOnAll)
            continue;

        QString displayName = DisplayNameForProperty(serializedName);
        QTreeWidgetItem* row = addRow(CategoryForPropertyName(displayName), displayName, serializedName,
                                      value, false);
        if (boolMixed)
            row->setCheckState(1, Qt::PartiallyChecked);

        // A Sound's SoundId gets Studio's inline preview: a play-button row nested under the id.
        if (serializedName == "SoundId" && mAssetResolver && mInstances.size() == 1) {
            int64_t soundAssetId = AssetIdFromUriText(UriTextFromProperty(prop));
            if (soundAssetId > 0) {
                auto* previewRow = new QTreeWidgetItem(row);
                previewRow->setText(0, "Preview");
                previewRow->setFlags(Qt::ItemIsEnabled);
                mTree->setItemWidget(previewRow, 1,
                                     new SoundPreviewWidget(soundAssetId, mAssetResolver, mTree));
                expandAfter.append(row);
            }
        }
    }

    for (const QString& pinned : kPinnedOrder) {
        if (auto it = categories.find(pinned); it != categories.end()) {
            mTree->addTopLevelItem(it->second);
            categories.erase(it);
        }
    }
    for (auto& [name, item] : categories)
        mTree->addTopLevelItem(item);

    mTree->expandToDepth(0);
    for (QTreeWidgetItem* row : expandAfter)
        row->setExpanded(true);
    mTree->resizeColumnToContents(0);
    mRebuilding = false;
}

void PropertiesWidget::ApplyFilter(const QString& text) {
    const QString needle = text.trimmed();
    for (int c = 0; c < mTree->topLevelItemCount(); c++) {
        QTreeWidgetItem* cat = mTree->topLevelItem(c);
        int visibleRows = 0;
        for (int r = 0; r < cat->childCount(); r++) {
            QTreeWidgetItem* row = cat->child(r);
            bool match = needle.isEmpty() || row->text(0).contains(needle, Qt::CaseInsensitive);
            row->setHidden(!match);
            if (match)
                visibleRows++;
        }
        cat->setHidden(visibleRows == 0);
    }
}

void PropertiesWidget::OnItemDoubleClicked(QTreeWidgetItem* item, int column) {
    if (mInstance == nullptr || item == nullptr)
        return;
    QString propName = item->data(0, kRoleProp).toString();
    if (propName.isEmpty())
        return;
    Roblox::Property* prop = mInstance->GetProperty(propName.toStdString());
    if (prop == nullptr)
        return;

    if (IsScriptSourceProperty(propName.toStdString(), *prop))
        return;

    const int component = item->data(0, kRoleComponent).toInt();

    // Writes one picked value into every selected instance's copy of the property.
    auto applyToSelection = [&](auto&& write) -> bool {
        bool any = false;
        for (Roblox::Instance* target : mInstances) {
            Roblox::Property* targetProp = target->GetProperty(propName.toStdString());
            if (targetProp == nullptr || !write(*targetProp))
                continue;
            targetProp->RawBuffer.clear();
            emit InstanceEdited(target);
            any = true;
        }
        return any;
    };
    
    if (component == kCompBrickColorSynthetic) {
        auto* c8 = prop->CastValue<Roblox::DataTypes::Color3uint8>();
        if (c8 == nullptr)
            return;
        const auto* nearest = NearestBrickColor(*c8);
        if (auto number = PickBrickColor(this, nearest != nullptr ? nearest->Number() : 194)) {
            if (const auto* picked = Roblox::BrickColors::FindByNumber(*number)) {
                bool any = applyToSelection([&](Roblox::Property& p) {
                    auto* v = p.CastValue<Roblox::DataTypes::Color3uint8>();
                    if (v == nullptr)
                        return false;
                    *v = picked->Color8;
                    return true;
                });
                if (any) {
                    RefreshSyntheticBrickColorRow();
                    if (QTreeWidgetItem* colorRow = FindPropertyRow(propName.toStdString(), kCompWhole))
                        RefreshRowFromValue(colorRow, propName.toStdString());
                }
            }
        }
        return;
    }

    if (component == kCompWhole) {
        // Colors get the Qt color picker, BrickColors the palette list - like Studio.
        if (auto* c3 = prop->CastValue<Roblox::DataTypes::Color3>()) {
            QColor picked = QColorDialog::getColor(
                QColor(std::clamp(static_cast<int>(std::lround(c3->R * 255.0f)), 0, 255),
                       std::clamp(static_cast<int>(std::lround(c3->G * 255.0f)), 0, 255),
                       std::clamp(static_cast<int>(std::lround(c3->B * 255.0f)), 0, 255)),
                this, "Select Color");
            if (picked.isValid() && applyToSelection([&](Roblox::Property& p) {
                    auto* v = p.CastValue<Roblox::DataTypes::Color3>();
                    if (v == nullptr)
                        return false;
                    *v = Roblox::DataTypes::Color3(picked.redF(), picked.greenF(), picked.blueF());
                    return true;
                }))
                RefreshRowFromValue(item, propName.toStdString());
            return;
        }
        if (auto* c8 = prop->CastValue<Roblox::DataTypes::Color3uint8>()) {
            QColor picked = QColorDialog::getColor(QColor(c8->R, c8->G, c8->B), this, "Select Color");
            if (picked.isValid() && applyToSelection([&](Roblox::Property& p) {
                    auto* v = p.CastValue<Roblox::DataTypes::Color3uint8>();
                    if (v == nullptr)
                        return false;
                    *v = Roblox::DataTypes::Color3uint8(static_cast<uint8_t>(picked.red()),
                                         static_cast<uint8_t>(picked.green()),
                                         static_cast<uint8_t>(picked.blue()));
                    return true;
                })) {
                RefreshRowFromValue(item, propName.toStdString());
                RefreshSyntheticBrickColorRow();
            }
            return;
        }
        if (auto* bc = prop->CastValue<Roblox::DataTypes::BrickColor>()) {
            if (auto number = PickBrickColor(this, bc->Number)) {
                if (applyToSelection([&](Roblox::Property& p) {
                        auto* v = p.CastValue<Roblox::DataTypes::BrickColor>();
                        if (v == nullptr)
                            return false;
                        v->Number = *number;
                        return true;
                    }))
                    RefreshRowFromValue(item, propName.toStdString());
            }
            return;
        }
        if (auto* i32 = prop->CastValue<int32_t>(); i32 != nullptr && propName == "BrickColor") {
            if (auto number = PickBrickColor(this, *i32)) {
                if (applyToSelection([&](Roblox::Property& p) {
                        auto* v = p.CastValue<int32_t>();
                        if (v == nullptr)
                            return false;
                        *v = *number;
                        return true;
                    }))
                    RefreshRowFromValue(item, propName.toStdString());
            }
            return;
        }
    }

    if (column == 1 && item->data(0, kRoleEditable).toBool() && (item->flags() & Qt::ItemIsEditable))
        mTree->editItem(item, 1);
}

void PropertiesWidget::OnItemChanged(QTreeWidgetItem* item, int column) {
    if (mRebuilding || mInstance == nullptr || column != 1)
        return;
    if (!item->data(0, kRoleEditable).toBool())
        return;
    ApplyEdit(item);
}

bool PropertiesWidget::ApplyEdit(QTreeWidgetItem* item) {
    const std::string propName = item->data(0, kRoleProp).toString().toStdString();
    const int component = item->data(0, kRoleComponent).toInt();
    const QString text = item->text(1).trimmed();

    // One typed write per selected instance.
    auto applyToTarget = [&](Roblox::Instance* target) -> bool {
        Roblox::Property* prop = target->GetProperty(propName);
        if (prop == nullptr)
            return false;
        bool applied = false;
        auto finish = [&]() {
            // A kept raw buffer would make the writer re-emit the replaced value's encoding.
            prop->RawBuffer.clear();
            applied = true;
        };

        if (auto* b = prop->CastValue<bool>()) {
            *b = item->checkState(1) == Qt::Checked;
            finish();
        } else if (auto* i32 = prop->CastValue<int32_t>()) {
            if (propName == "BrickColor") {
                if (auto number = ParseBrickColorText(text)) {
                    *i32 = *number;
                    finish();
                }
            } else if (auto nums = ParseNumbers(text, 1)) {
                *i32 = static_cast<int32_t>(std::llround((*nums)[0]));
                finish();
            }
        } else if (auto* u32 = prop->CastValue<uint32_t>()) {
            bool handledAsEnum = false;
#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
            // Enum rows arrive as the member name the dropdown chose.
            const QString enumType = item->data(0, kRoleEnumType).toString();
            if (!enumType.isEmpty()) {
                const std::string textStd = text.toStdString();
                if (const Roblox::EnumInfo* info = Roblox::FindEnumInfo(enumType.toStdString())) {
                    for (const Roblox::EnumItemInfo& enumItem : info->Items) {
                        if (textStd == enumItem.Name) {
                            *u32 = static_cast<uint32_t>(enumItem.Value);
                            finish();
                            handledAsEnum = true;
                            break;
                        }
                    }
                }
            }
#endif
            if (!handledAsEnum) {
                if (auto nums = ParseNumbers(text, 1); nums && (*nums)[0] >= 0) {
                    *u32 = static_cast<uint32_t>(std::llround((*nums)[0]));
                    finish();
                }
            }
        } else if (auto* i64 = prop->CastValue<int64_t>()) {
            if (auto nums = ParseNumbers(text, 1)) {
                *i64 = static_cast<int64_t>(std::llround((*nums)[0]));
                finish();
            }
        } else if (auto* f = prop->CastValue<float>()) {
            if (auto nums = ParseNumbers(text, 1)) {
                *f = static_cast<float>((*nums)[0]);
                finish();
            }
        } else if (auto* d = prop->CastValue<double>()) {
            if (auto nums = ParseNumbers(text, 1)) {
                *d = (*nums)[0];
                finish();
            }
        } else if (auto* s = prop->CastValue<std::string>()) {
            // Name must go through SetName so the Instance field stays in sync.
            if (propName == "Name")
                target->SetName(text.toStdString());
            else
                *s = text.toStdString();
            finish();
        } else if (auto* v3 = prop->CastValue<Roblox::DataTypes::Vector3>()) {
            if (component == kCompWhole) {
                if (auto nums = ParseNumbers(text, 3)) {
                    *v3 = Roblox::DataTypes::Vector3(static_cast<float>((*nums)[0]), static_cast<float>((*nums)[1]),
                                      static_cast<float>((*nums)[2]));
                    finish();
                }
            } else if (component >= kCompX && component <= kCompZ) {
                if (auto nums = ParseNumbers(text, 1)) {
                    float v = static_cast<float>((*nums)[0]);
                    if (component == kCompX) v3->X = v;
                    if (component == kCompY) v3->Y = v;
                    if (component == kCompZ) v3->Z = v;
                    finish();
                }
            }
        } else if (auto* v2 = prop->CastValue<Roblox::DataTypes::Vector2>()) {
            if (component == kCompWhole) {
                if (auto nums = ParseNumbers(text, 2)) {
                    v2->X = static_cast<float>((*nums)[0]);
                    v2->Y = static_cast<float>((*nums)[1]);
                    finish();
                }
            } else if (auto nums = ParseNumbers(text, 1)) {
                (component == kCompX ? v2->X : v2->Y) = static_cast<float>((*nums)[0]);
                finish();
            }
        } else if (auto* c3 = prop->CastValue<Roblox::DataTypes::Color3>()) {
            if (auto nums = ParseNumbers(text, 3)) {
                *c3 = Roblox::DataTypes::Color3(static_cast<float>(std::clamp((*nums)[0], 0.0, 255.0) / 255.0),
                                 static_cast<float>(std::clamp((*nums)[1], 0.0, 255.0) / 255.0),
                                 static_cast<float>(std::clamp((*nums)[2], 0.0, 255.0) / 255.0));
                finish();
            }
        } else if (auto* c8 = prop->CastValue<Roblox::DataTypes::Color3uint8>()) {
            if (auto nums = ParseNumbers(text, 3)) {
                *c8 = Roblox::DataTypes::Color3uint8(static_cast<uint8_t>(std::clamp((*nums)[0], 0.0, 255.0)),
                                      static_cast<uint8_t>(std::clamp((*nums)[1], 0.0, 255.0)),
                                      static_cast<uint8_t>(std::clamp((*nums)[2], 0.0, 255.0)));
                finish();
            }
        } else if (auto* bc = prop->CastValue<Roblox::DataTypes::BrickColor>()) {
            if (auto number = ParseBrickColorText(text)) {
                bc->Number = *number;
                finish();
            }
        } else if (auto* cf = prop->CastValue<Roblox::DataTypes::CFrame>()) {
            if (component == kCompCFrameRot) {
                if (auto nums = ParseNumbers(text, 3)) {
                    OrientationDegreesToCFrame(*cf, (*nums)[0], (*nums)[1], (*nums)[2]);
                    finish();
                }
            } else { // whole row and the Position child both edit the position
                if (auto nums = ParseNumbers(text, 3)) {
                    cf->Components[0] = static_cast<float>((*nums)[0]);
                    cf->Components[1] = static_cast<float>((*nums)[1]);
                    cf->Components[2] = static_cast<float>((*nums)[2]);
                    finish();
                }
            }
        } else if (auto* ud = prop->CastValue<Roblox::DataTypes::UDim>()) {
            if (auto nums = ParseNumbers(text, 2)) {
                ud->Scale = static_cast<float>((*nums)[0]);
                ud->Offset = static_cast<int32_t>(std::llround((*nums)[1]));
                finish();
            }
        } else if (auto* ud2 = prop->CastValue<Roblox::DataTypes::UDim2>()) {
            if (component == kCompUDim2X || component == kCompUDim2Y) {
                if (auto nums = ParseNumbers(text, 2)) {
                    Roblox::DataTypes::UDim& axis = component == kCompUDim2X ? ud2->X : ud2->Y;
                    axis.Scale = static_cast<float>((*nums)[0]);
                    axis.Offset = static_cast<int32_t>(std::llround((*nums)[1]));
                    finish();
                }
            } else if (auto nums = ParseNumbers(text, 4)) {
                ud2->X = Roblox::DataTypes::UDim(static_cast<float>((*nums)[0]), static_cast<int32_t>(std::llround((*nums)[1])));
                ud2->Y = Roblox::DataTypes::UDim(static_cast<float>((*nums)[2]), static_cast<int32_t>(std::llround((*nums)[3])));
                finish();
            }
        } else if (auto* cid = prop->CastValue<Roblox::DataTypes::ContentId>()) {
            cid->Uri = text.toStdString();
            finish();
        } else if (auto* content = prop->CastValue<Roblox::DataTypes::Content>()) {
            if (content->SourceType == Roblox::DataTypes::ContentSourceType::Uri) {
                content->Uri = text.toStdString();
                finish();
            }
        }
        return applied;
    };

    bool anyApplied = false;
    for (Roblox::Instance* target : mInstances) {
        if (applyToTarget(target)) {
            anyApplied = true;
            emit InstanceEdited(target);
        }
    }

    if (mInstances.size() > 1) {
        // Re-run the mixed-value blanking; deferred because a rebuild destroys `item` mid-signal.
        QMetaObject::invokeMethod(this, [this]() {
            Rebuild();
            ApplyFilter(mFilter->text());
        }, Qt::QueuedConnection);
    } else {
        // Re-render from the now-current value, whether the edit stuck or not.
        RefreshRowFromValue(item, propName);
        if (anyApplied && propName == "Color3uint8")
            RefreshSyntheticBrickColorRow();
    }
    return anyApplied;
}

QTreeWidgetItem* PropertiesWidget::FindPropertyRow(const std::string& propName,
                                                           int component) const {
    const QString needle = QString::fromStdString(propName);
    for (int c = 0; c < mTree->topLevelItemCount(); c++) {
        QTreeWidgetItem* cat = mTree->topLevelItem(c);
        for (int r = 0; r < cat->childCount(); r++) {
            QTreeWidgetItem* row = cat->child(r);
            if (row->data(0, kRoleProp).toString() == needle &&
                row->data(0, kRoleComponent).toInt() == component)
                return row;
        }
    }
    return nullptr;
}

void PropertiesWidget::RefreshSyntheticBrickColorRow() {
    QTreeWidgetItem* row = FindPropertyRow("Color3uint8", kCompBrickColorSynthetic);
    if (row == nullptr || mInstance == nullptr)
        return;
    Roblox::Property* prop = mInstance->GetProperty("Color3uint8");
    const auto* c8 = prop != nullptr ? prop->CastValue<Roblox::DataTypes::Color3uint8>() : nullptr;
    const auto* nearest = c8 != nullptr ? NearestBrickColor(*c8) : nullptr;
    if (nearest == nullptr)
        return;
    RenderedValue fresh = RenderBrickColorNumber(nearest->Number());
    mRebuilding = true;
    row->setText(1, fresh.Text);
    if (fresh.HasSwatch)
        row->setIcon(1, SwatchIcon(fresh.Swatch));
    mRebuilding = false;
}

void PropertiesWidget::RefreshRowFromValue(QTreeWidgetItem* item, const std::string& propName) {
    Roblox::Property* prop = mInstance != nullptr ? mInstance->GetProperty(propName) : nullptr;
    if (prop == nullptr || item == nullptr)
        return;
    QTreeWidgetItem* propRow = item;
    while (propRow->parent() != nullptr && propRow->data(0, kRoleComponent).toInt() != kCompWhole)
        propRow = propRow->parent();
    RenderedValue fresh = RenderPropertyValue(mInstance->ClassName, propName, *prop);
    mRebuilding = true;
    if (fresh.IsBool) {
        propRow->setCheckState(1, fresh.BoolValue ? Qt::Checked : Qt::Unchecked);
    } else {
        propRow->setText(1, fresh.Text);
        if (fresh.HasSwatch)
            propRow->setIcon(1, SwatchIcon(fresh.Swatch));
    }
    for (int i = 0; i < propRow->childCount() && i < fresh.Children.size(); i++)
        propRow->child(i)->setText(1, fresh.Children[i].Text);
    mRebuilding = false;
}
