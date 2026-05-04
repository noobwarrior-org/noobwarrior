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
static const char *migration_v8 = R"***(
CREATE TABLE Forum (
    Id INTEGER PRIMARY KEY,
    Name TEXT NOT NULL,
    Description TEXT NOT NULL,
    ParentId INTEGER,
    SortOrder INTEGER DEFAULT 0,
    FOREIGN KEY (ParentId) REFERENCES Forum(Id)
);

CREATE TABLE ForumThread (
    Id INTEGER PRIMARY KEY,
    Subject TEXT NOT NULL,
    Created INTEGER NOT NULL,
    Poster INTEGER NOT NULL,
    ForumId INTEGER NOT NULL,
    FOREIGN KEY (Poster) REFERENCES User(Id),
    FOREIGN KEY (ForumId) REFERENCES Forum(Id)
);

CREATE TABLE ForumPost (
    Id INTEGER PRIMARY KEY,
    ThreadId INTEGER NOT NULL,
    Created INTEGER NOT NULL,
    Edited INTEGER,
    Poster INTEGER NOT NULL,
    Content TEXT NOT NULL,
    FOREIGN KEY (ThreadId) REFERENCES ForumThread(Id),
    FOREIGN KEY (Poster) REFERENCES User(Id)
);

CREATE VIRTUAL TABLE ForumThreadSearchIndex USING fts5 (id, subject);
INSERT INTO ForumThreadSearchIndex (id, subject) SELECT Id, Subject FROM ForumThread;

CREATE TRIGGER ForumThreadSearchIndexInsertTrigger AFTER INSERT ON ForumThread
  BEGIN
    INSERT INTO ForumThreadSearchIndex (id, subject) VALUES (new.Id, new.Subject);
  END;

CREATE TRIGGER ForumThreadSearchIndexUpdateTrigger AFTER UPDATE OF Subject ON ForumThread
  BEGIN
    UPDATE ForumThreadSearchIndex SET subject = new.Subject WHERE Id = old.Id;
  END;

CREATE TRIGGER ForumThreadSearchIndexDeleteTrigger BEFORE DELETE ON ForumThread
  BEGIN
    DELETE FROM ForumThreadSearchIndex WHERE id = old.Id;
  END;

CREATE VIRTUAL TABLE ForumPostSearchIndex USING fts5 (id, content);
INSERT INTO ForumPostSearchIndex (id, content) SELECT Id, Content FROM ForumPost;

CREATE TRIGGER ForumPostSearchIndexInsertTrigger AFTER INSERT ON ForumPost
  BEGIN
    INSERT INTO ForumPostSearchIndex (id, content) VALUES (new.Id, new.Content);
  END;

CREATE TRIGGER ForumPostSearchIndexUpdateTrigger AFTER UPDATE OF Content ON ForumPost
  BEGIN
    UPDATE ForumPostSearchIndex SET content = new.Content WHERE id = old.Id;
  END;

CREATE TRIGGER ForumPostSearchIndexDeleteTrigger BEFORE DELETE ON ForumPost
  BEGIN
    DELETE FROM ForumPostSearchIndex WHERE Id = old.Id;
  END;
)***";