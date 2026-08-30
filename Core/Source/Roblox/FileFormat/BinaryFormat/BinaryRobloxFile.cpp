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
// File: BinaryRobloxFile.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Ported RobloxFiles.BinaryFormat.BinaryRobloxFile: chunks decoded into an object graph.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>

#include <algorithm>
#include <array>
#include <map>
#include <optional>

#include <NoobWarrior/Roblox/FileFormat/Utility/DefaultProperty.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/FontUtility.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <set>

#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
#include <NoobWarrior/Roblox/FileFormat/Generated/Registry.h>
#endif

using namespace NoobWarrior::Roblox;
using namespace NoobWarrior::Roblox::BinaryFormat;
using NoobWarrior::Roblox::Tokens::ParseInteger;

BinaryRobloxFile::BinaryRobloxFile() {
    Name = "Bin:";
    Referent = "-1";
    ParentLocked = true;
}

namespace {
constexpr std::array<unsigned char, 14> kMagic = {
    '<', 'r', 'o', 'b', 'l', 'o', 'x', '!', 0x89, 0xff, 0x0d, 0x0a, 0x1a, 0x0a,
};
constexpr size_t kMaximumObjectCount = 16 * 1024 * 1024;

void SetError(std::string *error, std::string message) {
    if (error != nullptr)
        *error = std::move(message);
}
} // namespace

Instance *BinaryRobloxFile::GetObject(int32_t referent) const {
    if (referent < 0 || static_cast<size_t>(referent) >= Objects.size())
        return nullptr;
    return Objects[static_cast<size_t>(referent)].get();
}

const INST *BinaryRobloxFile::FindClass(int32_t classIndex) const {
    for (const INST &descriptor : Classes) {
        if (descriptor.ClassIndex == classIndex)
            return &descriptor;
    }
    return nullptr;
}

Instance *BinaryRobloxFile::CreateInstance(const std::string &className,
                                          const std::string &name, Instance *parent) {
    Instance *instance = CreateObject(className);
    instance->Name = name;
    Property property;
    property.Name = "Name";
    property.Type = PropertyType::String;
    property.Value = name;
    instance->AddProperty(std::move(property));
    if (!instance->SetParent(parent == nullptr ? this : parent))
        return nullptr;
    return instance;
}

void BinaryRobloxFile::DestroySubtree(Instance *instance) {
    if (instance == nullptr)
        return;
    // Copy the child list: detaching mutates the parent's children while we walk it.
    for (Instance *child : instance->GetChildren())
        DestroySubtree(child);
    instance->SetParent(nullptr);
    int32_t referent = -1;
    if (ParseInteger(instance->Referent, referent) && referent >= 0 &&
        static_cast<size_t>(referent) < Objects.size()) {
        Objects[static_cast<size_t>(referent)].reset();
    }
}

std::vector<Instance *> BinaryRobloxFile::Roots() const {
    return GetChildren();
}

Instance *BinaryRobloxFile::CreateObject(const std::string &className) {
    auto object = std::make_unique<Instance>();
    object->ClassName = className;
    object->Name = className;
    object->Referent = std::to_string(Objects.size());
    Instance *raw = object.get();
    Objects.push_back(std::move(object));
    return raw;
}

Instance *BinaryRobloxFile::FindFirstOfClass(std::string_view className) const {
    for (const auto &object : Objects) {
        if (object != nullptr && object->ClassName == className)
            return object.get();
    }
    return nullptr;
}

