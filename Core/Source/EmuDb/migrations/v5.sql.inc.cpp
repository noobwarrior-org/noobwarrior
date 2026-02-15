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
static const char *migration_v5 = R"***(
PRAGMA foreign_keys = OFF;

CREATE VIRTUAL TABLE AssetSearchIndex USING fts5 (id, name);
INSERT INTO AssetSearchIndex (id, name) SELECT Id, Name FROM Asset;

CREATE TRIGGER AssetSearchIndexInsertTrigger AFTER INSERT ON Asset
  BEGIN
    INSERT INTO AssetSearchIndex (id, name) VALUES (new.Id, new.Name);
  END;
  
CREATE TRIGGER AssetSearchIndexUpdateTrigger UPDATE OF Name ON Asset
  BEGIN
    UPDATE AssetSearchIndex SET Name = new.Name WHERE Id = old.Id AND Name = old.Name;
  END;

CREATE TRIGGER AssetSearchIndexDeleteTrigger BEFORE DELETE ON Asset
  BEGIN
    DELETE FROM AssetSearchIndex WHERE Id = old.Id;
  END;
)***";