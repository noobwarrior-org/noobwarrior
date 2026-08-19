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
// File: XmlRobloxFile.cpp
// Description: Ported RobloxFiles.XmlFormat.XmlRobloxFile: an XML place/model as an object graph.
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlRobloxFile.h>

#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlFileReader.h>
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlFileWriter.h>

#include <NoobWarrior/Log.h>

#include <cstring>
#include <sstream>
#include <string_view>

using namespace NoobWarrior::Roblox;
using namespace NoobWarrior::Roblox::XmlFormat;

std::vector<Instance *> XmlRobloxFile::Roots() const {
    return GetChildren();
}

// XmlRobloxFile.cs:85-107. RefToken keeps the referent string as the property value rather than
// an Instance pointer, so resolving here means confirming the target exists: a Ref naming
// nothing would otherwise be written straight back out and mislead whatever reads the file next.
void XmlRobloxFile::ResolveReferences(const ReferentIndex &instances) {
    for (const auto &object : Objects) {
        if (object == nullptr)
            continue;
        for (auto &[name, property] : object->GetProperties()) {
            // A Content can name an instance too, and it carries the referent under its own
            // member rather than as the property value.
            if (property.Type == PropertyType::Content) {
                const auto *content = property.CastValue<DataTypes::Content>();
                if (content == nullptr || !content->HasRefId())
                    continue;
                if (instances.contains(content->RefId))
                    continue;
                NoobWarrior::Out("XmlRobloxFile", "Could not resolve reference for {}.{}",
                                 object->ClassName, name);
                DataTypes::Content cleared = *content;
                cleared.ClearReferent();
                property.Value = cleared;
                continue;
            }
            if (property.Type != PropertyType::Ref)
                continue;
            const std::string *target = property.CastValue<std::string>();
            const std::string referent = target != nullptr ? *target : std::string();
            if (instances.contains(referent))
                continue;
            // "null" and "Ref" are the two spellings of "points at nothing" and are not worth
            // reporting (XmlRobloxFile.cs:102).
            if (!referent.empty() && referent != "null" && referent != "Ref") {
                NoobWarrior::Out("XmlRobloxFile", "Could not resolve reference for {}.{}",
                                 object->ClassName, name);
            }
            // XmlRobloxFile.cs:105 nulls the property; RefToken writes "null" for an empty one.
            property.Value = std::string();
        }
    }
}

bool XmlRobloxFile::Load(std::span<const unsigned char> data, std::string *error) {
    Objects.clear();
    Metadata.clear();
    SharedStrings.clear();
    mNextReferent = 0;

    pugi::xml_document document;
    const pugi::xml_parse_result parsed = document.load_buffer(data.data(), data.size());
    if (!parsed) {
        SetError(error, std::string("could not parse XML: ") + parsed.description());
        return false;
    }

    const pugi::xml_node roblox = document.child("roblox");
    if (!roblox) {
        SetError(error, "XML document has no roblox root element");
        return false;
    }
    // A missing version is a refusal, not a default (XmlRobloxFile.cs:57-58): assuming 4 would
    // read a document written against some other schema as though it were a v4 one.
    const pugi::xml_attribute version = roblox.attribute("version");
    int schemaVersion = 0;
    if (!version || !Tokens::ParseInteger(std::string_view(version.as_string()), schemaVersion)) {
        SetError(error, "XML document has no version number");
        return false;
    }
    if (schemaVersion < 4) {
        SetError(error, "XML document version must be at least 4");
        return false;
    }
    Version = schemaVersion;

    ReferentIndex instances;

    for (pugi::xml_node child : roblox.children()) {
        if (std::strcmp(child.name(), "Meta") == 0) {
            if (!XmlRobloxFileReader::ReadMetadata(child, *this, error))
                return false;
        } else if (std::strcmp(child.name(), "SharedStrings") == 0) {
            if (!XmlRobloxFileReader::ReadSharedStrings(child, *this, error))
                return false;
        } else if (std::strcmp(child.name(), "Item") == 0) {
            // Roots hang off the file itself, matching RobloxFile deriving from Instance.
            if (XmlRobloxFileReader::ReadInstance(child, *this, this, instances, error) == nullptr)
                return false;
        }
    }

    ResolveReferences(instances);
    return true;
}

