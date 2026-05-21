-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Master Server
-- File: main.lua
-- Description: Main entrypoint for Master Server plugin
-- Started by: Hattozo
-- Started on: 1/3/2026
-- ////////////////////////////////////////////////////////////////////////////////
reg.SetKeyValueIfNotSet("master.autostart", false);
reg.SetKeyComment("master.autostart", "Set to true to automatically make the master server start listening on startup.");

reg.SetKeyValueIfNotSet("master.http_port", 80);
reg.SetKeyComment("master.http_port", "Port the master server HTTP listener binds to.");

reg.SetKeyValueIfNotSet("master.https_port", 443);
reg.SetKeyComment("master.https_port", "Port the master server HTTPS listener binds to.");

reg.SetKeyValueIfNotSet("master.branding.title", "noobWarrior Master Server");
reg.SetKeyValueIfNotSet("master.branding.icon", "/img/icon1024.png");
reg.SetKeyValueIfNotSet("master.branding.tagline", "My noobWarrior server");
reg.SetKeyComment("master.branding", "The branding that people will see when they connect to your master server.");

reg.SetKeyValueIfNotSet("master.workshop.max_upload_mb", 4096);
reg.SetKeyComment("master.workshop.max_upload_mb", "Maximum size in megabytes of a single .nwdb file uploaded to the workshop.");

core.ConsoleAdded:Connect(function(console)
    console:RegisterCommand("master", function(ctx)
        ctx:Reply("Hello from master server!")
    end, "Master server commands.")
end)

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

local http_base = require("plugin://http-base@noobwarrior.org/lua/base.lua")

local sitemap = {
    ["/"] = "plugin://master-server@noobwarrior.org/src/index.lhp",
    ["/home"] = "plugin://master-server@noobwarrior.org/src/index.lhp",
    ["/control-panel"] = "plugin://master-server@noobwarrior.org/src/controlpanel.lhp"
}

master = http_base.CreateServer({
    Name = "MasterServer",
    Sitemap = sitemap
})
master:MountVolume("/", "/static") -- also mount our own static directory alongside the shared http server's
if reg.GetKeyValue("master.autostart") then
    local http_port = reg.GetKeyValue("master.http_port") or 80
    local https_port = reg.GetKeyValue("master.https_port") or 443
    master:Start(http_port)
    master:StartSecure(https_port)
    print(string.format("Automatically started master server! HTTP server listening on port %d and HTTPS server listening on port %d", http_port, https_port))
end