bool BinaryRobloxFile::Load(std::span<const unsigned char> data, std::string *error) {
    Chunks.clear();
    Classes.clear();
    Objects.clear();
    SharedStrings = {};
    Metadata = {};
    Signatures = {};
    HasSharedStrings = HasMetadata = HasSignatures = false;

    if (data.size() < 32 || !std::equal(kMagic.begin(), kMagic.end(), data.begin())) {
        SetError(error, "file is not a binary RBXL/RBXM file");
        return false;
    }

    BinaryRobloxFileReader reader(data);
    reader.Seek(kMagic.size());
    reader.ReadBytes(&Version, sizeof(Version));
    NumClasses = reader.ReadUInt32();
    NumObjects = reader.ReadUInt32();
    reader.ReadBytes(&Reserved, sizeof(Reserved));
    if (reader.Failed() || Version != 0) {
        SetError(error, "unsupported binary file header");
        return false;
    }
    if (NumClasses > kMaximumObjectCount || NumObjects > kMaximumObjectCount) {
        SetError(error, "binary file header contains unreasonable object counts");
        return false;
    }

    Objects.resize(NumObjects);
    std::vector<int32_t> parentReferents(NumObjects, -1);

    // Property chunks are deferred: a PROP chunk can precede nothing, but decoding it needs the
    // class it belongs to, and INST chunks always come first in practice. Deferring keeps that
    // from being an ordering assumption.
    struct DeferredProperty {
        uint32_t ClassIndex {};
        std::string Name;
        PropertyType Type {};
        std::vector<unsigned char> Body;
    };
    std::vector<DeferredProperty> deferred;
    std::map<uint32_t, size_t> classByIndex;

    while (reader.Remaining() > 0) {
        BinaryRobloxFileChunk chunk;
        if (!chunk.Load(reader, error))
            return false;

        if (chunk.Is("END")) {
            Chunks.push_back(std::move(chunk));
            break;
        }

        BinaryRobloxFileReader body(chunk.Data);
        if (chunk.Is("INST")) {
            INST descriptor;
            if (!descriptor.Load(body)) {
                SetError(error, "invalid INST chunk");
                return false;
            }
            if (classByIndex.contains(static_cast<uint32_t>(descriptor.ClassIndex))) {
                SetError(error, "duplicate INST class index");
                return false;
            }
            classByIndex.emplace(static_cast<uint32_t>(descriptor.ClassIndex), Classes.size());

            for (size_t position = 0; position < descriptor.ObjectIds.size(); ++position) {
                const int32_t referent = descriptor.ObjectIds[position];
                if (referent < 0 || static_cast<size_t>(referent) >= Objects.size()) {
                    SetError(error, "INST referent is out of range");
                    return false;
                }
                if (Objects[static_cast<size_t>(referent)] != nullptr) {
                    SetError(error, "duplicate instance referent");
                    return false;
                }
                auto object = std::make_unique<Instance>();
                object->ClassName = descriptor.ClassName;
                object->Name = descriptor.ClassName;
                object->Referent = std::to_string(referent);
                object->IsService = descriptor.IsService;
                Objects[static_cast<size_t>(referent)] = std::move(object);
            }
            Classes.push_back(std::move(descriptor));
        } else if (chunk.Is("PROP")) {
            BinaryRobloxFileReader header(chunk.Data);
            DeferredProperty entry;
            entry.ClassIndex = header.ReadUInt32();
            entry.Name = header.ReadString();
            entry.Type = static_cast<PropertyType>(header.ReadByte());
            if (header.Failed()) {
                SetError(error, "invalid PROP chunk header");
                return false;
            }
            entry.Body.assign(chunk.Data.begin() + static_cast<std::ptrdiff_t>(header.Position()),
                              chunk.Data.end());
            deferred.push_back(std::move(entry));
        } else if (chunk.Is("PRNT")) {
            PRNT parents;
            if (!parents.Load(body)) {
                SetError(error, "invalid PRNT chunk");
                return false;
            }
            for (size_t index = 0; index < parents.ChildIds.size(); ++index) {
                const int32_t child = parents.ChildIds[index];
                const int32_t parent = parents.ParentIds[index];
                if (child < 0 || static_cast<size_t>(child) >= Objects.size() ||
                    Objects[static_cast<size_t>(child)] == nullptr) {
                    SetError(error, "PRNT references a missing child");
                    return false;
                }
                if (parent >= 0 && (static_cast<size_t>(parent) >= Objects.size() ||
                                    Objects[static_cast<size_t>(parent)] == nullptr)) {
                    SetError(error, "PRNT references a missing parent");
                    return false;
                }
                parentReferents[static_cast<size_t>(child)] = parent;
            }
        } else if (chunk.Is("SSTR")) {
            if (!SharedStrings.Load(body)) {
                SetError(error, "invalid SSTR chunk");
                return false;
            }
            HasSharedStrings = true;
        } else if (chunk.Is("META")) {
            if (!Metadata.Load(body)) {
                SetError(error, "invalid META chunk");
                return false;
            }
            HasMetadata = true;
        } else if (chunk.Is("SIGN")) {
            if (!Signatures.Load(body)) {
                SetError(error, "invalid SIGN chunk");
                return false;
            }
            HasSignatures = true;
        }
        Chunks.push_back(std::move(chunk));
    }

    for (const DeferredProperty &entry : deferred) {
        const auto found = classByIndex.find(entry.ClassIndex);
        if (found == classByIndex.end()) {
            SetError(error, "PROP chunk refers to a missing class");
            return false;
        }
        const INST &descriptor = Classes[found->second];

        BinaryRobloxFileReader values(entry.Body);
        PROP::ValueColumn column;
        if (!PROP::Read(values, entry.Type, descriptor.ObjectIds.size(), column)) {
            SetError(error, "could not decode property " + descriptor.ClassName + "." + entry.Name);
            return false;
        }

        for (size_t index = 0; index < descriptor.ObjectIds.size(); ++index) {
            Instance *object = GetObject(descriptor.ObjectIds[index]);
            if (object == nullptr)
                continue;
            Property property;
            property.Name = entry.Name;
            property.Type = entry.Type;
            property.Value = column.Values[index];
            // CFrame orientation ids and PhysicalProperties flag bytes are per column, not per
            // value, so stash them in RawBuffer -- the same escape hatch RobloxFiles uses to keep
            // a property's original byte form -- for Save to put back.
            if (index < column.OrientIds.size())
                property.RawBuffer = {column.OrientIds[index]};
            else if (index < column.RawFlags.size())
                property.RawBuffer = {column.RawFlags[index]};
            object->AddProperty(std::move(property));
            if (entry.Name == "Name") {
                if (const auto *text = std::any_cast<std::string>(&column.Values[index]))
                    object->Name = *text;
            }
        }
    }

    mLoadedObjectCount = Objects.size();

    // Parent links are applied last so every object exists first.
    for (size_t index = 0; index < Objects.size(); ++index) {
        if (Objects[index] == nullptr)
            continue;
        // Roots hang off the file itself, matching RobloxFile deriving from Instance.
        Instance *parent = parentReferents[index] < 0
            ? this : GetObject(parentReferents[index]);
        if (!Objects[index]->SetParent(parent)) {
            SetError(error, "PRNT associations do not form a valid tree");
            return false;
        }
    }
    return true;
}

