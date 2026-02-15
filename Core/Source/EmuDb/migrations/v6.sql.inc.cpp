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
static const char *migration_v6 = R"***(
CREATE TABLE "Outfit" (
    Id	INTEGER PRIMARY KEY,
	FirstRecorded	INTEGER DEFAULT (unixepoch()),
	LastRecorded	INTEGER DEFAULT (unixepoch()),
	Name	TEXT NOT NULL,
    Created	INTEGER,
	Updated	INTEGER,
    UserId	INTEGER,
    BodyType	INTEGER,
    Width  NUMERIC,
    Height  NUMERIC,
    Head  NUMERIC,
    Proportions  NUMERIC,
    FOREIGN KEY (UserId) REFERENCES User(Id)
);

CREATE TABLE "OutfitItem" (
    Id	INTEGER,
	AssetId	INTEGER,
    PRIMARY KEY (Id, AssetId),
    FOREIGN KEY (Id) REFERENCES Outfit(Id),
    FOREIGN KEY (AssetId) REFERENCES Asset(Id)
);

CREATE TABLE "OutfitBodyColor" (
    Id	INTEGER,
    BodyPart INTEGER,
	Color3	INTEGER NOT NULL,
    PRIMARY KEY (Id, BodyPart)
    FOREIGN KEY (Id) REFERENCES Outfit(Id)
);

CREATE VIRTUAL TABLE OutfitSearchIndex USING fts5 (id, name);
INSERT INTO OutfitSearchIndex (id, name) SELECT Id, Name FROM Outfit;

CREATE TRIGGER OutfitSearchIndexInsertTrigger AFTER INSERT ON Outfit
  BEGIN
    INSERT INTO OutfitSearchIndex (id, name) VALUES (new.Id, new.Name);
  END;
  
CREATE TRIGGER OutfitSearchIndexUpdateTrigger UPDATE OF Name ON Outfit
  BEGIN
    UPDATE OutfitSearchIndex SET Name = new.Name WHERE Id = old.Id AND Name = old.Name;
  END;

CREATE TRIGGER OutfitSearchIndexDeleteTrigger BEFORE DELETE ON Outfit
  BEGIN
    DELETE FROM OutfitSearchIndex WHERE Id = old.Id;
  END;
)***";