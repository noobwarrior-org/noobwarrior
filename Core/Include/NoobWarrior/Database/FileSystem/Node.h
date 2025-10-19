// === noobWarrior ===
// File: Node.h
// Started by: Hattozo
// Started on: 10/19/2025
// Description: A representation of a file or directory in the database.
#pragma once
#include <string>

namespace NoobWarrior {
enum class NodeType {
    File,
    Directory
};
struct Node {
    int Id;
    int ParentId;
    std::string Name;
    std::string Type;
};
}