std::vector<INST> BinaryRobloxFile::BuildTables() const {
    // Group by ClassName. RobloxFiles keys its class map the same way, which is why a class name
    // can never end up split across two INST chunks.
    std::map<std::string, std::vector<int32_t>> byClassName;
    for (size_t index = 0; index < Objects.size(); ++index) {
        if (Objects[index] == nullptr)
            continue;
        byClassName[Objects[index]->ClassName].push_back(static_cast<int32_t>(index));
    }

    std::vector<INST> table;
    table.reserve(byClassName.size());
    int32_t classIndex = 0;
    for (auto &[className, referents] : byClassName) {
        INST descriptor;
        descriptor.ClassIndex = classIndex++;
        descriptor.ClassName = className;
        descriptor.NumObjects = static_cast<int32_t>(referents.size());
        descriptor.ObjectIds = referents;
        descriptor.IsService = false;
        for (int32_t referent : referents) {
            if (Objects[static_cast<size_t>(referent)]->IsService) {
                descriptor.IsService = true;
                break;
            }
        }
        if (descriptor.IsService) {
            descriptor.RootedServices.reserve(referents.size());
            for (int32_t referent : referents) {
                const Instance *object = Objects[static_cast<size_t>(referent)].get();
                descriptor.RootedServices.push_back(
                    const_cast<Instance *>(object)->GetParent() == this);
            }
        }
        table.push_back(std::move(descriptor));
    }
    return table;
}

