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
static const char *migration_v7 = R"***(
DROP TABLE AssetMisc;
ALTER TABLE Asset ADD COLUMN MinimumMembershipLevel INTEGER;
ALTER TABLE Asset ADD COLUMN ContentRatingTypeId INTEGER;

-- delete character appearance because there's a new guy in town
ALTER TABLE User DROP COLUMN CharacterAppearance;

-- and here he is
CREATE TABLE UserCharacterItem (
    Id	INTEGER,
	AssetId	INTEGER,
    PRIMARY KEY (Id, AssetId),
    FOREIGN KEY (Id) REFERENCES Outfit(Id),
    FOREIGN KEY (AssetId) REFERENCES Asset(Id)
);

CREATE TABLE UserCharacterBodyColor (
    Id	INTEGER,
    BodyPart INTEGER,
	Color3	INTEGER NOT NULL,
    PRIMARY KEY (Id, BodyPart),
    FOREIGN KEY (Id) REFERENCES Outfit(Id)
);

ALTER TABLE User ADD COLUMN CharacterBodyType INTEGER;
ALTER TABLE User ADD COLUMN CharacterWidth NUMERIC;
ALTER TABLE User ADD COLUMN CharacterHeight NUMERIC;
ALTER TABLE User ADD COLUMN CharacterHead NUMERIC;
ALTER TABLE User ADD COLUMN CharacterProportions NUMERIC;
)***";