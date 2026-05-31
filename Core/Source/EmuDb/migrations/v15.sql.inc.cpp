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
static const char *migration_v15 = R"***(
-- AssetPlaceGearType previously used PRIMARY KEY(Id) with a globally UNIQUE GearType, which
-- only allowed a single gear type and prevented two places from permitting the same one.
-- Recreate it with a composite key so a place can permit any number of gear types.
DROP TABLE IF EXISTS AssetPlaceGearType;
CREATE TABLE IF NOT EXISTS AssetPlaceGearType (
    Id	INTEGER,
    GearType	INTEGER,
    PRIMARY KEY(Id, GearType),
	FOREIGN KEY(Id) REFERENCES Asset(Id)
);
)***";