namespace {
// The last resort when no default is declared. UniqueId is the exception: duplicated all-zero ids
// are not valid, so it is derived from the referent to stay unique within the file.
std::any DefaultZeroFor(NoobWarrior::Roblox::PropertyType type, int32_t referent);

// A column carries one value per instance of its class, so an instance that never had the property
// still needs one written. The type's zero is the wrong answer often enough to break a place --
// CollisionGroup wants "Default", CanQuery true, and Material's lowest member is 256 -- so the
// engine's declared default wins. Utility/DefaultProperty.cs is the reference for this.
// Some properties are one value under two names and the format stores whichever the writing engine
// used, so a column for the spelling an instance lacks has to be derived from the one it has -- a
// default would contradict it. Font's zero is Legacy, which the engine applies over an explicit
// FontFace.
std::optional<std::any> DeriveFromLinkedProperty(const Instance &object,
                                                 std::string_view propertyName) {
    if (propertyName == "Font") {
        if (const Property *face = const_cast<Instance &>(object).GetProperty("FontFace")) {
            if (const auto *value = face->CastValue<DataTypes::FontFace>())
                return std::any(static_cast<uint32_t>(Utility::GetLegacyFont(*value)));
        }
        return std::nullopt;
    }
    if (propertyName == "FontFace") {
        if (const Property *font = const_cast<Instance &>(object).GetProperty("Font")) {
            if (const auto *value = font->CastValue<uint32_t>()) {
                DataTypes::FontFace face;
                if (Utility::TryGetFontFace(static_cast<int32_t>(*value), face))
                    return std::any(face);
            }
        }
        return std::nullopt;
    }
    // FontSize and TextSize are the same pair one layer down. Enum.FontSize.Size8 is 0, so a
    // zero-filled FontSize column renders every label from the file at 8 pixels.
    if (propertyName == "FontSize") {
        if (const Property *size = const_cast<Instance &>(object).GetProperty("TextSize")) {
            if (const auto *value = size->CastValue<float>())
                return std::any(static_cast<uint32_t>(Utility::GetFontSize(*value)));
        }
        return std::nullopt;
    }
    if (propertyName == "TextSize") {
        if (const Property *size = const_cast<Instance &>(object).GetProperty("FontSize")) {
            if (const auto *value = size->CastValue<uint32_t>()) {
                const int32_t pixels = Utility::GetFontSizePixels(static_cast<int32_t>(*value));
                if (pixels != 0)
                    return std::any(static_cast<float>(pixels));
            }
        }
        return std::nullopt;
    }
    return std::nullopt;
}

std::any DefaultValueFor(const Instance *object, std::string_view className,
                         std::string_view propertyName,
                         NoobWarrior::Roblox::PropertyType type, int32_t referent) {
    // A linked spelling the instance does carry is stronger evidence than any global default.
    if (object != nullptr) {
        if (std::optional<std::any> derived = DeriveFromLinkedProperty(*object, propertyName))
            return *derived;
    }
    // Utility::DefaultProperty is the port of DefaultProperty.cs and the one place that answers
    // this; duplicating the registry walk here would be a second copy to keep in step.
    if (std::optional<std::any> declared =
            Utility::DefaultProperty::Get(className, propertyName)) {
        return *declared;
    }
    // Nothing declared: fall back to the type's zero. PROP::Write coerces it to whatever the
    // column settled on.
    return DefaultZeroFor(type, referent);
}

std::any DefaultZeroFor(NoobWarrior::Roblox::PropertyType type, int32_t referent) {
    using namespace NoobWarrior::Roblox;
    using namespace NoobWarrior::Roblox::DataTypes;
    switch (type) {
    case PropertyType::String:          return std::string();
    case PropertyType::ProtectedString: return ProtectedString();
    case PropertyType::Bool:            return false;
    case PropertyType::Int:             return int32_t {0};
    case PropertyType::Int64:           return int64_t {0};
    case PropertyType::Float:           return 0.0f;
    case PropertyType::Double:          return 0.0;
    case PropertyType::Enum:            return uint32_t {0};
    case PropertyType::SharedString:    return uint32_t {0};
    case PropertyType::Ref:             return int32_t {-1};
    case PropertyType::BrickColor:      return int32_t {194};
    case PropertyType::UDim:            return UDim();
    case PropertyType::UDim2:           return UDim2();
    case PropertyType::Ray:             return Ray();
    case PropertyType::Faces:           return Faces();
    case PropertyType::Axes:            return Axes();
    case PropertyType::Color3:          return Color3();
    case PropertyType::Color3uint8:     return Color3uint8();
    case PropertyType::Vector2:         return Vector2();
    case PropertyType::Vector3:         return Vector3();
    case PropertyType::Vector3int16:    return Vector3int16();
    case PropertyType::Rect:            return Rect();
    case PropertyType::NumberRange:     return NumberRange();
    case PropertyType::NumberSequence:  return NumberSequence();
    case PropertyType::ColorSequence:   return ColorSequence();
    case PropertyType::CFrame:
    case PropertyType::Quaternion:      return CFrame();
    case PropertyType::OptionalCFrame:  return std::optional<CFrame>();
    case PropertyType::PhysicalProperties: return std::optional<PhysicalProperties>();
    case PropertyType::FontFace:        return FontFace();
    case PropertyType::SecurityCapabilities: return SecurityCapabilities();
    case PropertyType::Content:         return Content();
    case PropertyType::UniqueId:
        return UniqueId(static_cast<int64_t>(referent) + 1, 0,
                        static_cast<uint32_t>(referent) + 1);
    default:                            return {};
    }
}
} // namespace

