// === noobWarrior ===
// File: ReflectionMetadata.cpp
// Started by: Hattozo
// Started on: 9/19/2025
// Description: Anything that requires runtime to dynamically understand the structure of data
// without having to manually specify it everywhere should go here
//
// ID types for example; you don't want to add checks in 30 different code paths because you added a new ID type.
// Just declare it here and those code paths will automatically discover it.
//
// Note: If you are adding a new ID type, you will still have to manually add glue code between the SQL database schema and
// the actual C structs. Check the Database.cpp & Database.h file in order to see how we did this.
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

NOOBWARRIOR_REFLECT_ID_TYPE(Asset)
NOOBWARRIOR_REFLECT_ID_TYPE(Badge)
NOOBWARRIOR_REFLECT_ID_TYPE(Bundle)
NOOBWARRIOR_REFLECT_ID_TYPE(DevProduct)
NOOBWARRIOR_REFLECT_ID_TYPE(Group)
NOOBWARRIOR_REFLECT_ID_TYPE(Pass)
NOOBWARRIOR_REFLECT_ID_TYPE(Set)
NOOBWARRIOR_REFLECT_ID_TYPE(Universe)
NOOBWARRIOR_REFLECT_ID_TYPE(User)

void NoobWarrior::Reflection::hi() {
    // put this here so that this source file can be referenced outside and therefore the compiler will be forced to link it and it wont be discarded
    // ps: this function is called in the constructor in Core.cpp
}