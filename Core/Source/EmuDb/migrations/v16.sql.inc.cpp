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
static const char *migration_v16 = R"***(
-- UniverseSocialLink previously used PRIMARY KEY(Id), allowing a single social link per
-- universe. Recreate it keyed by (Id, LinkType) so a universe can have one link per platform.
DROP TABLE IF EXISTS UniverseSocialLink;
CREATE TABLE IF NOT EXISTS UniverseSocialLink (
    Id	INTEGER,
    LinkType	INTEGER NOT NULL,
    Url	TEXT NOT NULL,
    Title	TEXT NOT NULL,
    PRIMARY KEY(Id, LinkType),
	FOREIGN KEY(Id) REFERENCES Universe(Id)
);
)***";