void BinaryRobloxFile::CompactObjects() {
    std::vector<int32_t> remap(Objects.size(), -1);
    int32_t next = 0;
    size_t loaded = 0;
    for (size_t index = 0; index < Objects.size(); ++index) {
        if (Objects[index] == nullptr)
            continue;
        // Compaction preserves order, so everything that came out of the file stays in a prefix
        // and the loaded/appended split survives as a smaller count.
        if (index < mLoadedObjectCount)
            ++loaded;
        remap[index] = next++;
    }
    if (static_cast<size_t>(next) == Objects.size())
        return;

    const auto moved = [&remap](int32_t referent) {
        if (referent < 0 || static_cast<size_t>(referent) >= remap.size())
            return -1;
        return remap[static_cast<size_t>(referent)];
    };

    for (auto &object : Objects) {
        if (object == nullptr)
            continue;
        for (auto &[name, property] : object->GetProperties()) {
            if (property.Type == PropertyType::Ref) {
                if (const auto *referent = property.CastValue<int32_t>())
                    property.Value = moved(*referent);
            } else if (property.Type == PropertyType::Content) {
                const auto *content = property.CastValue<DataTypes::Content>();
                if (content == nullptr ||
                    content->SourceType != DataTypes::ContentSourceType::Object) {
                    continue;
                }
                DataTypes::Content updated = *content;
                updated.ObjectReferent = moved(updated.ObjectReferent);
                property.Value = updated;
            }
        }
    }

    std::vector<std::unique_ptr<Instance>> compacted;
    compacted.reserve(static_cast<size_t>(next));
    for (auto &object : Objects) {
        if (object == nullptr)
            continue;
        object->Referent = std::to_string(compacted.size());
        compacted.push_back(std::move(object));
    }
    Objects = std::move(compacted);
    mLoadedObjectCount = loaded;
}

