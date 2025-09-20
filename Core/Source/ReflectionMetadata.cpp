// === noobWarrior ===
// File: ReflectionMetadata.cpp
// Started by: Hattozo
// Started on: 9/19/2025
// Description: Anything that requires runtime to dynamically understand the structure of data
// without having to manually specify it everywhere should go here
//
// ID types for example; you don't want to add checks in 30 different code paths because you added a new ID type.
// Just declare it here and those code paths will automatically discover it.
#include <NoobWarrior/ReflectionMetadata.h>

#include <NoobWarrior/Database/Record/IdType/Asset.h>
#include <NoobWarrior/Database/Record/IdType/Badge.h>
#include <NoobWarrior/Database/Record/IdType/Bundle.h>
#include <NoobWarrior/Database/Record/IdType/DevProduct.h>
#include <NoobWarrior/Database/Record/IdType/Group.h>
#include <NoobWarrior/Database/Record/IdType/Pass.h>
#include <NoobWarrior/Database/Record/IdType/Set.h>
#include <NoobWarrior/Database/Record/IdType/Universe.h>
#include <NoobWarrior/Database/Record/IdType/User.h>

using namespace NoobWarrior;

REFLECT_ID_TYPE(Asset)
REFLECT_ID_TYPE(Badge)
REFLECT_ID_TYPE(Bundle)
REFLECT_ID_TYPE(DevProduct)
REFLECT_ID_TYPE(Group)
REFLECT_ID_TYPE(Pass)
REFLECT_ID_TYPE(Set)
REFLECT_ID_TYPE(Universe)
REFLECT_ID_TYPE(User)