bool XmlRobloxFile::Save(std::vector<unsigned char> &output, std::string *error) const {
    pugi::xml_document document;

    // No <?xml?> declaration: XmlFileWriter.cs:22 omits it, and the engines noobWarrior targets
    // read the documents RobloxFiles writes.
    pugi::xml_node roblox = document.append_child("roblox");
    // Always 4, never the version that was read (XmlRobloxFile.cs:137). This writer only knows
    // how to emit the v4 schema, so echoing a higher number back would mislabel the document.
    roblox.append_attribute("version").set_value("4");

    for (const auto &[name, value] : Metadata) {
        pugi::xml_node meta = roblox.append_child("Meta");
        meta.append_attribute("name").set_value(name.c_str());
        meta.text().set(value.c_str());
    }

    // Rebuilt from the properties actually written (XmlRobloxFile.cs:140-142) rather than
    // replayed from load, so a shared string whose last user was removed does not linger. It is
    // a local because saving does not otherwise mutate the file.
    SharedStringTable sharedStrings;
    for (const Instance *root : GetChildren())
        XmlRobloxFileWriter::WriteInstance(*root, roblox, *this, sharedStrings);

    if (!sharedStrings.empty())
        XmlRobloxFileWriter::WriteSharedStrings(roblox, sharedStrings);

    std::ostringstream stream;
    document.save(stream, XmlRobloxFileWriter::kIndentChars,
                  pugi::format_default | pugi::format_no_declaration,
                  pugi::encoding_utf8);
    const std::string serialized = stream.str();
    output.assign(serialized.begin(), serialized.end());
    if (output.empty()) {
        SetError(error, "could not serialize the XML document");
        return false;
    }
    return true;
}

FileResponse XmlRobloxFile::ReadFile(const std::vector<unsigned char> &buffer) {
    std::string error;
    if (!Load(buffer, &error)) {
        SetLastError(std::move(error));
        return FileResponse::CouldNotParse;
    }
    return FileResponse::Success;
}

FileResponse XmlRobloxFile::Save(std::vector<unsigned char> &buffer) const {
    std::string error;
    if (Save(buffer, &error))
        return FileResponse::Success;
    // Surface why: the caller only sees a FileResponse, so a swallowed message here
    // turns any serialization fault into an unactionable "could not serialize".
    const_cast<XmlRobloxFile *>(this)->SetLastError(std::move(error));
    return FileResponse::Failed;
}

XmlRobloxFile::XmlRobloxFile() {
    Name = "Xml:";
    Referent = "null";
    ParentLocked = true;
}

Instance *XmlRobloxFile::CreateObject(const std::string &className) {
    auto object = std::make_unique<Instance>();
    object->ClassName = className;
    object->Name = className;
    object->Referent = "RBX_NOOBWARRIOR_" + std::to_string(++mNextReferent);
    Instance *raw = object.get();
    Objects.push_back(std::move(object));
    return raw;
}

Instance *XmlRobloxFile::FindFirstOfClass(std::string_view className) const {
    for (const auto &object : Objects) {
        if (object != nullptr && object->ClassName == className)
            return object.get();
    }
    return nullptr;
}

bool XmlRobloxFile::AppendLuaSourceContainers(
    std::span<const LuaSourceContainerSpec> containers, std::string *error) {
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
                    starterPlayer->SetParent(this);
                }
                parent->SetParent(starterPlayer);
            } else {
                parent->SetParent(this);
            }
        }

        Instance *script = CreateObject(std::string(container.ClassName));
        script->Name = std::string(container.Name);

        Property name;
        name.Name = "Name";
        name.Type = PropertyType::String;
        name.XmlToken = "string";
        name.Value = std::string(container.Name);
        script->AddProperty(std::move(name));

        Property source;
        source.Name = "Source";
        source.Type = PropertyType::ProtectedString;
        source.XmlToken = "ProtectedString";
        source.Value = DataTypes::ProtectedString(std::string(container.Source));
        script->AddProperty(std::move(source));

        if (container.ClassName != "ModuleScript") {
            Property disabled;
            disabled.Name = "Disabled";
            disabled.Type = PropertyType::Bool;
            disabled.XmlToken = "bool";
            disabled.Value = container.Disabled;
            script->AddProperty(std::move(disabled));
        }

        script->SetParent(parent);
    }
    return true;
}

Instance *XmlRobloxFile::CreateInstance(const std::string &className,
                                       const std::string &name, Instance *parent) {
    Instance *instance = CreateObject(className);
    instance->Name = name;
    Property property;
    property.Name = "Name";
    property.Type = PropertyType::String;
    property.XmlToken = "string";
    property.Value = name;
    instance->AddProperty(std::move(property));
    if (!instance->SetParent(parent == nullptr ? this : parent))
        return nullptr;
    return instance;
}

void XmlRobloxFile::DestroySubtree(Instance *instance) {
    if (instance == nullptr)
        return;
    for (Instance *child : instance->GetChildren())
        DestroySubtree(child);
    instance->SetParent(nullptr);
    for (auto &object : Objects) {
        if (object.get() == instance)
            object.reset();
    }
}