bool BinaryRobloxFile::Save(std::vector<unsigned char> &output, std::string *error) const {
    // Referents are positions in Objects, so anything destroyed since the last save has to be
    // squeezed out before the table is built. Const because callers serialize const files; the
    // change is to the representation, not to what the tree means.
    const_cast<BinaryRobloxFile *>(this)->CompactObjects();

    const std::vector<INST> table = BuildTables();
    uint32_t objectCount = 0;
    for (const auto &object : Objects) {
        if (object != nullptr)
            ++objectCount;
    }

    BinaryRobloxFileWriter file;
    file.WriteBytes(kMagic.data(), kMagic.size());
    file.WriteBytes(&Version, sizeof(Version));
    file.WriteUInt32(static_cast<uint32_t>(table.size()));
    file.WriteUInt32(objectCount);
    file.WriteBytes(&Reserved, sizeof(Reserved));

    const auto writeChunk = [&](const char *type, const std::vector<unsigned char> &body) {
        BinaryRobloxFileChunk chunk;
        for (size_t index = 0; index < chunk.ChunkType.size(); ++index)
            chunk.ChunkType[index] = type[index] == 0 ? 0 : static_cast<unsigned char>(type[index]);
        chunk.Data = body;
        return chunk.Save(file, true, error);
    };

    if (HasMetadata) {
        BinaryRobloxFileWriter body;
        Metadata.Save(body);
        if (!writeChunk("META", body.Data()))
            return false;
    }
    if (HasSharedStrings) {
        BinaryRobloxFileWriter body;
        SharedStrings.Save(body);
        if (!writeChunk("SSTR", body.Data()))
            return false;
    }

    for (const INST &descriptor : table) {
        BinaryRobloxFileWriter body;
        descriptor.Save(body);
        if (!writeChunk("INST", body.Data()))
            return false;
    }

    for (const INST &descriptor : table) {
        // Property order is the union across the class's objects, sorted so output is stable. A
        // column carries one type, and mounted instances can disagree with the file, so the type
        // the file itself wrote wins; otherwise whatever was seen first. The API dump is not
        // consulted -- see the note below.
        std::map<std::string, PropertyType> columns;
        std::map<std::string, PropertyType> fromFile;
        bool anyLoaded = false;
        for (int32_t referent : descriptor.ObjectIds) {
            const Instance *object = GetObject(referent);
            if (object == nullptr)
                continue;
            const bool loaded = static_cast<size_t>(referent) < mLoadedObjectCount;
            anyLoaded = anyLoaded || loaded;
            for (const auto &[name, property] : object->GetProperties()) {
                columns.emplace(name, property.Type);
                if (loaded)
                    fromFile.emplace(name, property.Type);
            }
        }

        // Roblox writes a consistent property set per class, so a name the file's own instances
        // never carried is one that engine does not serialize -- and a column cannot say "absent",
        // it would hand every loaded instance a zero. Drop it instead. A class with no loaded
        // instances is entirely new, so everything it brought is kept.
        if (anyLoaded) {
            std::erase_if(columns, [&fromFile](const auto &column) {
                return !fromFile.contains(column.first);
            });
        }
        for (auto &[name, type] : columns) {
            const auto original = fromFile.find(name);
            if (original != fromFile.end()) {
                type = original->second;
            }
            // Otherwise the type stays as first seen, which is the type the mounting plugin
            // declared. The API dump deliberately does not get a say here: it names a property's
            // *declared* type, not the type the format stores it as, and the two disagree often
            // enough to be dangerous -- BasePart.Color is DataType/Color3 in the dump and
            // Color3uint8 on the wire. PROP::Write already coerces between the pairs that differ,
            // so nothing is gained by overriding an explicit choice with a guess.
        }

        for (const auto &[name, type] : columns) {
            PROP::ValueColumn column;
            column.Type = type;
            column.Values.reserve(descriptor.ObjectIds.size());
            for (int32_t referent : descriptor.ObjectIds) {
                const Instance *object = GetObject(referent);
                const auto &properties = object->GetProperties();
                const auto found = properties.find(name);
                if (found == properties.end()) {
                    column.Values.push_back(
                        DefaultValueFor(object, descriptor.ClassName, name, type, referent));
                    continue;
                }
                column.Values.push_back(found->second.Value);
                if (!found->second.RawBuffer.empty()) {
                    if (type == PropertyType::CFrame || type == PropertyType::Quaternion ||
                        type == PropertyType::OptionalCFrame) {
                        column.OrientIds.push_back(found->second.RawBuffer[0]);
                    } else if (type == PropertyType::PhysicalProperties) {
                        column.RawFlags.push_back(found->second.RawBuffer[0]);
                    }
                }
            }

            const bool isCFrame = type == PropertyType::CFrame ||
                type == PropertyType::Quaternion || type == PropertyType::OptionalCFrame;
            if (isCFrame && column.OrientIds.size() != column.Values.size()) {
                // A graph assembled in memory carries no stored orientation ids, so derive them.
                column.OrientIds.clear();
                column.OrientIds.reserve(column.Values.size());
                for (const std::any &value : column.Values) {
                    const DataTypes::CFrame *frame = std::any_cast<DataTypes::CFrame>(&value);
                    if (frame == nullptr) {
                        const auto *optional =
                            std::any_cast<std::optional<DataTypes::CFrame>>(&value);
                        if (optional != nullptr && optional->has_value())
                            frame = &optional->value();
                    }
                    const int orientId = frame != nullptr ? frame->GetOrientId() : -1;
                    column.OrientIds.push_back(static_cast<uint8_t>(orientId + 1));
                }
            }

            BinaryRobloxFileWriter body;
            body.WriteUInt32(static_cast<uint32_t>(descriptor.ClassIndex));
            body.WriteString(name);
            body.WriteByte(static_cast<uint8_t>(type));
            if (!PROP::Write(body, column)) {
                SetError(error, "could not encode property " + descriptor.ClassName + "." + name);
                return false;
            }
            if (!writeChunk("PROP", body.Data()))
                return false;
        }
    }

    {
        PRNT parents;
        for (size_t index = 0; index < Objects.size(); ++index) {
            if (Objects[index] == nullptr)
                continue;
            Instance *parent = Objects[index]->GetParent();
            int32_t parentReferent = -1;
            if (parent != nullptr && parent != this)
                ParseInteger(parent->Referent, parentReferent);
            parents.ChildIds.push_back(static_cast<int32_t>(index));
            parents.ParentIds.push_back(parentReferent);
        }
        BinaryRobloxFileWriter body;
        parents.Save(body);
        if (!writeChunk("PRNT", body.Data()))
            return false;
    }

    // Signatures cover the bytes we just rewrote, so they are deliberately not re-emitted.
    const std::string ending = "</roblox>";
    if (!writeChunk("END", std::vector<unsigned char>(ending.begin(), ending.end())))
        return false;

    output = file.Release();
    return true;
}

FileResponse BinaryRobloxFile::ReadFile(const std::vector<unsigned char> &buffer) {
    std::string error;
    if (!Load(buffer, &error)) {
        SetLastError(std::move(error));
        return FileResponse::CouldNotParse;
    }
    return FileResponse::Success;
}

FileResponse BinaryRobloxFile::Save(std::vector<unsigned char> &buffer) const {
    std::string error;
    if (Save(buffer, &error))
        return FileResponse::Success;
    // Surface why: the caller only sees a FileResponse, so a swallowed message here
    // turns any serialization fault into an unactionable "could not serialize".
    const_cast<BinaryRobloxFile *>(this)->SetLastError(std::move(error));
    return FileResponse::Failed;
}

