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
// File: Instance.h
// Started by: Hattozo
// Started on: 3/9/2025
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Tree/Instance.cs)
#pragma once
#include <NoobWarrior/Roblox/FileFormat/Tree/Attributes.h>
#include <NoobWarrior/Roblox/FileFormat/Tree/RbxObject.h>

#include <string>
#include <vector>

namespace NoobWarrior::Roblox {
class Instance : public RbxObject {
public:
    Instance();

    /**
     * @brief The Name of this Instance.
     *
     * Read it freely, but write it through SetName(). The serialized "Name" property is a
     * separate store that is only synced into this field on load, so assigning here alone
     * writes the stale name back out on save.
     */
    std::string Name;

    /**
     * @brief Indicates whether the parent of this object is locked.
     */
    bool ParentLocked;

    /**
        * @brief Indicates whether this Instance is a Service.
        */
    bool IsService;

    /**
     * @brief The property the attribute blob lives in.
     *
     * Roblox types it as a BinaryString, which this port carries as a PropertyType::String whose
     * value is the raw bytes -- see Tokens/BinaryString.h.
     */
    static constexpr std::string_view kAttributesProperty = "AttributesSerialize";

    /**
     * @brief Reads an attribute of type T.
     *
     * Reference Tree/Instance.cs:128-141. Returns false when there is no such attribute, when it
     * holds another type, or when the blob could not be decoded.
     *
     * The reference keeps a live RbxAttributes alongside the Instance and syncs it through the
     * AttributesSerialize property getter/setter. There is no property-backed field mechanism
     * here, so the blob is decoded per call instead. Decode once through LoadAttributes when
     * reading more than one attribute off the same Instance.
     */
    template <typename T>
    bool GetAttribute(std::string key, T* value);

    /**
     * @brief Writes an attribute, replacing any attribute already under that key.
     *
     * Reference Tree/Instance.cs:152-172. Returns false when the key is longer than
     * RbxAttributes::kMaxKeyLength, when T is a type attributes cannot carry, or when the
     * existing blob is malformed -- in that last case the blob is left exactly as it was rather
     * than rewritten from a partial decode.
     *
     * Attributes of a type this port cannot decode survive untouched.
     */
    template <typename T>
    bool SetAttribute(std::string key, T value);

    /**
     * @brief Decodes the AttributesSerialize blob into @p attributes.
     *
     * Response::Empty when this Instance carries no blob at all, so a caller that only wants to
     * know whether decoding succeeded can test for Malformed.
     */
    RbxAttributes::Response LoadAttributes(RbxAttributes &attributes) const;

    /**
     * @brief Re-encodes @p attributes back into AttributesSerialize.
     *
     * An empty set drops the property instead of storing a zero-length blob: a binary PROP column
     * carries one value per instance of the class, so a property kept here costs every sibling of
     * this Instance's class a slot in that column.
     */
    void StoreAttributes(const RbxAttributes &attributes);

    /**
     * @brief Renames this Instance, writing both the Name field and the serialized "Name"
     * property so the new name survives a save.
     */
    void SetName(const std::string &name);

    /**
     * @brief Detaches this Instance and its descendants from the tree, locks its parent and
     * clears its properties -- reference Tree/Instance.cs:447-462.
     *
     * This frees nothing: a RobloxFile owns its Instances, so dropping one from a file still
     * goes through RobloxFile::DestroySubtree.
     */
    void Destroy() override;

    bool IsAncestorOf(Instance* ancestor);
    bool IsDescendantOf(Instance* descendant);

    Instance* GetParent();
    bool SetParent(Instance* inst);
    std::vector<Instance*> GetChildren() const;
    std::vector<Instance*> GetDescendants() const;

    std::string GetFullName(const std::string &separator = "\\");
private:
    Instance* ParentUnsafe;
    // Reference Tree/Instance.cs:24-27 keeps children in an insertion-ordered List, and that
    // order is load-bearing: BinaryRobloxFile::ResolvePath and PluginTreeMaterializer both take
    // the *first* child matching a name, so a hashed container made that choice vary between
    // runs whenever two siblings shared a name.
    std::vector<Instance*> Children;
};

template <typename T>
bool Instance::GetAttribute(std::string key, T* value) {
    if (value == nullptr)
        return false;

    RbxAttributes attributes;
    if (LoadAttributes(attributes) == RbxAttributes::Response::Malformed)
        return false;

    const RbxAttribute *attribute = attributes.Find(key);
    if (attribute == nullptr)
        return false;

    const AttributeValueType<T> *stored = attribute->Get<T>();
    if (stored == nullptr)
        return false;

    *value = *stored;
    return true;
}

template <typename T>
bool Instance::SetAttribute(std::string key, T value) {
    if constexpr (!SupportedAttributeType<T>) {
        // Instance.cs:165-166 reports an unsupported type rather than refusing to build, and a
        // caller working over a generic T has no other way to ask.
        return false;
    } else {
        RbxAttributes attributes;
        if (LoadAttributes(attributes) == RbxAttributes::Response::Malformed)
            return false;

        if (!attributes.Set(std::move(key), std::move(value)))
            return false;

        StoreAttributes(attributes);
        return true;
    }
}
}
