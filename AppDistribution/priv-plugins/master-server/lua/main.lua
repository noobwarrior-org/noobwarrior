-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Master Server
-- File: main.lua
-- Description: Main entrypoint for Master Server plugin
-- Started by: Hattozo
-- Started on: 1/3/2026
-- ////////////////////////////////////////////////////////////////////////////////
master_db = SqlDb.new(":memory:", "MasterDb")
master_db:Query([[CREATE TABLE contacts (
	contact_id INTEGER PRIMARY KEY,
	first_name TEXT NOT NULL,
	last_name TEXT NOT NULL,
	email TEXT NOT NULL UNIQUE,
	phone TEXT NOT NULL UNIQUE
);]])
-- master_db:Query("INSERT INTO contacts (contact_id, first_name, last_name, email, phone) VALUES (1, 'mr', 'poop', 'poop@gmail.com', '0000000000');")
master_db:QueryTyped("INSERT INTO contacts (contact_id, first_name, last_name, email, phone) VALUES (?, ?, ?, ?, ?)", 1, "mr", "poop", "poop@gmail.com", "0000000000")
local rows = master_db:Query("SELECT * FROM contacts;")
for k, v in pairs(rows) do
    print("Row " .. k .. ": Contact Id: " .. tostring(v.contact_id) .. ", First Name: " .. v.first_name .. ", Last Name: " .. v.last_name .. ", Email: " .. v.email .. ", Phone: " .. v.phone)
end

local http_shared = require("plugin://http-shared@noobwarrior.org/lua/shared.lua")

local sitemap = {
    ["/"] = "plugin://master-server@noobwarrior.org/src/index.lhp",
    ["/home"] = "plugin://master-server@noobwarrior.org/src/index.lhp"
}

master = http_shared.CreateServer({
    Name = "MasterServer",
    Sitemap = sitemap
})
master:MountVolume("/", "/static") -- also mount our own static directory alongside the shared http server's
master:Start(4040)
