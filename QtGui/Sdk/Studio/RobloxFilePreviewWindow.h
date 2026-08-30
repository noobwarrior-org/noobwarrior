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
// File: RobloxFilePreviewWindow.h
// Started by: Hattozo
// Started on: 8/29/2026
// Description: Standalone window that allows you to preview Roblox files by grouping Explorer and Properties widgets together
#pragma once
#include <QMainWindow>
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>

#include "ExplorerWidget.h"
#include "PropertiesWidget.h"

namespace NoobWarrior {
class RobloxFilePreviewWindow : public QMainWindow {
    Q_OBJECT
public:
    RobloxFilePreviewWindow(std::unique_ptr<Roblox::RobloxFile> file, const QString& filePath,
                            QWidget* parent = nullptr);

    // Parse and show a preview; the window frees itself when closed.
    static void OpenFromFile(QWidget* parent, const QString& filePath);
protected:
    void closeEvent(QCloseEvent* event) override; // prompts about unsaved edits
private:
    void Save(const QString& toPath);

    std::unique_ptr<Roblox::RobloxFile> mFile;
    QString mFilePath;
    ExplorerWidget* mExplorer;
    PropertiesWidget* mProperties;
    QDockWidget* mExplorerDock;
    QDockWidget* mPropertiesDock;
};
}