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
// File: EmuDbListWidget.h
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#pragma once
#include <QListWidget>
#include <NoobWarrior/EmuDb/EmuDb.h>

namespace NoobWarrior {
class EmuDbListWidget : public QListWidget {
    Q_OBJECT
public:
    enum class Mode {
        ShowEntriesInDir, // Shows every database file in the databases folder, even the ones that aren't mounted
        ShowMounted, // Shows only the currently mounted databases in the database manager
        ShowNotMounted, // Shows the databases not mounted in the database manager
        Manual // Shows nothing, you add stuff manually
    };

    static constexpr int SourceUrlRole = Qt::UserRole + 1;
    static constexpr int LockedRole = Qt::UserRole + 2;
    static constexpr int FilePathRole = Qt::UserRole + 3;

    EmuDbListWidget(Mode mode = Mode::ShowEntriesInDir, QWidget* parent = nullptr);
    ~EmuDbListWidget();
    
    static QIcon IconFromData(const std::vector<unsigned char>& iconData);

    void Refresh();
    void AddDatabase(EmuDb* db, bool isTemp = false);

    // Adds a row for a database a plugin offers but has not mounted. Its title and icon are read
    // out of the file by the caller, since there is no open EmuDb to ask.
    void AddOfferedDatabase(const QString& title, const QString& sourceUrl, const QString& pluginTitle,
                            const std::vector<unsigned char>& iconData = {});

    EmuDb* GetSelectedDatabase();
    QList<EmuDb*> GetSelectedDatabases();
signals:
    void filesDropped(const QStringList& filePaths);
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
private:
    Mode mMode;
};
}