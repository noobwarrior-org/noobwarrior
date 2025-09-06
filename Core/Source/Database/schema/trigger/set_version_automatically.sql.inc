CREATE TRIGGER SetVersionAutomatically
BEFORE INSERT ON ImageId
FOR EACH ROW
BEGIN
    SELECT MAX("Version") INTO NEW."ImageVersion" FROM "Asset" WHERE Id = NEW."ImageId";
END;