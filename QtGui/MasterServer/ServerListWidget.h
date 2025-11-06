// === noobWarrior ===
// File: ServerListWidget.h
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Widget that contains a list of servers retrieved from the master server
#pragma once
#include <QWidget>

namespace NoobWarrior {
class ServerListWidget : public QWidget {
    Q_OBJECT
public:
    ServerListWidget(QWidget* parent = nullptr);
};
}