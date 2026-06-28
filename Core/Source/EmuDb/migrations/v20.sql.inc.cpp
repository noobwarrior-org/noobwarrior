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
// V20: fleshes out the FsNode table (the user-facing file manager tree) with creation/modification
// timestamps and a shortcut target. A node's Type is now one of DatabaseFileSystem::NodeType
// (0 = Directory, 1 = File, 2 = Shortcut). For a Shortcut, ShortcutItemType holds an ItemType value
// and ShortcutItemId the referenced item's Id. These are deliberately NOT foreign keys: the target
// table varies with ShortcutItemType, and a dangling shortcut should simply render as broken rather
// than block a delete. ADD COLUMN defaults must be constant, so the timestamps default to 0 and are
// filled with unixepoch() at insert time.
static const char *migration_v20 = R"***(
ALTER TABLE FsNode ADD COLUMN CreatedAt INTEGER NOT NULL DEFAULT 0;
ALTER TABLE FsNode ADD COLUMN ModifiedAt INTEGER NOT NULL DEFAULT 0;
ALTER TABLE FsNode ADD COLUMN ShortcutItemType INTEGER;
ALTER TABLE FsNode ADD COLUMN ShortcutItemId INTEGER;
)***";
