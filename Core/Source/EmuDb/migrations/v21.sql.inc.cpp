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
// V21: adds the AuthTicket table for the one-time game-join ticket flow. Tickets are single-use
// (Redeemed flips 0->1) and short-lived (TTL enforced in code against CreatedTimestamp).
static const char *migration_v21 = R"***(
CREATE TABLE IF NOT EXISTS "AuthTicket" (
    "Ticket"           TEXT NOT NULL UNIQUE,
    "UserId"           INTEGER NOT NULL,
    "PlaceId"          INTEGER,
    "CreatedTimestamp" INTEGER DEFAULT (unixepoch()),
    "Redeemed"         INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY("Ticket"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id")
);
)***";