bool BinaryRobloxFile::AppendLuaSourceContainers(
    std::span<const LuaSourceContainerSpec> containers, std::string *error) {
    // Validate the whole batch first so a malformed entry cannot leave earlier ones behind.
    for (const LuaSourceContainerSpec &container : containers) {
        if (container.ClassName != "Script" && container.ClassName != "LocalScript" &&
            container.ClassName != "ModuleScript") {
            SetError(error, "unsupported LuaSourceContainer class " +
                            std::string(container.ClassName));
            return false;
        }
        const std::string_view parentClassName = container.ParentClassName.empty()
            ? std::string_view("ServerScriptService") : container.ParentClassName;
        if (parentClassName != "ServerScriptService" &&
            parentClassName != "StarterPlayerScripts") {
            SetError(error, "unsupported LuaSourceContainer parent " +
                            std::string(parentClassName));
            return false;
        }
    }

    for (const LuaSourceContainerSpec &container : containers) {
        const std::string parentClassName(container.ParentClassName.empty()
            ? std::string_view("ServerScriptService") : container.ParentClassName);

        Instance *parent = FindFirstOfClass(parentClassName);
        if (parent == nullptr) {
            parent = CreateObject(parentClassName);
            if (parentClassName == "StarterPlayerScripts") {
                Instance *starterPlayer = FindFirstOfClass("StarterPlayer");
                if (starterPlayer == nullptr) {
                    starterPlayer = CreateObject("StarterPlayer");
                    starterPlayer->IsService = true;
                    starterPlayer->SetParent(this);
                }
                parent->SetParent(starterPlayer);
            } else {
                parent->IsService = true;
                parent->SetParent(this);
            }
        }

        Instance *script = CreateObject(std::string(container.ClassName));
        script->Name = std::string(container.Name);

        Property name;
        name.Name = "Name";
        name.Type = PropertyType::String;
        name.Value = std::string(container.Name);
        script->AddProperty(std::move(name));

        Property source;
        source.Name = "Source";
        source.Type = PropertyType::String;
        source.XmlToken = "ProtectedString";
        source.Value = DataTypes::ProtectedString(std::string(container.Source));
        script->AddProperty(std::move(source));

        if (container.ClassName != "ModuleScript") {
            Property disabled;
            disabled.Name = "Disabled";
            disabled.Type = PropertyType::Bool;
            disabled.Value = container.Disabled;
            script->AddProperty(std::move(disabled));
        }

        script->SetParent(parent);
    }
    return true;
}

namespace {
// Services are identified by name at the top of the DataModel, matching how Roblox stores them.
// The API dump tags every service class, so the generated registry answers this for all of them
// rather than the handful a hand-kept list can hold. The list below is what a build without a
// Generated/ directory falls back to; anything outside it lands in the tree as a plain instance,
// which is what happened for every service before the registry existed.
bool IsDataModelService(std::string_view name) {
#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
    if (const ClassDescriptor *descriptor = FindClass(name))
        return descriptor->IsService;
#endif
    static constexpr std::string_view kServices[] = {
        "Chat", "Lighting", "Players", "ReplicatedFirst", "ReplicatedStorage",
        "ServerScriptService", "ServerStorage", "SoundService", "StarterGui",
        "StarterPack", "StarterPlayer", "Teams", "TextChatService", "Workspace",
        "HttpService", "CollectionService", "RunService", "InsertService",
    };
    return std::find(std::begin(kServices), std::end(kServices), name) != std::end(kServices);
}
} // namespace



Instance *BinaryRobloxFile::ResolvePath(std::span<const BinaryModelPathElement> parentPath) {
    Instance *current = this;
    for (const BinaryModelPathElement &element : parentPath) {
        if (element.Name.empty())
            return nullptr;

        Instance *found = nullptr;
        for (Instance *child : current->GetChildren()) {
            if (child->Name == element.Name) {
                found = child;
                break;
            }
        }
        if (found == nullptr) {
            const bool service = current == this && IsDataModelService(element.Name);
            std::string className = service ? element.Name : element.ClassName;
            if (className.empty())
                className = "Folder";
            found = CreateObject(className);
            found->Name = element.Name;
            found->IsService = service;

            Property name;
            name.Name = "Name";
            name.Type = PropertyType::String;
            name.Value = element.Name;
            found->AddProperty(std::move(name));
            found->SetParent(current);
        }
        current = found;
    }
    return current;
}

