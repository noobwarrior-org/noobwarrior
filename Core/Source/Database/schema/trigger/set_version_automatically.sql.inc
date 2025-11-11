CREATE TRIGGER SetSnapshotAutomatically
BEFORE INSERT ON ImageId
FOR EACH ROW
BEGIN
    SELECT MAX("Snapshot") INTO NEW."ImageSnapshot" FROM "Asset" WHERE Id = NEW."ImageId";
END;