bool BinaryRobloxFile::AppendBinaryModel(std::span<const unsigned char> model,
                                         std::span<const BinaryModelPathElement> parentPath,
                                         std::string_view singleRootName,
                                         std::string *error, size_t *replacedRoots) {
    BinaryRobloxFile source;
    if (!source.Load(model, error))
        return false;

    const std::vector<Instance *> sourceRoots = source.Roots();
    if (sourceRoots.empty()) {
        SetError(error, "binary model does not contain any root instances");
        return false;
    }
    if (!singleRootName.empty() && sourceRoots.size() != 1) {
        SetError(error, "cannot rename a binary model with multiple roots");
        return false;
    }

    Instance *destination = ResolvePath(parentPath);
    if (destination == nullptr) {
        SetError(error, "binary model destination path contains an empty name");
        return false;
    }

    // A model file is one replaceable unit: two plugins shipping the same .rbxm at the same path
    // means the later one wins outright, rather than both landing as same-named siblings. The
    // unit's identity at the destination is the set of names its roots arrive under.
    std::vector<std::string> incomingRoots;
    if (!singleRootName.empty()) {
        incomingRoots.emplace_back(singleRootName);
    } else {
        incomingRoots.reserve(sourceRoots.size());
        for (const Instance *root : sourceRoots)
            incomingRoots.push_back(root->Name);
    }
    for (Instance *child : destination->GetChildren()) {
        if (std::find(incomingRoots.begin(), incomingRoots.end(), child->Name) ==
            incomingRoots.end()) {
            continue;
        }
        DestroySubtree(child);
        if (replacedRoots != nullptr)
            ++*replacedRoots;
    }

    // Shared strings are addressed by index, so the source table has to be merged first and the
    // resulting index map applied to every SharedString value that comes across.
    std::vector<uint32_t> sharedStringMap;
    sharedStringMap.reserve(source.SharedStrings.Strings.size());
    for (const SSTR::Entry &entry : source.SharedStrings.Strings)
        sharedStringMap.push_back(SharedStrings.Add(entry));
    if (!source.SharedStrings.Strings.empty())
        HasSharedStrings = true;

    // Copy every instance first so referents exist before anything is remapped.
    std::map<int32_t, int32_t> referentMap;
    std::vector<std::pair<const Instance *, Instance *>> copied;
    for (const auto &object : source.Objects) {
        if (object == nullptr)
            continue;
        int32_t sourceReferent = -1;
        if (!ParseInteger(object->Referent, sourceReferent))
            continue;

        Instance *clone = CreateObject(object->ClassName);
        clone->Name = object->Name;
        clone->IsService = false;
        int32_t cloneReferent = -1;
        ParseInteger(clone->Referent, cloneReferent);
        referentMap.emplace(sourceReferent, cloneReferent);
        copied.emplace_back(object.get(), clone);
    }

    const auto remapReferent = [&](int32_t value) {
        if (value < 0)
            return -1;
        const auto found = referentMap.find(value);
        return found == referentMap.end() ? -1 : found->second;
    };

    for (const auto &[original, clone] : copied) {
        for (const auto &[name, property] : original->GetProperties()) {
            Property copy = property;
            if (property.Type == PropertyType::Ref) {
                if (const auto *referent = property.CastValue<int32_t>())
                    copy.Value = remapReferent(*referent);
            } else if (property.Type == PropertyType::SharedString) {
                if (const auto *index = property.CastValue<uint32_t>()) {
                    if (*index >= sharedStringMap.size()) {
                        SetError(error, "binary model SharedString index is out of range");
                        return false;
                    }
                    copy.Value = sharedStringMap[*index];
                }
            } else if (property.Type == PropertyType::Content) {
                if (const auto *content = property.CastValue<DataTypes::Content>()) {
                    DataTypes::Content updated = *content;
                    if (updated.SourceType == DataTypes::ContentSourceType::Object)
                        updated.ObjectReferent = remapReferent(updated.ObjectReferent);
                    copy.Value = updated;
                }
            }
            clone->AddProperty(std::move(copy));
        }
    }

    // Reparent inside the copied tree, then hang the roots off the destination.
    for (const auto &[original, clone] : copied) {
        Instance *originalParent = const_cast<Instance *>(original)->GetParent();
        Instance *parent = destination;
        if (originalParent != nullptr && originalParent != &source) {
            int32_t parentReferent = -1;
            ParseInteger(originalParent->Referent, parentReferent);
            const int32_t mapped = remapReferent(parentReferent);
            parent = mapped < 0 ? destination : GetObject(mapped);
        }
        if (!clone->SetParent(parent)) {
            SetError(error, "binary model does not form a valid tree");
            return false;
        }
    }

    if (!singleRootName.empty()) {
        int32_t rootReferent = -1;
        ParseInteger(sourceRoots.front()->Referent, rootReferent);
        Instance *root = GetObject(remapReferent(rootReferent));
        if (root != nullptr) {
            root->Name = std::string(singleRootName);
            Property name;
            name.Name = "Name";
            name.Type = PropertyType::String;
            name.Value = std::string(singleRootName);
            root->AddProperty(std::move(name));
        }
    }
    return true;
}
